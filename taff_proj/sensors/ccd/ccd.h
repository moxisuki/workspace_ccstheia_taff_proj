// 线性 CCD (TSL1401CL, 128 像素) 驱动
// 硬件: AO->PB18(ADC1_A5), SI->PB19, CLK->PB20
// WHY: MCU 直接用 SI+CLK 时序驱动 CCD, 每个 CLK 低电平期间用 ADC 采一个像素
#pragma once
#include <stdint.h>

namespace sensors::ccd {

constexpr int kPixels = 128;  // TSL1401 像素数

// 曝光(积分)时间调节: 值越大越亮; 环境暗调大, 亮调小
// 实际作用于每像素 CLK 低电平的停留时间(空循环计数)
void set_exposure(uint32_t units);

void init();

// 触发一次完整扫描, 返回指向 128 字节像素亮度(0..255)的指针
// 阻塞执行, 耗时约 = 128 x (曝光 + ADC 转换)
const uint8_t* scan();

// 返回最近一次扫描结果(不重新采集)
const uint8_t* data();

}  // namespace sensors::ccd
