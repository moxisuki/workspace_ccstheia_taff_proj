#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TSL1401 线性 CCD 上位机 (配合 taff_proj MSPM0G3507 固件)

串口协议 (与固件 task/ccd_debug_task.cpp 一致):
    每帧一行, 逗号分隔 128 个像素亮度(0..255), 以 '\n' 结尾:
        v0,v1,...,v127\n
    波特率 115200, 8N1。

功能 (对标厂家 CCD 上位机):
    - 实时显示 128 像素亮度剖面曲线 (横轴=像素序号 0~127, 纵轴=亮度 0~255)
    - 动态阈值 (max+min)/2, 阈值线
    - 二值化, 标出黑线区域 + 中线 (ZHONGZHI, 64=居中)
    - 顶部显示 阈值 / 中线 / 帧率
交互:
    空格 = 暂停/继续    i = 反转黑白极性(白线场景)    q = 退出

依赖:  pip install pyserial matplotlib numpy
运行:  python ccd_host.py --port COM11
       (不带 --port 时会列出可用串口)
"""
import argparse
import sys
import threading
import time
from collections import deque

import numpy as np

try:
    import serial
    import serial.tools.list_ports as list_ports
except ImportError:
    sys.exit("缺少 pyserial, 请先: pip install pyserial")

try:
    import matplotlib
    import matplotlib.pyplot as plt
    from matplotlib.animation import FuncAnimation
except ImportError:
    sys.exit("缺少 matplotlib, 请先: pip install matplotlib")

# 让 matplotlib 正常显示中文(否则中文变成 □□), 按常见 Windows 字体优先
matplotlib.rcParams["font.sans-serif"] = [
    "Microsoft YaHei", "SimHei", "DengXian", "SimSun", "Arial Unicode MS"
]
matplotlib.rcParams["axes.unicode_minus"] = False

N_PIX = 128          # 像素数
VMAX = 255           # 8 位亮度上限


# ----------------------------------------------------------------------------
# 串口读取线程: 后台不停读行, 解析成一帧 128 int, 存到 self.frame
# ----------------------------------------------------------------------------
class CcdReader(threading.Thread):
    def __init__(self, port, baud):
        super().__init__(daemon=True)
        self.ser = serial.Serial(port, baud, timeout=1)
        self.frame = np.zeros(N_PIX, dtype=np.int16)   # 最近一帧
        self.lock = threading.Lock()
        self.running = True
        self.frame_count = 0

    def run(self):
        buf = b""
        while self.running:
            try:
                chunk = self.ser.read(256)
            except Exception:
                break
            if not chunk:
                continue
            buf += chunk
            # 按 '\n' 切帧, 保留最后不完整的一段
            while b"\n" in buf:
                line, buf = buf.split(b"\n", 1)
                self._parse(line)

    def _parse(self, line: bytes):
        s = line.decode("ascii", errors="ignore").strip()
        if not s:
            return
        # 兼容可能的 "标签:" 前缀
        if ":" in s:
            s = s.rsplit(":", 1)[-1]
        parts = s.split(",")
        if len(parts) != N_PIX:      # 不完整/异常帧直接丢弃, 自动重同步
            return
        try:
            vals = np.array([int(p) for p in parts], dtype=np.int16)
        except ValueError:
            return
        with self.lock:
            self.frame = vals
            self.frame_count += 1

    def get(self):
        with self.lock:
            return self.frame.copy(), self.frame_count

    def close(self):
        self.running = False
        try:
            self.ser.close()
        except Exception:
            pass


# ----------------------------------------------------------------------------
# 线检测: 动态阈值 + 二值化 + 加权质心求中线
# ----------------------------------------------------------------------------
def analyze(frame: np.ndarray, invert: bool):
    lo, hi = int(frame.min()), int(frame.max())
    threshold = (lo + hi) // 2

    # invert=False: 黑线=低亮度(<阈值); invert=True: 目标=高亮度(白线)
    if not invert:
        mask = frame < threshold
        weight = np.clip(threshold - frame, 0, None).astype(np.float64)
    else:
        mask = frame > threshold
        weight = np.clip(frame - threshold, 0, None).astype(np.float64)

    idx = np.arange(N_PIX)
    wsum = weight.sum()
    center = float((idx * weight).sum() / wsum) if wsum > 1e-6 else float("nan")
    return threshold, mask, center, (hi - lo)


def main():
    ap = argparse.ArgumentParser(description="TSL1401 CCD 上位机")
    ap.add_argument("--port", help="串口号, 如 COM11")
    ap.add_argument("--baud", type=int, default=115200, help="波特率 (默认 115200)")
    args = ap.parse_args()

    if not args.port:
        ports = list(list_ports.comports())
        if not ports:
            sys.exit("未找到任何串口, 请插好 USB 后重试")
        print("可用串口:")
        for p in ports:
            print(f"  {p.device}  -  {p.description}")
        sys.exit("请用 --port 指定, 例如: python ccd_host.py --port COM11")

    try:
        reader = CcdReader(args.port, args.baud)
    except Exception as e:
        sys.exit(f"打开串口 {args.port} 失败: {e}\n(是否被 VOFA+/串口助手占用? 一次只能一个程序打开)")
    reader.start()
    print(f"已连接 {args.port} @ {args.baud}, 关闭窗口或按 q 退出")

    # ---- 绘图 ----
    state = {"pause": False, "invert": False}
    x = np.arange(N_PIX)

    fig, ax = plt.subplots(figsize=(10, 5))
    fig.canvas.manager.set_window_title("TSL1401 线性CCD 上位机")
    (line_plot,) = ax.plot(x, np.zeros(N_PIX), "-", lw=1.5, color="#1f77b4", label="亮度")
    (th_line,) = ax.plot([0, N_PIX - 1], [0, 0], "--", lw=1.0, color="#ff7f0e", label="阈值")
    center_line = ax.axvline(64, color="#d62728", lw=1.5, label="中线")
    fill = [ax.axvspan(0, 0, color="#d62728", alpha=0.12)]  # 黑线区域高亮(用 list 便于替换)

    ax.set_xlim(0, N_PIX - 1)
    ax.set_ylim(-5, VMAX + 5)
    ax.set_xlabel("像素序号 (0~127)")
    ax.set_ylabel("亮度 (0~255)")
    ax.grid(True, alpha=0.3)
    ax.legend(loc="upper right")
    title = ax.set_title("等待数据...")

    # 帧率统计
    fps_hist = deque(maxlen=30)
    last = {"cnt": 0, "t": time.time()}

    def on_key(event):
        if event.key == " ":
            state["pause"] = not state["pause"]
        elif event.key == "i":
            state["invert"] = not state["invert"]
        elif event.key == "q":
            plt.close(fig)

    fig.canvas.mpl_connect("key_press_event", on_key)

    def update(_):
        if state["pause"]:
            return line_plot, th_line, center_line, title
        frame, cnt = reader.get()
        line_plot.set_ydata(frame)

        th, mask, center, contrast = analyze(frame, state["invert"])
        th_line.set_ydata([th, th])
        if not np.isnan(center):
            center_line.set_xdata([center, center])

        # 更新黑线高亮区域(取 mask 的最小/最大索引作为区间)
        fill[0].remove()
        if mask.any():
            i0, i1 = int(np.argmax(mask)), N_PIX - 1 - int(np.argmax(mask[::-1]))
            fill[0] = ax.axvspan(i0, i1, color="#d62728", alpha=0.12)
        else:
            fill[0] = ax.axvspan(0, 0, color="#d62728", alpha=0.0)

        # 帧率
        now = time.time()
        dt = now - last["t"]
        if dt >= 0.5:
            fps_hist.append((cnt - last["cnt"]) / dt)
            last["cnt"], last["t"] = cnt, now
        fps = np.mean(fps_hist) if fps_hist else 0.0

        c_txt = f"{center:5.1f}" if not np.isnan(center) else " --- "
        pol = "白线" if state["invert"] else "黑线"
        title.set_text(
            f"阈值={th:3d}  中线(ZHONGZHI)={c_txt} (64=居中)  对比度={contrast:3d}  "
            f"帧率={fps:4.1f}fps  [{pol}] 空格暂停 i反相 q退出"
        )
        return line_plot, th_line, center_line, title

    # blit=False 因为高亮区块每帧重建
    _anim = FuncAnimation(fig, update, interval=30, blit=False, cache_frame_data=False)
    try:
        plt.show()
    finally:
        reader.close()
        print("已退出")


if __name__ == "__main__":
    main()
