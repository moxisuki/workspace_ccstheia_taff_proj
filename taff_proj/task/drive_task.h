#pragma once
#include "sensors/state/state.h"

namespace task::drive {

void init();
void loop(const sensors::state::State& s, float dt);
void cmd_demo();   // 演示:直行→右转→直行→停车

}