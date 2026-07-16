#pragma once
#include <stdint.h>

namespace common {

// 固定周期放行的门控
class RateGate {
public:
    explicit RateGate(uint32_t period_ms) : period_(period_ms) {}

    float tick(uint32_t now_ms) {
        if (now_ms - last_ < period_) return 0;
        float dt = (now_ms - last_) / 1000.0f;
        last_ = now_ms;
        return dt;
    }

    void reset(uint32_t now_ms = 0) { last_ = now_ms; }
    uint32_t period() const { return period_; }

private:
    uint32_t period_;
    uint32_t last_ = 0;
};

}