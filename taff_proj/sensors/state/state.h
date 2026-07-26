#pragma once
#include <stdint.h>

namespace sensors::state {

struct Vision {
    bool     valid;
    bool     fresh;
    float    x, y;
    uint8_t  type, flag;
    uint16_t heartbeat_cnt;
    uint32_t bad_xor;
    uint32_t overflow;
};

struct State {
    float yaw, pitch, roll;
    float yaw_rate, gx_raw;
    int16_t m1, m2, m3, m4;
    uint32_t timestamp_ms;
    bool valid, fresh;
    Vision vision;
};

void init();
State read();
}