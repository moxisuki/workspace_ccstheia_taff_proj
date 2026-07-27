#pragma once

#include <stdint.h>

namespace common::vehicle_params {

// 阿克曼近似混控车体参数。
namespace ackermann {
constexpr float kTrackWidthMm = 150.0f;  // 左右轮中心距
constexpr float kWheelBaseMm = 200.0f;   // 前后轴中心距
constexpr int16_t kSpeedLimit = 1000;    // 下发电驱前的四轮速度限幅
}  // namespace ackermann

// 电机测速参数, 需和电驱/编码器配置一致。
namespace motor_board {
constexpr int kPulsePerRev = 330;  // kLine(11) * kPhase(30), 与电驱配置保持一致
}  // namespace motor_board

}  // namespace common::vehicle_params
