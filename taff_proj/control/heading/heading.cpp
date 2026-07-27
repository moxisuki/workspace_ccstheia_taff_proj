#include "control/heading/heading.h"

#include "common/math.h"
#include "common/pid_params.h"
#include "control/pid/pid.h"

namespace control::heading {

namespace {
namespace pid_cfg = common::pid_params::heading;

Pid   g_pid(pid_cfg::kP, pid_cfg::kI, pid_cfg::kD, pid_cfg::kIMax);
float g_target = 0;
float g_last_err = 0;
}

void set_target(float target_yaw) {
    g_target = target_yaw;
}

void reset() {
    g_pid.reset();
    g_last_err = 0;
}

float step(float current_yaw, float yaw_rate, float dt) {
    g_last_err = common::wrap_180(g_target - current_yaw);
    return g_pid.step(g_last_err, dt) - pid_cfg::kYawRateFf * yaw_rate;
}

float last_error() { return g_last_err; }

void set_pid(float kp, float ki, float kd) {
    g_pid.set_gains(kp, ki, kd);
}

void get_pid(float& kp, float& ki, float& kd) {
    kp = g_pid.kp();
    ki = g_pid.ki();
    kd = g_pid.kd();
}

}
