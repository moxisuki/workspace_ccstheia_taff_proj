// 行车演示: 直行 2s → 弧线右转 90° → 直行 2s → 停车
// 调用 task::drive::cmd_demo() 启动

#include "task/drive_task.h"

#include "control/drive/drive.h"
#include "drivers/motor/motor.h"
#include "drivers/systick/systick.h"

namespace task::drive {

namespace {
enum { D_IDLE, D_GO, D_WAIT_TURN, D_TURN_DONE, D_STOP } g_step;
uint32_t g_t0;
}  // namespace

void init() {
    drivers::motor::init();
    control::drive::init();
}

void cmd_demo() { g_step = D_GO; }

void loop(const sensors::state::State& s, float dt) {
    uint32_t now = drivers::systick::now_ms();
    switch (g_step) {
    case D_GO:
        control::drive::cmd_go(300);
        g_t0   = now;
        g_step = D_WAIT_TURN;
        break;
    case D_WAIT_TURN:
        if (now - g_t0 > 2000) {
            control::drive::cmd_turn(90, 150);
            g_step = D_TURN_DONE;
        }
        break;
    case D_TURN_DONE:
        if (!control::drive::turning()) {
            control::drive::cmd_go(300);
            g_t0   = now;
            g_step = D_STOP;
        }
        break;
    case D_STOP:
        if (now - g_t0 > 2000) {
            control::drive::cmd_stop();
            g_step = D_IDLE;
        }
        break;
    default: break;
    }
    auto out = control::drive::step(s, dt);
    drivers::motor::set_diff(out.base, out.turn);
}

}  // namespace task::drive