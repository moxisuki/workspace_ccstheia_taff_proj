# 数字识别 1-8 #9322

from libs.detect import ModelSpec, INPUT_SIZE


DIGIT_LABELS = ['1', '2', '3', '4', '5', '6', '7', '8']


def _digit_renderer(osd, ctx):
    dets   = ctx.get("dets") or []
    labels = ctx.get("labels") or []
    dw     = ctx.get("width", 640)

    if not dets:
        osd.draw_string_advanced(dw // 2 - 80, 8, 32,
            "no digit", color=(180, 180, 180))
        return

    best = max(dets, key=lambda d: d[4])
    x, y, w, h, score, cls = best
    digit = labels[int(cls)] if int(cls) < len(labels) else '?'

    big = "DETECTED: {} ({:.0%})".format(digit, score)
    color = (50, 220, 50) if score >= 0.7 else (255, 200, 50)
    osd.draw_string_advanced(dw // 2 - 180, 8, 48, big, color=color)

    osd.draw_rectangle(x, y, w, h, color=color, thickness=4)
    osd.draw_string_advanced(x, max(0, y - 24), 20,
        "{}  {:.2f}".format(digit, score), color=color)


def make():
    return ModelSpec(
        name         = "digits",
        kmodel_candidates = [
            "/sdcard/models/digits/yolo11n_digits_320.kmodel",
        ],
        labels      = DIGIT_LABELS,
        input_size  = INPUT_SIZE,
        yolo_version = "v11",
        conf_thresh = 0.6,
        nms_thresh  = 0.45,
        renderer    = _digit_renderer,
    )