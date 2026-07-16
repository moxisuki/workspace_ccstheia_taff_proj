#pragma once
#include <stdint.h>

namespace sensors::state {

// 当前状态的struct
struct State {
    float yaw;             // deg, 0~360,可能跳变(用 wrap_180 算误差)
    float pitch, roll;     // deg
    float yaw_rate;        // deg/s,已低通
    float gx_raw;          // deg/s,未滤波,调试对比用
    uint32_t timestamp_ms; // SysTick 1ms 计数
    bool valid;            // IMU 通讯正常
    bool fresh;            // 自上次 read 后有新帧
};

void init();
State read();

}