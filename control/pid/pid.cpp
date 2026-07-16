#include "control/pid/pid.h"
#include "common/math_util.h"

namespace control {

float Pid::step(float err, float dt) {
    if (dt <= 0) return 0;
    integral_ += err * dt;
    integral_ = common::clamp(integral_, -i_max_, i_max_);
    float deriv = (err - last_err_) / dt;
    last_err_ = err;
    return kp_ * err + ki_ * integral_ + kd_ * deriv;
}

void Pid::reset() {
    integral_ = 0;
    last_err_ = 0;
}

}