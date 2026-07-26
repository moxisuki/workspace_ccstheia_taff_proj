#pragma once
#include <stdint.h>
#include <stddef.h>

namespace sensors::motor_speed {
struct Data { int16_t m1, m2, m3, m4; bool fresh; };
void init();
Data read();
void on_rx_byte(uint8_t b);
uint32_t total_rx_bytes();
size_t peek_raw(uint8_t* buf, size_t max);
}