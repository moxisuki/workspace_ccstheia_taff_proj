#include "task/debug_task.h"

#include "app/global_mode.h"
#include "common/pid_params.h"
#include "control/drive/drive.h"
#include "control/heading/heading.h"
#include "drivers/led3/led3.h"
#include "drivers/systick/systick.h"
#include "drivers/uart/uart.h"
#include "drivers/uart/uart_ringbuf.h"
#include "ti_msp_dl_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace task::debug {
namespace {

drivers::uart::RingBuf g_cmd_rx;
char g_cmd_line[96];
uint8_t g_cmd_len = 0;
bool g_param_flash_active = false;
uint32_t g_param_flash_until_ms = 0;
bool g_white_on = false;
bool g_red_on = false;
bool g_blue_on = false;

int32_t scale10(float v) {
    return static_cast<int32_t>(v * 10.0f + (v >= 0.0f ? 0.5f : -0.5f));
}

void send_text(const char* s) {
    drivers::uart::write_async(drivers::uart::Id::Debug, s, strlen(s));
}

void set_led(drivers::led3::Color color, bool on, bool& cached) {
    if (cached == on) return;
    drivers::led3::set(color, on);
    cached = on;
}

bool timeout_reached(uint32_t now, uint32_t until) {
    return (now - until) < 0x80000000UL;
}

void flash_param_changed() {
    g_param_flash_active = true;
    g_param_flash_until_ms = drivers::systick::now_ms() + 180U;
    set_led(drivers::led3::BLUE, true, g_blue_on);
}

void service_led() {
    uint32_t now = drivers::systick::now_ms();

    set_led(drivers::led3::WHITE, true, g_white_on);
    set_led(
        drivers::led3::RED,
        app::global_mode::mode() == app::global_mode::Mode::Stop,
        g_red_on);

    bool blue_on = g_param_flash_active && !timeout_reached(now, g_param_flash_until_ms);
    set_led(drivers::led3::BLUE, blue_on, g_blue_on);
    if (g_param_flash_active && !blue_on) {
        g_param_flash_active = false;
    }
}

bool starts_with(const char* s, const char* prefix) {
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

bool find_float(const char* s, const char* key, float& out) {
    const char* p = strstr(s, key);
    if (!p) return false;
    p += strlen(key);
    char* end = nullptr;
    float v = strtof(p, &end);
    if (end == p) return false;
    out = v;
    return true;
}

bool in_range(float v, float lo, float hi) {
    return v == v && v >= lo && v <= hi;
}

bool valid_pid(float p, float i, float d) {
    return in_range(p, 0.0f, 100.0f)
        && in_range(i, 0.0f, 20.0f)
        && in_range(d, 0.0f, 20.0f);
}

void send_status() {
    float lp, li, ld, hp, hi, hd;
    control::drive::get_line_pid(lp, li, ld);
    control::heading::get_pid(hp, hi, hd);

    char resp[160];
    snprintf(resp, sizeof(resp),
        "STATUS app=%s drv=%s LINE=%.3f,%.3f,%.3f HEAD=%.3f,%.3f,%.3f\r\n",
        app::global_mode::to_string(app::global_mode::mode()),
        control::drive::to_string(control::drive::mode()),
        (double)lp, (double)li, (double)ld,
        (double)hp, (double)hi, (double)hd);
    send_text(resp);
}

void handle_set(const char* cmd) {
    float p, i, d;
    if (!find_float(cmd, "P:", p) || !find_float(cmd, "I:", i) || !find_float(cmd, "D:", d)) {
        send_text("ERR SET needs P: I: D:\r\n");
        return;
    }
    if (!valid_pid(p, i, d)) {
        send_text("ERR SET range\r\n");
        return;
    }

    if (strstr(cmd, "LINE")) {
        control::drive::set_line_pid(p, i, d);
        send_text("OK SET LINE\r\n");
        flash_param_changed();
    } else if (strstr(cmd, "HEAD") || strstr(cmd, "HEADING")) {
        control::heading::set_pid(p, i, d);
        send_text("OK SET HEAD\r\n");
        flash_param_changed();
    } else {
        send_text("ERR SET group LINE|HEAD\r\n");
    }
}

void handle_mode(const char* cmd) {
    auto now = drivers::systick::now_ms();
    bool ok = false;
    if (strstr(cmd, "STOP")) {
        ok = app::global_mode::change_mode(app::global_mode::Mode::Stop, now);
    } else if (strstr(cmd, "LINE")) {
        ok = app::global_mode::change_mode(app::global_mode::Mode::LineTrace, now);
    } else if (strstr(cmd, "STRAIGHT")) {
        ok = app::global_mode::change_mode(app::global_mode::Mode::Straight, now);
    } else if (strstr(cmd, "UTURN")) {
        ok = app::global_mode::change_mode(app::global_mode::Mode::UTurn, now);
    }
    if (ok) {
        send_text("OK MODE\r\n");
        service_led();
    } else {
        send_text("ERR MODE\r\n");
    }
}

void handle_command(char* cmd) {
    if (starts_with(cmd, "SET ")) {
        handle_set(cmd);
    } else if (starts_with(cmd, "MODE ")) {
        handle_mode(cmd);
    } else if (strcmp(cmd, "STOP") == 0) {
        app::global_mode::change_mode(app::global_mode::Mode::Stop, drivers::systick::now_ms());
        send_text("OK STOP\r\n");
        service_led();
    } else if (strcmp(cmd, "RESET") == 0) {
        control::drive::set_line_pid(
            common::pid_params::line_trace::kP,
            common::pid_params::line_trace::kI,
            common::pid_params::line_trace::kD);
        control::heading::set_pid(
            common::pid_params::heading::kP,
            common::pid_params::heading::kI,
            common::pid_params::heading::kD);
        send_text("OK RESET\r\n");
        flash_param_changed();
    } else if (strcmp(cmd, "STATUS") == 0) {
        send_status();
    } else {
        send_text("ERR CMD\r\n");
    }
}

}  // namespace

void init() {
    g_cmd_rx.flush();
    g_cmd_len = 0;
    g_param_flash_active = false;
    g_white_on = false;
    g_red_on = false;
    g_blue_on = false;
    service_led();
    NVIC_ClearPendingIRQ(DEBUG_UART_INST_INT_IRQN);
    NVIC_EnableIRQ(DEBUG_UART_INST_INT_IRQN);
}

void poll_commands() {
    service_led();
    uint8_t b;
    while (g_cmd_rx.pop(&b, 1) == 1) {
        if (b == '\r' || b == '\n') {
            if (g_cmd_len > 0) {
                g_cmd_line[g_cmd_len] = '\0';
                handle_command(g_cmd_line);
                g_cmd_len = 0;
            }
        } else if (g_cmd_len < sizeof(g_cmd_line) - 1) {
            g_cmd_line[g_cmd_len++] = static_cast<char>(b);
        } else {
            g_cmd_len = 0;
            send_text("ERR CMD TOO LONG\r\n");
        }
    }
}

void loop(const sensors::state::State& s) {
    auto ds = control::drive::status();

    char line[224];
    int n = snprintf(line, sizeof(line),
        "DBG,t=%ld,app=%s,drv=%s,base=%d,steer=%d,target=%d,turning=%d,"
        "yaw10=%ld,yr10=%ld,hd10=%ld,line=%d,lc10=%ld,le10=%ld,"
        "lth=%u,lmin=%u,lmax=%u,m1=%d,m2=%d,m3=%d,m4=%d,"
        "imu=%d,fresh=%d,vis=%d,vfresh=%d,kbad=%lu,kovf=%lu\r\n",
        (long)drivers::systick::now_ms(),
        app::global_mode::to_string(app::global_mode::mode()),
        control::drive::to_string(ds.mode),
        ds.last_base, ds.last_steer, ds.target_speed, ds.turning ? 1 : 0,
        (long)scale10(s.yaw), (long)scale10(s.yaw_rate),
        (long)scale10(control::heading::last_error()),
        ds.line.valid ? 1 : 0, (long)scale10(ds.line.center), (long)scale10(ds.line.error),
        ds.line.threshold, ds.line.min, ds.line.max,
        s.m1, s.m2, s.m3, s.m4,
        s.valid ? 1 : 0, s.fresh ? 1 : 0,
        s.vision.valid ? 1 : 0, s.vision.fresh ? 1 : 0,
        (unsigned long)s.vision.bad_xor, (unsigned long)s.vision.overflow);
    if (n > 0) {
        drivers::uart::write_async(drivers::uart::Id::Debug, line, (size_t)n);
    }
}

extern "C" void DEBUG_UART_INST_IRQHandler(void) {
    if (DL_UART_Main_getPendingInterrupt(DEBUG_UART_INST) == DL_UART_MAIN_IIDX_RX) {
        while (!DL_UART_Main_isRXFIFOEmpty(DEBUG_UART_INST)) {
            g_cmd_rx.push(DL_UART_Main_receiveData(DEBUG_UART_INST));
        }
    }
}

}  // namespace task::debug
