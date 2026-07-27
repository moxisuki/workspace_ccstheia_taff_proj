#pragma once
#include "sensors/state/state.h"

namespace task::drive {

void init();
void loop(const sensors::state::State& s, float dt);
void cmd_demo();   // 切回默认循迹模式

}
