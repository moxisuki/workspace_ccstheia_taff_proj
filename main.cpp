#include "ti_msp_dl_config.h"

#include "drivers/uart/uart.h"
#include "drivers/systick/systick.h"
#include "sensors/imu/imu.h"
#include "sensors/state/state.h"
#include "common/rate.h"
#include "task/drive_task.h"
#include "task/debug_task.h"

namespace {

common::RateGate gate_100hz(10);  // 100 Hz 任务组
common::RateGate gate_20hz(50);   // 20 Hz 任务组

}

int main(void) {
    SYSCFG_DL_init();
    drivers::systick::init();
    drivers::uart::init();
    sensors::imu::init();
    sensors::state::init();
    task::drive::init();
    task::debug::init();

    uint32_t now = drivers::systick::now_ms();
    gate_100hz.reset(now);
    gate_20hz.reset(now);

    while (1) {
        now = drivers::systick::now_ms();
        auto s = sensors::state::read();

        if (float dt = gate_100hz.tick(now); dt > 0) {
            task::drive::loop(s, dt);
        }

        if (gate_20hz.tick(now) > 0) {
            task::debug::loop(s);
        }
    }
}