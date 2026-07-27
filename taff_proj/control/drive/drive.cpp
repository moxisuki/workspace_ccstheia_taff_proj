#include "control/drive/drive.h"

#include "common/drive_params.h"
#include "common/math.h"
#include "common/pid_params.h"
#include "control/heading/heading.h"
#include "control/pid/pid.h"

namespace control::drive {

namespace {
Mode g_mode = Mode::Stop;
LinePolarity g_polarity = LinePolarity::Dark;
LineInfo g_line = {};

namespace line_pid = common::pid_params::line_trace;

Pid g_line_pid(line_pid::kP, line_pid::kI, line_pid::kD, line_pid::kIMax);

int16_t g_speed = 0;
int16_t g_manual_base = 0;
int16_t g_manual_steer = 0;
int16_t g_last_base = 0;
int16_t g_last_steer = 0;
bool g_lock_pending = false;
bool g_turn_target_pending = false;
bool g_turning = false;
int g_settle = 0;
float g_turn_deg = 0.0f;

constexpr float kLineCenter = (sensors::ccd::kPixels - 1) * 0.5f;
constexpr uint8_t kMinContrast = common::drive_params::line::kMinContrast;
constexpr float kSettleErr = common::drive_params::turn::kSettleErr;
constexpr float kSettleRate = common::drive_params::turn::kSettleRate;
constexpr int kSettleCnt = common::drive_params::turn::kSettleCnt;

bool settled(float err, float rate) {
    if (common::clamp(err, -kSettleErr, kSettleErr) == err
     && common::clamp(rate, -kSettleRate, kSettleRate) == rate) {
        return ++g_settle >= kSettleCnt;
    }
    g_settle = 0;
    return false;
}

int16_t clamp_speed(int16_t speed) {
    return static_cast<int16_t>(common::clamp(speed, -1000, 1000));
}

int16_t clamp_steer(float steer) {
    return static_cast<int16_t>(common::clamp(steer, -kMaxSteer, kMaxSteer));
}

Output make_output(int16_t base, int16_t steer, bool active) {
    g_last_base = active ? base : 0;
    g_last_steer = active ? steer : 0;

    Output out = {};
    out.base = g_last_base;
    out.steer = g_last_steer;
    out.active = active;
    out.line = g_line;
    return out;
}

Output inactive() {
    return make_output(0, 0, false);
}

Output step_straight(const sensors::state::State& s, float dt) {
    if (!s.valid || dt <= 0.0f) return inactive();

    if (g_lock_pending) {
        heading::reset();
        heading::set_target(s.yaw);
        g_lock_pending = false;
    }

    float steer = heading::step(s.yaw, s.yaw_rate, dt);
    return make_output(g_speed, clamp_steer(steer), true);
}

Output step_uturn(const sensors::state::State& s, float dt) {
    if (!s.valid || dt <= 0.0f) return inactive();

    if (g_turn_target_pending) {
        heading::reset();
        heading::set_target(s.yaw + g_turn_deg);
        g_turn_target_pending = false;
    }

    float steer = heading::step(s.yaw, s.yaw_rate, dt);
    if (g_turning && settled(heading::last_error(), s.yaw_rate)) {
        g_turning = false;
    }

    if (!g_turning) return inactive();
    return make_output(g_speed, clamp_steer(steer), true);
}

Output step_line_trace(const sensors::state::State& s, const uint8_t* pixels, float dt) {
    g_line = analyze_line(pixels);
    if (!g_line.valid || dt <= 0.0f) return inactive();

    float steer = g_line_pid.step(g_line.error, dt);
    if (s.valid) {
        steer -= line_pid::kYawRateDamp * s.yaw_rate;
    }
    return make_output(g_speed, clamp_steer(steer), true);
}

}  // namespace

void init() {
    enter_stop();
}

void set_line_polarity(LinePolarity polarity) {
    g_polarity = polarity;
    g_line_pid.reset();
}

void enter_stop() {
    g_mode = Mode::Stop;
    g_speed = 0;
    g_manual_base = 0;
    g_manual_steer = 0;
    g_lock_pending = false;
    g_turn_target_pending = false;
    g_turning = false;
    g_settle = 0;
    g_turn_deg = 0.0f;
    heading::reset();
    g_line_pid.reset();
    make_output(0, 0, false);
}

void enter_straight(int16_t speed) {
    g_mode = Mode::Straight;
    g_speed = clamp_speed(speed);
    g_lock_pending = true;
    g_turn_target_pending = false;
    g_turning = false;
    g_settle = 0;
    heading::reset();
}

void enter_line_trace(int16_t speed) {
    g_mode = Mode::LineTrace;
    g_speed = clamp_speed(speed);
    g_lock_pending = false;
    g_turn_target_pending = false;
    g_turning = false;
    g_settle = 0;
    g_line_pid.reset();
}

void enter_uturn(float deg, int16_t speed) {
    g_mode = Mode::UTurn;
    heading::reset();
    g_speed = clamp_speed(speed);
    g_lock_pending = false;
    g_turn_target_pending = true;
    g_turning = true;
    g_settle = 0;
    g_turn_deg = deg;
}

void enter_manual(int16_t base, int16_t steer) {
    g_mode = Mode::Manual;
    g_manual_base = clamp_speed(base);
    g_manual_steer = clamp_steer(steer);
    g_lock_pending = false;
    g_turn_target_pending = false;
    g_turning = false;
    g_settle = 0;
}

LineInfo analyze_line(const uint8_t* pixels, int count) {
    LineInfo info = {};
    if (pixels == nullptr || count <= 0) return info;

    uint8_t lo = pixels[0];
    uint8_t hi = pixels[0];
    for (int i = 1; i < count; ++i) {
        if (pixels[i] < lo) lo = pixels[i];
        if (pixels[i] > hi) hi = pixels[i];
    }

    info.min = lo;
    info.max = hi;
    info.threshold = static_cast<uint8_t>((static_cast<uint16_t>(lo) + hi) / 2);
    if (static_cast<uint8_t>(hi - lo) < kMinContrast) return info;

    float wsum = 0.0f;
    float xsum = 0.0f;
    for (int i = 0; i < count; ++i) {
        float w = 0.0f;
        if (g_polarity == LinePolarity::Dark && pixels[i] < info.threshold) {
            w = static_cast<float>(info.threshold - pixels[i]);
        } else if (g_polarity == LinePolarity::Light && pixels[i] > info.threshold) {
            w = static_cast<float>(pixels[i] - info.threshold);
        }
        wsum += w;
        xsum += static_cast<float>(i) * w;
    }

    if (wsum <= 0.0f) return info;

    info.center = xsum / wsum;
    info.error = info.center - kLineCenter;
    info.valid = true;
    return info;
}

Output step(const sensors::state::State& s, const uint8_t* ccd_pixels, float dt) {
    switch (g_mode) {
    case Mode::Stop:
        return inactive();
    case Mode::Straight:
        return step_straight(s, dt);
    case Mode::LineTrace:
        return step_line_trace(s, ccd_pixels, dt);
    case Mode::UTurn:
        return step_uturn(s, dt);
    case Mode::Manual:
        return make_output(g_manual_base, g_manual_steer, true);
    }
    return inactive();
}

Status status() {
    return {
        g_mode,
        g_speed,
        g_last_base,
        g_last_steer,
        g_line.valid,
        g_turning,
        g_line,
    };
}

void cmd_go(int16_t speed) {
    enter_straight(speed);
}

void cmd_turn(float deg, int16_t speed) {
    enter_uturn(deg, speed);
}

void cmd_steer(int16_t base, int16_t steer) {
    enter_manual(base, steer);
}

void cmd_stop() {
    enter_stop();
}

Mode mode() { return g_mode; }

LineInfo last_line() { return g_line; }

bool turning() { return g_turning; }

bool uturn_done() { return g_mode == Mode::UTurn && !g_turning; }

void set_line_pid(float kp, float ki, float kd) {
    g_line_pid.set_gains(kp, ki, kd);
}

void get_line_pid(float& kp, float& ki, float& kd) {
    kp = g_line_pid.kp();
    ki = g_line_pid.ki();
    kd = g_line_pid.kd();
}

const char* to_string(Mode mode) {
    switch (mode) {
    case Mode::Stop:      return "Stop";
    case Mode::Straight:  return "Straight";
    case Mode::LineTrace: return "LineTrace";
    case Mode::UTurn:     return "UTurn";
    case Mode::Manual:    return "Manual";
    }
    return "Unknown";
}

}  // namespace control::drive
