#pragma once
#include "sensors/state/state.h"
namespace task::motor_test {
void init();
void loop(const sensors::state::State& s, float dt);
void cmd_stop();
}