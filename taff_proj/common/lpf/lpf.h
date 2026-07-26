#pragma once

namespace common {

class Lpf {
public:
    explicit Lpf(float alpha = 0.1f) : alpha_(alpha) {}

    float step(float x) {
        y_ = alpha_ * x + (1.0f - alpha_) * y_;
        return y_;
    }
    void reset() { y_ = 0; }
    void set_alpha(float a) { alpha_ = a; }
    float value() const { return y_; }

private:
    float alpha_;
    float y_ = 0;
};

}