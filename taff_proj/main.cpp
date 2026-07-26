#include "ti_msp_dl_config.h"
#include "drivers/uart/uart.h"
#include "drivers/systick/systick.h"
#include "sensors/imu/imu.h"
#include "sensors/k230/k230.h"
#include "sensors/state/state.h"
#include "common/print.h"
#include "common/rate.h"
#include "task/drive_task.h"
#include "task/debug_task.h"

namespace {
common::RateGate gate_100hz(10);
common::RateGate gate_20hz(50);
}

int main(void) {
    SYSCFG_DL_init();
    drivers::systick::init();
    drivers::uart::init();
    sensors::imu::init();
    sensors::state::init();
    sensors::k230::init();
    task::drive::init();
    task::debug::init();

    uint32_t now = drivers::systick::now_ms();
    gate_100hz.reset(now);
    gate_20hz.reset(now);

    while (1) {
        now = drivers::systick::now_ms();
        auto s = sensors::state::read();

        if (float dt = gate_100hz.tick(now); dt > 0)
            task::drive::loop(s, dt);

        if (gate_20hz.tick(now) > 0)
            task::debug::loop(s);
    }
}