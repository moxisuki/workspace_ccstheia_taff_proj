#pragma once
#include "sensors/state/state.h"

namespace task::heading {

void init();
void loop(const sensors::state::State& s, float dt);
float last_turn();

}