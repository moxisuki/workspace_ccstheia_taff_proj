#pragma once
#include <stdint.h>

namespace sensors::imu {

struct Data {
    float roll;        // deg, from 0x53
    float pitch;       // deg
    float yaw;         // deg
    float gx, gy, gz;  // deg/s, from 0x52
    float ax, ay, az;  // g,     from 0x51
    float temperature; // degC
    uint32_t timestamp_ms;
    bool  valid;
    bool  fresh;       // new frame parsed since last read()
};

void init();
Data read();
bool isFault();

// Called by the USART2 ISR. Do not call from main.
void on_rx_byte(uint8_t b);

}