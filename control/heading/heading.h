#pragma once

namespace control::heading {

void init();
void set_target(float target_yaw);
float step(float current_yaw, float yaw_rate, float dt);
float last_error();

}