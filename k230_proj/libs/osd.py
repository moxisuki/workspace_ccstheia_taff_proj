# OSD — CH2 / cls_video 同款 image.Image + show_image
import image
from media.display import Display


# HB 色码
HB_GREEN  = (50, 220, 50)
HB_YELLOW = (255, 200, 50)
HB_RED    = (255, 80, 80)

class OSD:
    def __init__(self, width, height):
        self.img = image.Image(width, height, image.ARGB8888)
        self.width  = width
        self.height = height
        self.img.clear()

    def clear(self):
        self.img.clear()

    def show(self):
        Display.show_image(self.img, 0, 0, Display.LAYER_OSD3)

    @property
    def buffer(self):
        """底层 image.Image, 给 renderer 直接画."""
        return self.img

    def status_bar(self, model_name, fps, hb_count, hb_age_ms):
        """画顶部状态栏: model / fps / HB + 链接指示."""
        # HB 颜色 + 文字标签
        if hb_age_ms < 800:
            hb_color = HB_GREEN
            hb_label = "OK"
        elif hb_age_ms < 2000:
            hb_color = HB_YELLOW
            hb_label = "LAG"
        else:
            hb_color = HB_RED
            hb_label = "LOST"

        # 文字 (左侧) + 彩色 HB 标签 (右侧)
        left = "{}  {}fps  HB {}".format(model_name, int(fps), hb_count)
        self.img.draw_string_advanced(4, 4, 26, left, color=(200, 200, 200))
        self.img.draw_string_advanced(self.width - 64, 4, 26, hb_label, color=hb_color)

        # 分割线
        self.img.draw_line(4, 32, self.width - 4, 32, color=(60, 60, 60))