#include "drivers/motor/motor.h"
#include "drivers/uart/uart.h"
#include "drivers/systick/systick.h"
#include "common/math.h"
#include <math.h>
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

static void send_float_value(float v) {
    int a = static_cast<int>(v);
    float frac = v - static_cast<float>(a);
    if (frac < 0.0f) frac = -frac;
    int d = static_cast<int>(frac * 100.0f + 0.5f);
    if (d >= 100) {
        a += (v >= 0.0f) ? 1 : -1;
        d = 0;
    }

    send_int(a);
    send_str(".");
    if (d < 10) send_str("0");
    send_int(static_cast<int16_t>(d));
}

static void send_pid_cmd() {
    send_str("$MPID:");
    send_float_value(kSpeedP); send_str(",");
    send_float_value(kSpeedI); send_str(",");
    send_float_value(kSpeedD); send_str("#");
    send_cmd("");
}

static void send_float_cmd(const char* prefix, float v) {
    send_str(prefix);
    send_float_value(v);
    send_str("#");
    send_cmd("");
}

static int16_t clamp_speed(float v) {
    return static_cast<int16_t>(common::clamp(v, -kSpeedLimit, kSpeedLimit));
}

static int dir_for_channel(int ch) {
    if (ch == kChL1) return kDirL1;
    if (ch == kChL2) return kDirL2;
    if (ch == kChR1) return kDirR1;
    if (ch == kChR2) return kDirR2;
    return 1;
}

static float absf(float v) {
    return v < 0.0f ? -v : v;
}

static int16_t signed_hypot(float forward, float lateral) {
    float mag = sqrtf(forward * forward + lateral * lateral);
    return static_cast<int16_t>((forward >= 0.0f ? 1.0f : -1.0f) * mag);
}

static WheelSpeeds limit_wheels(float lf, float lr, float rf, float rr) {
    float m = absf(lf);
    if (absf(lr) > m) m = absf(lr);
    if (absf(rf) > m) m = absf(rf);
    if (absf(rr) > m) m = absf(rr);

    if (m > kSpeedLimit) {
        float scale = static_cast<float>(kSpeedLimit) / m;
        lf *= scale; lr *= scale; rf *= scale; rr *= scale;
    }

    return {
        clamp_speed(lf),
        clamp_speed(lr),
        clamp_speed(rf),
        clamp_speed(rr),
    };
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

void set_wheels(const WheelSpeeds& wheels) {
    int16_t spd[4] = {};
    spd[kChL1 - 1] = clamp_speed(wheels.left_front)  * dir_for_channel(kChL1);
    spd[kChL2 - 1] = clamp_speed(wheels.left_rear)   * dir_for_channel(kChL2);
    spd[kChR1 - 1] = clamp_speed(wheels.right_front) * dir_for_channel(kChR1);
    spd[kChR2 - 1] = clamp_speed(wheels.right_rear)  * dir_for_channel(kChR2);
    set(spd[0], spd[1], spd[2], spd[3]);
}

WheelSpeeds mix_ackermann(int16_t base, int16_t steer) {
    float rear_l = static_cast<float>(base + steer);
    float rear_r = static_cast<float>(base - steer);

    float lateral = static_cast<float>(steer) * (kWheelBase / kTrackWidth);
    float front_l = static_cast<float>(signed_hypot(rear_l, lateral));
    float front_r = static_cast<float>(signed_hypot(rear_r, lateral));

    return limit_wheels(front_l, rear_l, front_r, rear_r);
}

void set_ackermann(int16_t base, int16_t steer) {
    set_wheels(mix_ackermann(base, steer));
}

}
