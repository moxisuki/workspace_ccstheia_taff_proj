#include "drivers/motor/motor.h"
#include "drivers/uart/uart.h"
#include "drivers/systick/systick.h"
#include <string.h>

namespace drivers::motor {

static void send_str(const char* s) {
    drivers::uart::write(drivers::uart::Id::Drive, s, strlen(s));
}

static void send_int(int16_t v) {
    char buf[7];
    int i = 0;
    bool neg = v < 0;
    if (neg) v = -v;
    do { buf[i++] = '0' + (v % 10); v /= 10; } while (v);
    if (neg) buf[i++] = '-';
    while (i--) {
        char c = buf[i];
        drivers::uart::write(drivers::uart::Id::Drive, &c, 1);
    }
}

static void send_cmd(const char* cmd) {
    send_str(cmd);
    drivers::systick::delay_ms(50);
}

static void send_int_cmd(const char* prefix, int v) {
    send_str(prefix);
    send_int(v);
    send_str("#");
    send_cmd("");
}

static void send_pid_cmd() {
    send_str("$MPID:");
    send_int(kSpeedP); send_str(",");
    send_int(kSpeedI); send_str(",");
    send_int(kSpeedD); send_str("#");
    send_cmd("");
}

static void send_float_cmd(const char* prefix, float v) {
    int a = (int)v, d = (int)((v - a) * 100 + 0.5f);
    send_str(prefix);
    send_int(a);
    send_str(".");
    if (d < 10) send_str("0");
    send_int(d);
    send_str("#");
    send_cmd("");
}

void init() {
    send_int_cmd("$mtype:",     kType);
    send_int_cmd("$mphase:",    kPhase);
    send_int_cmd("$mline:",     kLine);
    send_float_cmd("$wdiameter:", kWheelDia);
    send_int_cmd("$deadzone:",  kDeadzone);
    send_pid_cmd();
    send_cmd("$upload:1,0,0#");  // 开启速度上报
}

void set(int16_t m1, int16_t m2, int16_t m3, int16_t m4) {
    send_str("$spd:");
    send_int(m1); send_str(",");
    send_int(m2); send_str(",");
    send_int(m3); send_str(",");
    send_int(m4);
    send_str("#\r\n");
}

void set_diff(int16_t base, int16_t turn) {
    int16_t l = (base + turn) * kDirL;
    int16_t r = (base - turn) * kDirR;
    int16_t spd[4] = {};
    spd[kChL1 - 1] = l; spd[kChL2 - 1] = l;
    spd[kChR1 - 1] = r; spd[kChR2 - 1] = r;
    set(spd[0], spd[1], spd[2], spd[3]);
}

}