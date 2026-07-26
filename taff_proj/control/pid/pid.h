#pragma once

namespace control {

class Pid {
public:
    Pid(float kp, float ki, float kd, float i_max = 100.0f)
        : kp_(kp), ki_(ki), kd_(kd), i_max_(i_max) {}

    float step(float err, float dt);
    void reset();

private:
    float kp_, ki_, kd_;
    float i_max_;
    float integral_ = 0;
    float last_err_ = 0;
};

}