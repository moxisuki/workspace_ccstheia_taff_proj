// 三色 LED 驱动 (蓝/白/红)
// 硬件: 共阳 (公共 VCC=3.3V), 三个独立阴极接 GPIO
//   蓝 -> PB25, 白 -> PB26, 红 -> PB27
// WHY 低=亮: 共阳时电流 VCC -> LED -> GPIO 灌入; GPIO 拉低才有压差点亮,
//            GPIO 拉高(=3.3V)两端等电位 -> 熄灭。故本驱动内部为"低电平点亮"。
// 注意: 每路需串限流电阻; 逻辑上层无需关心极性, 传 on/off 即可。
#pragma once
#include <stdint.h>

namespace drivers::led3 {

enum Color : uint8_t { BLUE = 0, WHITE = 1, RED = 2 };

void init();                        // SysConfig 已设为输出且初始灭, 这里再确保全灭

void set(Color c, bool on);         // 单色 开/关
void blue(bool on);                 // 便捷封装
void white(bool on);
void red(bool on);

void all(bool on);                  // 三色同时 开/关
void off();                         // 全灭
void toggle(Color c);               // 翻转某一路

}  // namespace drivers::led3
