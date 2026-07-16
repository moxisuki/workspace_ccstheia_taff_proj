#include "task/debug_task.h"

#include <stdio.h>

#include "drivers/systick/systick.h"
#include "control/heading/heading.h"
#include "task/heading_task.h"

namespace task::debug {

void init() {
    printf("\r\n[boot] taff_proj v2 (debug)\r\n");
}

void loop(const sensors::state::State& s) {
    printf("y=%6.1f yr=%5.1f gz_raw=%5.1f err=%5.1f turn=%6.1f t=%u\r\n",
           s.yaw, s.yaw_rate, s.gx_raw,
           control::heading::last_error(),
           task::heading::last_turn(),
           drivers::systick::now_ms());
}

}  // namespace task::debug