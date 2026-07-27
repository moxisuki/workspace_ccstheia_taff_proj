#include "task/drive_task.h"

#include "app/global_mode.h"
#include "control/drive/drive.h"
#include "drivers/motor/motor.h"
#include "drivers/systick/systick.h"
#include "sensors/ccd/ccd.h"

namespace task::drive {

namespace {
bool g_synced = false;
app::global_mode::Mode g_last_mode = app::global_mode::Mode::Stop;

void enter_mode(app::global_mode::Mode mode) {
    switch (mode) {
    case app::global_mode::Mode::Stop:
        control::drive::enter_stop();
        break;
    case app::global_mode::Mode::LineTrace:
        control::drive::enter_line_trace();
        break;
    case app::global_mode::Mode::Straight:
        control::drive::enter_straight();
        break;
    case app::global_mode::Mode::UTurn:
        control::drive::enter_uturn(180.0f);
        break;
    }

    g_last_mode = mode;
    g_synced = true;
}

const uint8_t* ccd_pixels_for(app::global_mode::Mode mode) {
    if (mode == app::global_mode::Mode::LineTrace) {
        return sensors::ccd::scan();
    }
    return sensors::ccd::data();
}

}  // namespace

void init() {
    g_synced = false;
    enter_mode(app::global_mode::mode());
}

void loop(const sensors::state::State& s, float dt) {
    uint32_t now = drivers::systick::now_ms();
    app::global_mode::Mode app_mode = app::global_mode::mode();

    if (!g_synced || app_mode != g_last_mode) {
        enter_mode(app_mode);
    }

    const uint8_t* pixels = ccd_pixels_for(app_mode);
    control::drive::Output out = control::drive::step(s, pixels, dt);
    drivers::motor::set_ackermann(out.base, out.steer);

    if (app_mode == app::global_mode::Mode::UTurn && control::drive::uturn_done()) {
        app::global_mode::change_mode(app::global_mode::Mode::LineTrace, now);
    }
}

void cmd_demo() {
    app::global_mode::change_mode(
        app::global_mode::Mode::LineTrace,
        drivers::systick::now_ms());
}

}  // namespace task::drive
