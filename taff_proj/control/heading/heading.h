#pragma once

namespace control::heading {

void set_target(float target_yaw);
void reset();
float step(float current_yaw, float yaw_rate, float dt);
float last_error();
void set_pid(float kp, float ki, float kd);
void get_pid(float& kp, float& ki, float& kd);

}
