#include "app/app.h"

#include <stdio.h>

#include "ti_msp_dl_config.h"
#include "common/rate.h"
#include "drivers/uart/uart.h"
#include "drivers/systick/systick.h"
#include "sensors/imu/imu.h"
#include "sensors/state/state.h"
#include "control/heading/heading.h"

namespace app {

namespace {

common::RateGate ctl_gate(10);  // 100 Hz
common::RateGate prt_gate(50);  // 20 Hz
float            last_turn = 0;

}

void init() {
    drivers::systick::init();
    drivers::uart::init();
    sensors::imu::init();
    sensors::state::init();
    control::heading::init();

    uint32_t now = drivers::systick::now_ms();
    ctl_gate.reset(now);
    prt_gate.reset(now);

    printf("\r\n[boot] taff_proj v2 (debug)\r\n");
    printf("MCLK=%u Hz\r\n", (unsigned)DEBUG_UART_INST_FREQUENCY);
}

void tick() {
    uint32_t now = drivers::systick::now_ms();
    auto     s   = sensors::state::read();

    if (float dt = ctl_gate.tick(now); dt > 0) {
        if (s.valid && s.fresh) {
            last_turn = control::heading::step(s.yaw, s.yaw_rate, dt);
        }
    }

    if (prt_gate.tick(now) > 0) {
        printf("y=%6.1f yr=%5.1f gz_raw=%5.1f err=%5.1f turn=%6.1f t=%u\r\n",
               s.yaw, s.yaw_rate, s.gx_raw,
               control::heading::last_error(), last_turn, now);
    }
}

}  // namespace app