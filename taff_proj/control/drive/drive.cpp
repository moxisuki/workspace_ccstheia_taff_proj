#include "control/drive/drive.h"

#include "common/math.h"
#include "control/heading/heading.h"

namespace control::drive {

namespace {
Mode    g_mode        = Mode::Idle;
int16_t g_speed       = 0;
int16_t g_steer_base  = 0;
int16_t g_steer_turn  = 0;
float   g_yaw         = 0;
bool    g_lock_pending = false;
bool    g_turning      = false;
int     g_settle       = 0;

constexpr float kSettleErr  = 3.0f;
constexpr float kSettleRate = 10.0f;
constexpr int   kSettleCnt  = 5;

bool settled(float err, float rate) {
    if (common::clamp(err, -kSettleErr, kSettleErr)   == err
     && common::clamp(rate, -kSettleRate, kSettleRate) == rate) {
        return ++g_settle >= kSettleCnt;
    }
    g_settle = 0;
    return false;
}
}  // namespace

void init() {}

Output step(const sensors::state::State& s, float dt) {
    if (!s.valid || !s.fresh) return {};

    g_yaw = s.yaw;
    float turn  = heading::step(s.yaw, s.yaw_rate, dt);
    float err   = heading::last_error();
    float rate  = s.yaw_rate;
    Output out  = {};

    switch (g_mode) {
    case Mode::Idle:
        break;
    case Mode::Heading:
        if (g_lock_pending) {
            heading::set_target(s.yaw);
            g_lock_pending = false;
        }
        if (g_turning && settled(err, rate)) {
            g_turning = false;
        }
        out.base = g_speed;
        out.turn = static_cast<int16_t>(turn);
        break;
    case Mode::Steer:
        out.base = g_steer_base;
        out.turn = g_steer_turn;
        break;
    }
    return out;
}

void cmd_go(int16_t speed) {
    g_speed        = speed;
    g_lock_pending = true;
    g_turning      = false;
    g_mode         = Mode::Heading;
}

void cmd_turn(float deg, int16_t speed) {
    heading::set_target(g_yaw + deg);
    g_speed   = speed;
    g_turning = true;
    g_settle  = 0;
    g_mode    = Mode::Heading;
}

void cmd_steer(int16_t base, int16_t turn) {
    g_steer_base = base;
    g_steer_turn = turn;
    g_mode       = Mode::Steer;
}

void cmd_stop() { g_mode = Mode::Idle; }

Mode mode()      { return g_mode; }
bool turning()   { return g_turning; }

}  // namespace control::drive