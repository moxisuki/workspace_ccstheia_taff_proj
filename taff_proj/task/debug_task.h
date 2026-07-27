#pragma once
#include "sensors/state/state.h"

namespace task::debug {

void init();
void poll_commands();
void loop(const sensors::state::State& s);

}
