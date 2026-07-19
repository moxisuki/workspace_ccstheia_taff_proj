#include "sensors/state/state.h"
#include "sensors/imu/imu.h"
#include "drivers/lpf/lpf.h"

namespace sensors::state {

namespace {
drivers::Lpf gz_lpf(0.3f);
}

void init() {
    gz_lpf.reset();
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
    return s;
}

}  // namespace sensors::state