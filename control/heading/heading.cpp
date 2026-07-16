#include "control/heading/heading.h"
#include "control/pid/pid.h"
#include "common/math_util.h"

namespace control::heading {

namespace {
Pid    g_pid(2.0f, 0.0f, 0.05f, 50.0f);  // PD: P=2.0, D=0.05
float  g_target = 0;
float  g_last_err = 0;
}

void init() {
    g_pid.reset();
    g_target = 0;
    g_last_err = 0;
}

void set_target(float target_yaw) {
    g_target = target_yaw;
}

float step(float current_yaw, float yaw_rate, float dt) {
    g_last_err = common::wrap_180(g_target - current_yaw);
    return g_pid.step(g_last_err, dt) - 0.05f * yaw_rate;
}

float last_error() { return g_last_err; }

}