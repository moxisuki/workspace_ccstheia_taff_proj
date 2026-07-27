#pragma once

namespace common::pid_params {

// MPU 航向 PID: Straight/UTurn 使用, 误差单位 deg, 输出 steer。
// P 增强响应, I 修正长期偏差, D 抑制过冲。
namespace heading {
constexpr float kP = 2.0f;
constexpr float kI = 0.05f;
constexpr float kD = 0.05f;
constexpr float kIMax = 30.0f;
constexpr float kYawRateFf = 0.05f;
}  // namespace heading

// CCD 循迹 PID: 输入为像素偏差, 输出 steer。
// P 管贴线强度, D 管摆动, I 通常先保持 0。
namespace line_trace {
constexpr float kP = 5.0f;
constexpr float kI = 0.0f;
constexpr float kD = 0.12f;
constexpr float kIMax = 20.0f;
constexpr float kYawRateDamp = 0.45f;
}  // namespace line_trace

// 电驱板内置速度 PID, MCU 只下发参数。
namespace motor_speed {
constexpr float kP = 1.5f;
constexpr float kI = 0.03f;
constexpr float kD = 0.1f;
}  // namespace motor_speed

}  // namespace common::pid_params
