#include "sensors/state/state.h"
#include "sensors/imu/imu.h"
#include "sensors/motor_speed/motor_speed.h"
#include "sensors/k230/k230.h"
#include "common/lpf/lpf.h"
#include "drivers/systick/systick.h"

namespace sensors::state {
namespace {
common::Lpf gz_lpf(0.3f);
constexpr uint32_t kImuStaleMs = 300;
}
void init() { gz_lpf.reset(); sensors::motor_speed::init(); }
State read() {
    auto imu = sensors::imu::read();
    State s = {};
    if (imu.valid) {
        uint32_t now = drivers::systick::now_ms();
        s.yaw = imu.yaw; s.pitch = imu.pitch; s.roll = imu.roll;
        s.gx_raw = imu.gz; s.yaw_rate = gz_lpf.step(imu.gz);
        s.timestamp_ms = imu.timestamp_ms;
        s.valid = (now - imu.timestamp_ms) <= kImuStaleMs;
        s.fresh = imu.fresh;
    }
    auto spd = sensors::motor_speed::read();
    s.m1 = spd.m1; s.m2 = spd.m2; s.m3 = spd.m3; s.m4 = spd.m4;
    s.vision = sensors::k230::read();
    return s;
}
}  // namespace sensors::state
