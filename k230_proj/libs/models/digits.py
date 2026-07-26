# 数字识别模型 (来自 01Studio 在线训练产物 #9322)
#
# 标签: {0:'1', 1:'2', ..., 7:'8'} — 只识别 1-8, 没有 0/9
# 应用场景: 七段数码管 / 电表读数等

from libs.detect import ModelSpec, INPUT_SIZE


DIGIT_LABELS = ['1', '2', '3', '4', '5', '6', '7', '8']


def _digit_renderer(osd, ctx):
    """数字模型专属 OSD: 顶部大字显示识别到的数字 + 画框.

    ctx: {"dets": [...], "labels": [...], "width": int}
    dets 元素: (x, y, w, h, score, cls)"""
    dets   = ctx.get("dets") or []
    labels = ctx.get("labels") or []
    dw     = ctx.get("width", 640)

    if not dets:
        osd.draw_string_advanced(dw // 2 - 80, 8, 32,
            "no digit", color=(180, 180, 180))
        return

    # 选置信度最高的
    best = max(dets, key=lambda d: d[4])
    x, y, w, h, score, cls = best
    digit = labels[int(cls)] if int(cls) < len(labels) else '?'

    # 顶部居中显示大数字
    big = "DETECTED: {} ({:.0%})".format(digit, score)
    color = (50, 220, 50) if score >= 0.7 else (255, 200, 50)
    osd.draw_string_advanced(dw // 2 - 180, 8, 48, big, color=color)

    # 框出数字位置
    osd.draw_rectangle(x, y, w, h, color=color, thickness=4)
    osd.draw_string_advanced(x, max(0, y - 24), 20,
        "{}  {:.2f}".format(digit, score), color=color)


def make():
    return ModelSpec(
        name         = "digits",
        kmodel_candidates = [
            "/sdcard/models/yolo11n_digits_320.kmodel",  # 新标准 (9322 重命名)
            "/sdcard/models/yolo11n_det_320.kmodel",     # 别名 (9322 原始名)
            "/sdcard/yolo11n_det_320.kmodel",            # 9322 demo 默认
        ],
        labels      = DIGIT_LABELS,
        input_size  = INPUT_SIZE,
        yolo_version = "v11",
        conf_thresh = 0.6,            # 数字模型阈值高一些, 减少误识
        nms_thresh  = 0.45,
        renderer    = _digit_renderer,
    )