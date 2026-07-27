#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
taff_proj PID 调参上位机

串口协议:
    MCU 每行输出一帧:
        DBG,t=123,app=LineTrace,drv=LineTrace,base=260,steer=-35,...

数值约定:
    yaw10 / yr10 / hd10 / lc10 / le10 为原值 x10, 上位机显示时自动除以 10。

依赖:
    pip install pyserial matplotlib numpy

运行:
    python tune_host.py --port COM11
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
    from matplotlib import font_manager
    import matplotlib.pyplot as plt
    from matplotlib.animation import FuncAnimation
    from matplotlib.widgets import Button, TextBox
except ImportError:
    sys.exit("缺少 matplotlib, 请先: pip install matplotlib")

FONT_CANDIDATES = (
    "Microsoft YaHei", "SimHei", "DengXian", "SimSun",
    "Noto Sans CJK SC", "Arial Unicode MS", "DejaVu Sans",
)


def configure_fonts():
    installed = {f.name for f in font_manager.fontManager.ttflist}
    chosen = next((name for name in FONT_CANDIDATES if name in installed), "DejaVu Sans")
    matplotlib.rcParams["font.family"] = [chosen]
    matplotlib.rcParams["font.sans-serif"] = [chosen, *FONT_CANDIDATES]
    matplotlib.rcParams["font.monospace"] = [chosen, "Consolas", "DejaVu Sans Mono"]


configure_fonts()
matplotlib.rcParams["axes.unicode_minus"] = False


SCALED10_KEYS = {"yaw10", "yr10", "hd10", "lc10", "le10"}
DEFAULT_HISTORY = 120
DEFAULT_UPDATE_MS = 250
HISTORY = DEFAULT_HISTORY
UPDATE_MS = DEFAULT_UPDATE_MS

BG = "#f6f8fb"
PANEL = "#ffffff"
GRID = "#e7ecf2"
TEXT = "#243042"
MUTED = "#6b7788"
BLUE = "#2563eb"
GREEN = "#16a34a"
RED = "#dc2626"
ORANGE = "#f59e0b"
PURPLE = "#7c3aed"
CYAN = "#0891b2"


def parse_value(key: str, value: str):
    if key in {"app", "drv"}:
        return value
    try:
        num = int(value)
    except ValueError:
        try:
            return float(value)
        except ValueError:
            return value
    return num / 10.0 if key in SCALED10_KEYS else num


def parse_dbg(line: bytes):
    text = line.decode("ascii", errors="ignore").strip()
    if not text.startswith("DBG"):
        return None

    parts = text.split(",")
    data = {}
    for part in parts[1:]:
        if "=" not in part:
            continue
        key, value = part.split("=", 1)
        data[key] = parse_value(key, value)
    return data if data else None


class TuneReader(threading.Thread):
    def __init__(self, port: str, baud: int):
        super().__init__(daemon=True)
        self.ser = serial.Serial(port, baud, timeout=1)
        self.lock = threading.Lock()
        self.running = True
        self.count = 0
        self.last = {}
        self.last_response = ""
        self.rows = deque(maxlen=HISTORY)
        self.responses = deque(maxlen=8)

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
            while b"\n" in buf:
                line, buf = buf.split(b"\n", 1)
                data = parse_dbg(line)
                if data is None:
                    text = line.decode("ascii", errors="ignore").strip()
                    if text:
                        with self.lock:
                            self.last_response = text
                            self.responses.append(text)
                    continue
                with self.lock:
                    self.last = data
                    self.rows.append(data)
                    self.count += 1

    def snapshot(self):
        with self.lock:
            return list(self.rows), dict(self.last), self.count, self.last_response

    def close(self):
        self.running = False
        try:
            self.ser.close()
        except Exception:
            pass

    def send_command(self, text: str):
        cmd = text.strip()
        if not cmd:
            return
        if not cmd.endswith("\r\n"):
            cmd += "\r\n"
        self.ser.write(cmd.encode("ascii", errors="ignore"))


def list_ports_and_exit():
    ports = list(list_ports.comports())
    if not ports:
        sys.exit("未找到串口, 请插好调试串口后重试")
    print("可用串口:")
    for p in ports:
        print(f"  {p.device}  -  {p.description}")
    sys.exit("请用 --port 指定, 例如: python tune_host.py --port COM11")


def series(rows, key, default=0.0):
    return np.array([float(r.get(key, default)) for r in rows], dtype=np.float64)


def latest_text(last, fps):
    def f(key, default="-"):
        return last.get(key, default)

    return (
        f"app={f('app')}  drv={f('drv')}  fps={fps:4.1f}\n"
        f"base={f('base')}  steer={f('steer')}  target={f('target')}  turning={f('turning')}\n"
        f"line={f('line')}  center={f('lc10')}  err={f('le10')}  "
        f"th={f('lth')}  min/max={f('lmin')}/{f('lmax')}\n"
        f"yaw={f('yaw10')}  yaw_rate={f('yr10')}  heading_err={f('hd10')}\n"
        f"m1={f('m1')}  m2={f('m2')}  m3={f('m3')}  m4={f('m4')}"
    )


def fmt_value(value, digits=1, default="-"):
    if value is None:
        return default
    if isinstance(value, float):
        return f"{value:.{digits}f}"
    return str(value)


def style_axis(ax, title: str, ylabel: str):
    ax.set_facecolor(PANEL)
    ax.set_title(title, loc="left", color=TEXT, fontsize=11, fontweight="bold", pad=8)
    ax.set_ylabel(ylabel, color=MUTED, fontsize=10)
    ax.grid(True, color=GRID, linewidth=0.9)
    ax.tick_params(axis="both", colors=MUTED, labelsize=9)
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    ax.spines["left"].set_color("#d5dce6")
    ax.spines["bottom"].set_color("#d5dce6")
    ax.axhline(0, color="#aab4c2", lw=0.9, zorder=0)


def style_widget(widget):
    for name in ("label", "text_disp"):
        artist = getattr(widget, name, None)
        if artist is not None:
            artist.set_color(TEXT)
            artist.set_fontsize(9)
            artist.set_fontfamily(matplotlib.rcParams["font.family"][0])
    if hasattr(widget, "cursor"):
        widget.cursor.set_color(BLUE)


def make_card(ax, x, title, color):
    return ax.text(
        x, 0.52, f"{title}\n-",
        transform=ax.transAxes,
        ha="left",
        va="center",
        fontsize=10,
        color=TEXT,
        linespacing=1.55,
        bbox={
            "boxstyle": "round,pad=0.55,rounding_size=0.08",
            "facecolor": PANEL,
            "edgecolor": color,
            "linewidth": 1.2,
        },
    )


def main():
    global HISTORY, UPDATE_MS

    ap = argparse.ArgumentParser(description="taff_proj PID 调参上位机")
    ap.add_argument("--port", help="串口号, 如 COM11")
    ap.add_argument("--baud", type=int, default=115200, help="波特率, 默认 115200")
    ap.add_argument("--history", type=int, default=DEFAULT_HISTORY,
                    help=f"曲线历史帧数, 默认 {DEFAULT_HISTORY}; 卡顿时可降到 80")
    ap.add_argument("--interval", type=int, default=DEFAULT_UPDATE_MS,
                    help=f"界面刷新间隔 ms, 默认 {DEFAULT_UPDATE_MS}; 卡顿时可升到 300")
    args = ap.parse_args()
    HISTORY = max(40, min(args.history, 400))
    UPDATE_MS = max(80, min(args.interval, 1000))

    if not args.port:
        list_ports_and_exit()

    try:
        reader = TuneReader(args.port, args.baud)
    except Exception as e:
        sys.exit(f"打开串口 {args.port} 失败: {e}")

    reader.start()
    print(f"已连接 {args.port} @ {args.baud}, 关闭窗口或按 q 退出")

    fig = plt.figure(figsize=(13.5, 8.7), facecolor=BG)
    fig.canvas.manager.set_window_title("taff_proj PID 调参")
    fig.suptitle("taff_proj PID 调参面板", x=0.02, y=0.985, ha="left",
                 color=TEXT, fontsize=15, fontweight="bold")

    gs = fig.add_gridspec(
        5, 1,
        height_ratios=[0.95, 1.35, 1.35, 1.35, 0.28],
        left=0.06,
        right=0.985,
        top=0.925,
        bottom=0.245,
        hspace=0.48,
    )
    status_ax = fig.add_subplot(gs[0])
    axs = [fig.add_subplot(gs[i]) for i in range(1, 4)]
    hint_ax = fig.add_subplot(gs[4])

    status_ax.set_facecolor(BG)
    status_ax.axis("off")
    hint_ax.set_facecolor(BG)
    hint_ax.axis("off")

    x = np.arange(HISTORY)
    style_axis(axs[0], "循迹闭环", "px / steer")
    style_axis(axs[1], "MPU 航向闭环", "deg / deg/s")
    style_axis(axs[2], "速度目标与电机反馈", "cmd / rpm")
    axs[2].set_xlabel(f"最近 {HISTORY} 帧", color=MUTED, fontsize=10)
    axs[0].set_ylim(-520, 520)
    axs[1].set_ylim(-220, 220)
    axs[2].set_ylim(-1100, 1100)

    (line_err_plot,) = axs[0].plot(x, np.zeros(HISTORY), color=BLUE, lw=1.8, label="line.err")
    (steer_plot,) = axs[0].plot(x, np.zeros(HISTORY), color=ORANGE, lw=1.7, label="steer")
    axs[0].legend(loc="upper right", frameon=False, ncols=2, fontsize=9)

    (yaw_plot,) = axs[1].plot(x, np.zeros(HISTORY), color=PURPLE, lw=1.7, label="yaw")
    (hd_plot,) = axs[1].plot(x, np.zeros(HISTORY), color=RED, lw=1.8, label="heading.err")
    (yr_plot,) = axs[1].plot(x, np.zeros(HISTORY), color=CYAN, lw=1.35, label="yaw_rate")
    axs[1].legend(loc="upper right", frameon=False, ncols=3, fontsize=9)

    (base_plot,) = axs[2].plot(x, np.zeros(HISTORY), color=TEXT, lw=1.8, label="base")
    (m1_plot,) = axs[2].plot(x, np.zeros(HISTORY), color=BLUE, lw=1.2, alpha=0.85, label="m1")
    (m2_plot,) = axs[2].plot(x, np.zeros(HISTORY), color=GREEN, lw=1.2, alpha=0.85, label="m2")
    (m3_plot,) = axs[2].plot(x, np.zeros(HISTORY), color=ORANGE, lw=1.2, alpha=0.85, label="m3")
    (m4_plot,) = axs[2].plot(x, np.zeros(HISTORY), color=PURPLE, lw=1.2, alpha=0.85, label="m4")
    axs[2].legend(loc="upper right", frameon=False, ncols=5, fontsize=9)

    cards = {
        "mode": make_card(status_ax, 0.00, "模式", BLUE),
        "drive": make_card(status_ax, 0.19, "输出", ORANGE),
        "line": make_card(status_ax, 0.38, "CCD", GREEN),
        "mpu": make_card(status_ax, 0.57, "MPU", PURPLE),
        "motor": make_card(status_ax, 0.76, "电机", CYAN),
    }
    hint_text = hint_ax.text(
        0.0, 0.45,
        "LED: 程序运行白灯常亮, Stop 额外亮红灯, 参数变更蓝灯短闪; 默认低刷新+短历史以减少卡顿",
        transform=hint_ax.transAxes,
        color=MUTED,
        fontsize=10,
        va="center",
    )
    line_p_ax = fig.add_axes([0.06, 0.145, 0.07, 0.04], facecolor=PANEL)
    line_i_ax = fig.add_axes([0.145, 0.145, 0.07, 0.04], facecolor=PANEL)
    line_d_ax = fig.add_axes([0.23, 0.145, 0.07, 0.04], facecolor=PANEL)
    line_btn_ax = fig.add_axes([0.315, 0.145, 0.09, 0.04], facecolor=PANEL)
    head_p_ax = fig.add_axes([0.455, 0.145, 0.07, 0.04], facecolor=PANEL)
    head_i_ax = fig.add_axes([0.54, 0.145, 0.07, 0.04], facecolor=PANEL)
    head_d_ax = fig.add_axes([0.625, 0.145, 0.07, 0.04], facecolor=PANEL)
    head_btn_ax = fig.add_axes([0.71, 0.145, 0.09, 0.04], facecolor=PANEL)
    status_btn_ax = fig.add_axes([0.815, 0.145, 0.07, 0.04], facecolor=PANEL)
    cmd_ax = fig.add_axes([0.06, 0.075, 0.70, 0.04], facecolor=PANEL)
    send_ax = fig.add_axes([0.775, 0.075, 0.09, 0.04], facecolor=PANEL)
    stop_ax = fig.add_axes([0.875, 0.075, 0.09, 0.04], facecolor=PANEL)
    cmd_box = TextBox(cmd_ax, "", initial="STATUS", color=PANEL, hovercolor="#eef3fb")
    send_btn = Button(send_ax, "发送", color=PANEL, hovercolor="#eef3fb")
    stop_btn = Button(stop_ax, "STOP", color=PANEL, hovercolor="#fee2e2")
    line_p = TextBox(line_p_ax, "Line P", initial="5.0", color=PANEL, hovercolor="#eef3fb")
    line_i = TextBox(line_i_ax, "I", initial="0.0", color=PANEL, hovercolor="#eef3fb")
    line_d = TextBox(line_d_ax, "D", initial="0.12", color=PANEL, hovercolor="#eef3fb")
    line_btn = Button(line_btn_ax, "Apply Line", color=PANEL, hovercolor="#eef3fb")
    head_p = TextBox(head_p_ax, "Head P", initial="2.0", color=PANEL, hovercolor="#eef3fb")
    head_i = TextBox(head_i_ax, "I", initial="0.05", color=PANEL, hovercolor="#eef3fb")
    head_d = TextBox(head_d_ax, "D", initial="0.05", color=PANEL, hovercolor="#eef3fb")
    head_btn = Button(head_btn_ax, "Apply Head", color=PANEL, hovercolor="#eef3fb")
    status_btn = Button(status_btn_ax, "STATUS", color=PANEL, hovercolor="#eef3fb")
    for widget in (
        cmd_box, send_btn, stop_btn,
        line_p, line_i, line_d, line_btn,
        head_p, head_i, head_d, head_btn,
        status_btn,
    ):
        style_widget(widget)

    for widget_ax in (
        line_p_ax, line_i_ax, line_d_ax, line_btn_ax,
        head_p_ax, head_i_ax, head_d_ax, head_btn_ax,
        status_btn_ax, cmd_ax, send_ax, stop_ax,
    ):
        for spine in widget_ax.spines.values():
            spine.set_edgecolor("#d5dce6")

    paused = {"value": False}
    fps_hist = deque(maxlen=20)
    last_counter = {"count": 0, "time": time.time()}
    last_render = {"count": -1, "response": ""}
    artists = (
        line_err_plot, steer_plot,
        yaw_plot, hd_plot, yr_plot,
        base_plot, m1_plot, m2_plot, m3_plot, m4_plot,
        cards["mode"], cards["drive"], cards["line"], cards["mpu"], cards["motor"],
        hint_text,
    )

    def on_key(event):
        if event.key == " ":
            paused["value"] = not paused["value"]
        elif event.key == "q":
            plt.close(fig)

    fig.canvas.mpl_connect("key_press_event", on_key)

    def send_current(_=None):
        reader.send_command(cmd_box.text)

    def send_stop(_=None):
        reader.send_command("STOP")

    def apply_line(_=None):
        reader.send_command(f"SET LINE P:{line_p.text} I:{line_i.text} D:{line_d.text}")

    def apply_head(_=None):
        reader.send_command(f"SET HEAD P:{head_p.text} I:{head_i.text} D:{head_d.text}")

    cmd_box.on_submit(send_current)
    send_btn.on_clicked(send_current)
    stop_btn.on_clicked(send_stop)
    line_btn.on_clicked(apply_line)
    head_btn.on_clicked(apply_head)
    status_btn.on_clicked(lambda _=None: reader.send_command("STATUS"))

    def padded(vals):
        if len(vals) >= HISTORY:
            return vals[-HISTORY:]
        pad = np.full(HISTORY - len(vals), np.nan)
        return np.concatenate([pad, vals])

    def update(_):
        if paused["value"]:
            hint_text.set_color(RED)
            hint_text.set_text("已暂停: 空格继续, q退出")
            return artists

        rows, last, count, last_response = reader.snapshot()
        now = time.time()
        dt = now - last_counter["time"]
        if dt >= 0.5:
            fps_hist.append((count - last_counter["count"]) / dt)
            last_counter["count"] = count
            last_counter["time"] = now
        fps = float(np.mean(fps_hist)) if fps_hist else 0.0

        if not rows:
            return artists

        if count == last_render["count"] and last_response == last_render["response"]:
            return artists
        last_render["count"] = count
        last_render["response"] = last_response

        line_err_plot.set_ydata(padded(series(rows, "le10")))
        steer_plot.set_ydata(padded(series(rows, "steer")))
        yaw_plot.set_ydata(padded(series(rows, "yaw10")))
        hd_plot.set_ydata(padded(series(rows, "hd10")))
        yr_plot.set_ydata(padded(series(rows, "yr10")))
        base_plot.set_ydata(padded(series(rows, "base")))
        m1_plot.set_ydata(padded(series(rows, "m1")))
        m2_plot.set_ydata(padded(series(rows, "m2")))
        m3_plot.set_ydata(padded(series(rows, "m3")))
        m4_plot.set_ydata(padded(series(rows, "m4")))

        cards["mode"].set_text(
            "模式\n"
            f"{last.get('app', '-')} / {last.get('drv', '-')}\n"
            f"{fps:4.1f} fps"
        )
        cards["drive"].set_text(
            "输出\n"
            f"base {fmt_value(last.get('base'))}  steer {fmt_value(last.get('steer'))}\n"
            f"target {fmt_value(last.get('target'))}  turn {fmt_value(last.get('turning'))}"
        )
        cards["line"].set_text(
            "CCD\n"
            f"valid {fmt_value(last.get('line'))}  center {fmt_value(last.get('lc10'))}\n"
            f"err {fmt_value(last.get('le10'))}  th {fmt_value(last.get('lth'))}"
        )
        cards["mpu"].set_text(
            "MPU\n"
            f"yaw {fmt_value(last.get('yaw10'))}  rate {fmt_value(last.get('yr10'))}\n"
            f"heading err {fmt_value(last.get('hd10'))}"
        )
        cards["motor"].set_text(
            "电机反馈\n"
            f"m1 {fmt_value(last.get('m1'))}  m2 {fmt_value(last.get('m2'))}\n"
            f"m3 {fmt_value(last.get('m3'))}  m4 {fmt_value(last.get('m4'))}"
        )
        hint_text.set_color(RED if paused["value"] else MUTED)
        hint_text.set_text(
            "已暂停: 空格继续, q退出"
            if paused["value"]
            else (last_response or "LED: 程序运行白灯常亮, Stop 额外亮红灯, 参数变更蓝灯短闪; 默认低刷新+短历史以减少卡顿")
        )
        return artists

    anim = FuncAnimation(fig, update, interval=UPDATE_MS, blit=True, cache_frame_data=False)
    try:
        plt.show()
    finally:
        reader.close()
        _ = anim
        print("已退出")


if __name__ == "__main__":
    main()
