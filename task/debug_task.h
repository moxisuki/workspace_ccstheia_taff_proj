#pragma once
#include "sensors/state/state.h"

namespace task::debug {

void init();
void print(const sensors::state::State& s);

}