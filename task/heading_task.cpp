#include "task/heading_task.h"

#include "common/rate.h"
#include "drivers/systick/systick.h"
#include "control/heading/heading.h"

namespace task::heading {

namespace {
common::RateGate ctl_gate(10);  // 100 Hz
float            g_last_turn = 0;
}

void init() {
    ctl_gate.reset(drivers::systick::now_ms());
}

void step(const sensors::state::State& s) {
    if (float dt = ctl_gate.tick(drivers::systick::now_ms()); dt > 0) {
        if (s.valid && s.fresh) {
            g_last_turn = control::heading::step(s.yaw, s.yaw_rate, dt);
        }
    }
}

float last_turn() { return g_last_turn; }

}  // namespace task::heading