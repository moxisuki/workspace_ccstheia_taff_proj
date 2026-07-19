#include "sensors/state/state.h"
#include "sensors/imu/imu.h"
#include "sensors/motor_speed.h"
#include "common/lpf/lpf.h"

namespace sensors::state {

namespace {
common::Lpf gz_lpf(0.3f);
}

void init() {
    gz_lpf.reset();
    sensors::motor_speed::init();
}

State read() {
    auto imu = sensors::imu::read();
    State s = {};
    if (imu.valid) {
        // 角度直接透传(DMP 已融合,不重复算)
        s.yaw   = imu.yaw;
        s.pitch = imu.pitch;
        s.roll  = imu.roll;
        s.gx_raw   = imu.gz; // 角速度:原始值
        s.yaw_rate = gz_lpf.step(imu.gz);
        s.timestamp_ms = imu.timestamp_ms;
        s.valid = imu.valid;
        s.fresh = imu.fresh;
    }
    auto spd = sensors::motor_speed::read();
    s.m1 = spd.m1; s.m2 = spd.m2;
    s.m3 = spd.m3; s.m4 = spd.m4;
    return s;
}

}  // namespace sensors::state