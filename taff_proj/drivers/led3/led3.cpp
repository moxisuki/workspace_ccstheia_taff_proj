// 三色 LED 驱动实现 (共阳, 低电平点亮)
#include "drivers/led3/led3.h"
#include "ti_msp_dl_config.h"

namespace drivers::led3 {
namespace {

// 颜色 -> 对应引脚位 (均在 LED3_PORT = GPIOB 上)
uint32_t pin_of(Color c) {
    switch (c) {
        case BLUE:  return LED3_BLUE_PIN;
        case WHITE: return LED3_WHITE_PIN;
        case RED:   return LED3_RED_PIN;
    }
    return 0;
}

}  // namespace

void init() {
    off();  // 上电全灭 (SysConfig 已初始化为输出+高电平, 这里再兜底)
}

void set(Color c, bool on) {
    uint32_t p = pin_of(c);
    if (p == 0) return;
    if (on) DL_GPIO_clearPins(LED3_PORT, p);  // 低电平 = 点亮
    else    DL_GPIO_setPins(LED3_PORT, p);    // 高电平 = 熄灭
}

void blue(bool on)  { set(BLUE,  on); }
void white(bool on) { set(WHITE, on); }
void red(bool on)   { set(RED,   on); }

void all(bool on) {
    uint32_t mask = LED3_BLUE_PIN | LED3_WHITE_PIN | LED3_RED_PIN;
    if (on) DL_GPIO_clearPins(LED3_PORT, mask);
    else    DL_GPIO_setPins(LED3_PORT, mask);
}

void off() { all(false); }

void toggle(Color c) {
    uint32_t p = pin_of(c);
    if (p) DL_GPIO_togglePins(LED3_PORT, p);
}

}  // namespace drivers::led3
