#include "task/heading_task.h"

#include "control/heading/heading.h"

namespace task::heading {

float g_last_turn = 0;

void init() {}

void loop(const sensors::state::State& s, float dt) {
    if (s.valid && s.fresh) {
        g_last_turn = control::heading::step(s.yaw, s.yaw_rate, dt);
    }
}

float last_turn() { return g_last_turn; }

}  // namespace task::heading