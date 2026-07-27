#pragma once

#include <stdint.h>

namespace common::drive_params {

// 基础速度, 单位沿用电驱速度命令。
namespace speed {
constexpr int16_t kStraight = 300;   // Straight 模式巡航速度
constexpr int16_t kLineTrace = 260;  // LineTrace 模式循迹速度
constexpr int16_t kUTurn = 140;      // UTurn 模式掉头速度, 通常应低于循迹速度
}  // namespace speed

// steer 限幅: >0 右转, <0 左转。
namespace steer {
constexpr int16_t kMax = 420;
}  // namespace steer

// CCD 最小反差, max-min 小于该值认为丢线。
namespace line {
constexpr uint8_t kMinContrast = 18;
}  // namespace line

// 掉头完成判定阈值。
namespace turn {
constexpr float kSettleErr = 4.0f;    // deg
constexpr float kSettleRate = 12.0f;  // deg/s
constexpr int kSettleCnt = 6;         // 100Hz ticks
}  // namespace turn

}  // namespace common::drive_params
