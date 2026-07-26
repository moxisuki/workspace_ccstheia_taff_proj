#pragma once

namespace control::heading {

// PID 参数(调这)
constexpr float kP  = 2.0f;
constexpr float kI  = 0.05f;
constexpr float kD  = 0.05f;
constexpr float kIMax = 30.0f;
constexpr float kFf = 0.05f;   // 前馈阻尼系数

void set_target(float target_yaw);
float step(float current_yaw, float yaw_rate, float dt);
float last_error();

}