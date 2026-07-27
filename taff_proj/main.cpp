#include "ti_msp_dl_config.h"
#include "app/global_mode.h"
#include "control/drive/drive.h"
#include "drivers/led3/led3.h"
#include "drivers/motor/motor.h"
#include "drivers/uart/uart.h"
#include "drivers/systick/systick.h"
#include "sensors/ccd/ccd.h"
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

void feed_watchdog() {
#ifdef WWDT0_INST
    DL_WWDT_restart(WWDT0_INST);
#endif
}
}

int main(void) {
    SYSCFG_DL_init();
    drivers::systick::init();
    __enable_irq();
    drivers::uart::init();
    drivers::led3::init();
    app::global_mode::init();

    common::uart_print("[boot] taff_proj (drive)\r\n");
    common::uart_print("[boot] led3 ok\r\n");
    feed_watchdog();

    sensors::imu::init();
    common::uart_print("[boot] imu ok\r\n");
    sensors::state::init();
    common::uart_print("[boot] state ok\r\n");
    feed_watchdog();
    sensors::k230::init();
    common::uart_print("[boot] k230 ok\r\n");
    sensors::ccd::init();
    common::uart_print("[boot] ccd ok\r\n");
    feed_watchdog();

    drivers::motor::init();
    common::uart_print("[boot] motor ok\r\n");
    control::drive::init();
    control::drive::set_line_polarity(control::drive::LinePolarity::Dark);
    common::uart_print("[boot] drive ctrl ok\r\n");
    feed_watchdog();

    task::drive::init();
    task::debug::init();

    sensors::state::State state = sensors::state::read();
    const uint32_t scheduler_start_ms = drivers::systick::now_ms();
    gate_100hz.reset(scheduler_start_ms);
    gate_20hz.reset(scheduler_start_ms);

    while (1) {
        const uint32_t now = drivers::systick::now_ms();
        drivers::uart::service_tx();
        task::debug::poll_commands();

        if (float dt = gate_100hz.tick(now); dt > 0) {
            state = sensors::state::read();
            task::drive::loop(state, dt);
        }

        if (gate_20hz.tick(now) > 0) {
            task::debug::loop(state);
        }

        feed_watchdog();
    }
}
