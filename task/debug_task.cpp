#include "task/debug_task.h"

#include <stdio.h>

#include "ti_msp_dl_config.h"
#include "common/rate.h"
#include "drivers/systick/systick.h"
#include "control/heading/heading.h"
#include "task/heading_task.h"

namespace task::debug {

namespace {
common::RateGate prt_gate(50);  // 20 Hz
}

void init() {
    prt_gate.reset(drivers::systick::now_ms());
    printf("\r\n[boot] taff_proj v2 (debug)\r\n");
    printf("MCLK=%u Hz\r\n", (unsigned)DEBUG_UART_INST_FREQUENCY);
}

void print(const sensors::state::State& s) {
    if (prt_gate.tick(drivers::systick::now_ms()) > 0) {
        printf("y=%6.1f yr=%5.1f gz_raw=%5.1f err=%5.1f turn=%6.1f t=%u\r\n",
               s.yaw, s.yaw_rate, s.gx_raw,
               control::heading::last_error(),
               task::heading::last_turn(),
               drivers::systick::now_ms());
    }
}

}  // namespace task::debug