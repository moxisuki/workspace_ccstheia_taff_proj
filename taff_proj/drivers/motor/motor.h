#pragma once
#include <stdint.h>

namespace drivers::motor {

// 通道映射:车前进方向,左=同侧,右=同侧(调这)
constexpr int kChL1 = 1, kChL2 = 2;  // 左侧 M1,M2
constexpr int kChR1 = 3, kChR2 = 4;  // 右侧 M3,M4
constexpr int kDirL = 1,  kDirR = 1; // 方向系数(1=正转前进, -1=反转前进)

// MG310 参数(调这)
constexpr int     kType      = 2;
constexpr int     kPhase     = 20;     // 减速比
constexpr int     kLine      = 13;     // 磁环线数
constexpr float   kWheelDia  = 48.00f; // 轮径 mm
constexpr int     kDeadzone  = 1600;
constexpr float   kSpeedP    = 1.5f;
constexpr float   kSpeedI    = 0.03f;
constexpr float   kSpeedD    = 0.1f;

void init();
void set(int16_t m1, int16_t m2, int16_t m3, int16_t m4);
void set_diff(int16_t base, int16_t turn);  // 差速:左=base+turn,右=base-turn

}