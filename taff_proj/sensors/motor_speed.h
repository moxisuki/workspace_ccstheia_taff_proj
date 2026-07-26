#pragma once
#include <stdint.h>

namespace sensors::motor_speed {

struct Data {
    int16_t m1, m2, m3, m4;  // rpm / 10
    bool    fresh;
};

void init();
Data read();
void on_rx_byte(uint8_t b);

}