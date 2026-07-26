# 视觉检测 + UART 上报 + 多模型切换
#
# 数据流:
#   img → 当前 model.run() → dets → proto.send_target() per box
#
# 使用:
#   det = Detector()
#   det.register(spec)       # spec = ModelSpec(...)
#   det.setup_yolo(proto)    # 只绑 proto
#   det.switch("digits")     # 加载 kmodel + config_preprocess
#   while True:
#       dets = det.tick_yolo(img)   # 推理 + 上报, 返回统一格式 dets

from libs.YOLO import YOLOv5, YOLOv8, YOLO11
import os
import gc


INPUT_SIZE = (320, 320)


def _yolo_class(version):
    return {"v5": YOLOv5, "v8": YOLOv8, "v11": YOLO11}[version]


def _pick_kmodel(candidates):
    for p in candidates:
        try:
            os.stat(p)
            return p
        except OSError:
            continue
    return None


class ModelSpec:
    def __init__(self, name, kmodel_candidates, labels,
                 input_size=INPUT_SIZE,
                 yolo_version="v8",
                 conf_thresh=0.5, nms_thresh=0.2,
                 renderer=None):
        self.name              = name
        self.kmodel_candidates = kmodel_candidates
        self.kmodel_path       = None
        self.labels            = labels
        self.input_size        = input_size
        self.yolo_version      = yolo_version
        self.conf_thresh       = conf_thresh
        self.nms_thresh        = nms_thresh
        self.renderer          = renderer
        self._yolo             = None


class Detector:
    def __init__(self, rgb888p_size=INPUT_SIZE, display_size=None):
        self.rgb888p_size = list(rgb888p_size)
        # display_size: YOLO 输出坐标映射到显示尺寸 (屏幕 640x480, 不是 AI 640x360)
        self.display_size = list(display_size) if display_size else list(rgb888p_size)
        self.models   = {}
        self.current  = None
        self.proto    = None

    def register(self, spec):
        self.models[spec.name] = spec

    def setup_yolo(self, proto):
        self.proto = proto

    def switch(self, name):
        if name not in self.models:
            raise KeyError("model '{}' not registered. have: {}".format(
                name, list(self.models)))
        spec = self.models[name]
        if (self.current is not None
                and self.current.name == name
                and spec._yolo is not None):
            return

        found = _pick_kmodel(spec.kmodel_candidates)
        if found is None:
            raise FileNotFoundError(
                "model '{}' kmodel not found. tried:\n".format(name) +
                "\n".join("  " + p for p in spec.kmodel_candidates) +
                "\ntrain at: https://ai.01studio.cc"
            )

        print("[detect] switch ->", name, "(", found, ")")
        spec.kmodel_path = found

        cls = _yolo_class(spec.yolo_version)
        spec._yolo = cls(
            task_type="detect",
            mode="video",
            kmodel_path=found,
            labels=spec.labels,
            rgb888p_size=self.rgb888p_size,
            model_input_size=spec.input_size,
            display_size=self.display_size,  # 映射到实际屏幕尺寸 (640x480)
            conf_thresh=spec.conf_thresh,
            nms_thresh=spec.nms_thresh,
            max_boxes_num=50,
        )
        spec._yolo.config_preprocess()
        gc.collect()
        self.current = spec

    def current_model(self):
        return self.current.name if self.current else None

    def current_model_labels(self):
        return self.current.labels if self.current else []

    def current_renderer(self):
        return self.current.renderer if self.current else None

    def _to_target(self, box, score, cls):
        x, y, w, h = box
        flag = 1 if score >= 0.7 else 0
        return (int(x + w // 2), int(y + h // 2), int(cls), flag)

    def tick_yolo(self, img):
        if self.current is None or self.current._yolo is None:
            return []

        try:
            input_np = img.to_numpy_ref()
        except Exception:
            return []

        res = self.current._yolo.run(input_np)
        boxes, classes, scores = [], [], []
        if res is not None and len(res) == 3:
            boxes, classes, scores = res[0], res[1], res[2]
        n = min(len(boxes), 10)

        dets = []
        for i in range(n):
            det = (boxes[i][0], boxes[i][1], boxes[i][2], boxes[i][3],
                   scores[i], classes[i])
            dets.append(det)
            cx, cy, type_id, flag = self._to_target(boxes[i], scores[i], classes[i])
            if self.proto is not None:
                self.proto.send_target(cx, cy, type_id, flag)

        return dets