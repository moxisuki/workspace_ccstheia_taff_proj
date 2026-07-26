// 电机速度监视 (不上驱动) — 5Hz 打印 m1~m4
#include "task/motor_test.h"
#include "common/print.h"
#include "common/rate.h"
#include "drivers/motor/motor.h"
#include "drivers/systick/systick.h"
#include "sensors/motor_speed/motor_speed.h"
namespace task::motor_test {
namespace {
common::RateGate g_log(200);
void log_fb(int16_t m1, int16_t m2, int16_t m3, int16_t m4) {
    common::uart_print("[mot_mon] m1="); common::uart_print_int(m1);
    common::uart_print(" m2="); common::uart_print_int(m2);
    common::uart_print(" m3="); common::uart_print_int(m3);
    common::uart_print(" m4="); common::uart_print_int(m4);
    common::uart_println();
}}
void init() { drivers::motor::init(); common::uart_println(); common::uart_print("[mot_mon] monitor mode, no drive, 5Hz log"); common::uart_println(); }
void cmd_stop() { common::uart_println(); common::uart_print("[mot_mon] (no drive, nothing to stop)"); common::uart_println(); }
void loop(const sensors::state::State& s, float dt) {
    (void)dt;
    if (g_log.tick(drivers::systick::now_ms()) > 0) log_fb(s.m1, s.m2, s.m3, s.m4);
}
}