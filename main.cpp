#include "ti_msp_dl_config.h"

#include "drivers/uart/uart.h"
#include "drivers/systick/systick.h"
#include "sensors/imu/imu.h"
#include "sensors/state/state.h"
#include "control/heading/heading.h"
#include "task/heading_task.h"
#include "task/debug_task.h"

int main(void) {
    SYSCFG_DL_init();
    drivers::systick::init();
    drivers::uart::init();
    sensors::imu::init();
    sensors::state::init();
    control::heading::init();
    task::heading::init();
    task::debug::init();

    while (1) {
        auto s = sensors::state::read();
        task::heading::step(s);
        task::debug::print(s);
    }
}