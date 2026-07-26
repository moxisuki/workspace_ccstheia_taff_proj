# K230 vision + digits detect — hardware-tested configuration

import os, gc, utime, image
from media.sensor import *
from media.display import *
from media.media import *

from libs.uart_proto import UARTProto
from libs.detect import Detector
from libs.osd import OSD

# ---------- hardware-tested config ----------
DISPLAY_WIDTH  = ALIGN_UP(640, 16)
DISPLAY_HEIGHT = 480
AI_WIDTH       = ALIGN_UP(640, 16)
AI_HEIGHT      = 360
LOG_TAG = "[k230]"

def log(msg):
    print("{} {:>8} ms | {}".format(LOG_TAG, utime.ticks_ms(), msg))

# ---------- digits model ----------
def digits_model():
    from libs.detect import ModelSpec
    from libs.models.digits import DIGIT_LABELS, _digit_renderer
    return ModelSpec(
        name="digits",
        kmodel_candidates=[
            "/sdcard/models/yolo11n_digits_320.kmodel",
            "/sdcard/models/yolo11n_det_320.kmodel",
            "/sdcard/yolo11n_det_320.kmodel",
        ],
        labels=DIGIT_LABELS, yolo_version="v11",
        conf_thresh=0.6, nms_thresh=0.45,
        renderer=_digit_renderer,
    )

# ---------- main ----------
def main():
    log("boot")

    # --- UART ---
    proto = UARTProto()
    proto.init()

    # --- sensor (hardware-tested) ---
    sensor = Sensor(width=1280, height=960)
    sensor.reset()
    sensor.set_hmirror(False)
    sensor.set_vflip(False)
    # CHN0: 640x480 YUV420 → bind_layer 自动推显示
    sensor.set_framesize(width=DISPLAY_WIDTH, height=DISPLAY_HEIGHT)
    sensor.set_pixformat(PIXEL_FORMAT_YUV_SEMIPLANAR_420)
    # CHN2: 1280x720 RGB888 → AI (必须 RGB888, YOLO to_numpy_ref 只认这个)
    sensor.set_framesize(width=AI_WIDTH, height=AI_HEIGHT, chn=CAM_CHN_ID_2)
    sensor.set_pixformat(PIXEL_FORMAT_RGB_888_PLANAR, chn=CAM_CHN_ID_2)

    # --- display ---
    Display.init(Display.ST7701, width=DISPLAY_WIDTH, height=DISPLAY_HEIGHT, to_ide=True)
    sensor_bind_info = sensor.bind_info(x=0, y=0, chn=CAM_CHN_ID_0)
    Display.bind_layer(**sensor_bind_info, layer=Display.LAYER_VIDEO1)

    # --- OSD ---
    osd = OSD(DISPLAY_WIDTH, DISPLAY_HEIGHT)

    # --- detector ---
    detector = Detector(rgb888p_size=(AI_WIDTH, AI_HEIGHT),
                        display_size=(DISPLAY_WIDTH, DISPLAY_HEIGHT))
    detector.register(digits_model())
    detector.setup_yolo(proto)
    detector.switch("digits")
    log("model: digits")

    # --- MediaManager + sensor.run ---
    MediaManager.init()
    sensor.run()
    log("capture loop start")

    t0 = utime.ticks_ms()
    last_hb = t0
    last_osd = t0
    frames = 0
    fps_frames = 0
    fps_t0 = t0
    fps_val = 0
    last_ack_ms = t0
    prev_ack = 0

    while True:
        os.exitpoint()

        img = sensor.snapshot(chn=CAM_CHN_ID_2)
        if img == -1:
            gc.collect()
            continue
        frames += 1

        dets = detector.tick_yolo(img)
        n = len(dets)
        now = utime.ticks_ms()

        # poll every frame (不丢失 MSPM0 ACK)
        proto.poll()
        cur_ack = proto.acked_count
        if cur_ack != prev_ack:
            last_ack_ms = now
            prev_ack = cur_ack

        # heartbeat send at 500ms
        if utime.ticks_diff(now, last_osd) >= 500:
            proto.send_heartbeat()
            last_osd = now

        # OSD: status bar + detection overlay
        osd.clear()

        # ── status bar (top, 26px font) ──
        # instant FPS (1s sliding window)
        fps_frames += 1
        fps_elapsed = utime.ticks_diff(now, fps_t0)
        if fps_elapsed >= 1000:
            fps_val = fps_frames * 1000 / fps_elapsed
            fps_frames = 0
            fps_t0 = now
        osd.status_bar(
            detector.current_model() or "?",
            fps_val,
            proto.acked_count,
            utime.ticks_diff(now, last_ack_ms))

        # ── detection ──
        if n > 0:
            renderer = detector.current_renderer()
            if renderer is not None:
                renderer(osd.buffer, {
                    "dets": dets,
                    "labels": detector.current_model_labels(),
                    "width": DISPLAY_WIDTH,
                })

        osd.show()

        if n > 0 and utime.ticks_diff(now, last_hb) >= 1000:
            el = utime.ticks_diff(now, t0)
            fps = frames * 1000 / el if el else 0
            log("detect targets={} fps={:.1f}".format(n, fps))
            last_hb = now

        gc.collect()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("KeyboardInterrupt")
    except Exception as e:
        print("main error:", e)