#pragma once
#include "sensors/state/state.h"

namespace task::heading {

void init();
void step(const sensors::state::State& s);
float last_turn();

}