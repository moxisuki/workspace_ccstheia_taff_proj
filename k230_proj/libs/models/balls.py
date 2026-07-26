# 钢球识别模型 (来自 01Studio 在线训练产物 #9340)
#
# 标签: {0: 'ball'} — 单类钢球检测
# 应用场景: 钢球定位、抓取引导

from libs.detect import ModelSpec, INPUT_SIZE


BALL_LABELS = ['ball']


def _ball_renderer(osd, ctx):
    """钢球模型专属 OSD: 画框 + 置信度 + 计数.

    ctx: {"dets": [...], "labels": [...], "width": int}
    dets 元素: (x, y, w, h, score, cls)"""
    dets   = ctx.get("dets") or []
    labels = ctx.get("labels") or []
    dw     = ctx.get("width", 640)

    n = len(dets)

    # 顶部居中: 钢球计数
    count_text = "BALLS: {}".format(n)
    count_color = (50, 220, 50) if n > 0 else (180, 180, 180)
    osd.draw_string_advanced(dw // 2 - 60, 40, 36, count_text, color=count_color)

    if n == 0:
        osd.draw_string_advanced(dw // 2 - 80, 80, 26,
            "no ball", color=(180, 180, 180))
        return

    # 每个球画框 + 置信度, 颜色按置信度分档
    for det in dets:
        x, y, w, h, score, cls = det
        label = labels[int(cls)] if int(cls) < len(labels) else '?'

        if score >= 0.7:
            color = (50, 220, 50)       # 绿: 高置信
        elif score >= 0.5:
            color = (255, 200, 50)      # 黄: 中置信
        else:
            color = (50, 180, 255)      # 蓝: 低置信

        osd.draw_rectangle(x, y, w, h, color=color, thickness=3)
        osd.draw_string_advanced(x, max(0, y - 22), 20,
            "{} {:.0%}".format(label, score), color=color)


def make():
    return ModelSpec(
        name         = "balls",
        kmodel_candidates = [
            "/sdcard/models/balls/yolo11n_balls_320.kmodel",
        ],
        labels       = BALL_LABELS,
        input_size   = INPUT_SIZE,
        yolo_version = "v11",
        conf_thresh  = 0.6,          # 同 9340 demo
        nms_thresh   = 0.45,
        renderer     = _ball_renderer,
    )
