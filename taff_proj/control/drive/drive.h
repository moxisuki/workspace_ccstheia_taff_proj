#pragma once
#include "common/drive_params.h"
#include "sensors/state/state.h"
#include "sensors/ccd/ccd.h"
#include <stdint.h>

namespace control::drive {

enum class Mode : uint8_t {
    Stop,
    Straight,
    LineTrace,
    UTurn,
    Manual,
};

enum class LinePolarity : uint8_t {
    Dark,   // 黑线: 像素亮度低于背景
    Light,  // 白线: 像素亮度高于背景
};

struct LineInfo {
    bool valid;
    uint8_t min;
    uint8_t max;
    uint8_t threshold;
    float center;
    float error;
};

struct Output {
    int16_t base;
    int16_t steer;  // >0 右转, <0 左转
    bool active;
    LineInfo line;
};

struct Status {
    Mode mode;
    int16_t target_speed;
    int16_t last_base;
    int16_t last_steer;
    bool line_valid;
    bool turning;
    LineInfo line;
};

constexpr int16_t kStraightSpeed = common::drive_params::speed::kStraight;
constexpr int16_t kLineTraceSpeed = common::drive_params::speed::kLineTrace;
constexpr int16_t kUTurnSpeed = common::drive_params::speed::kUTurn;
constexpr int16_t kMaxSteer = common::drive_params::steer::kMax;

void init();
void set_line_polarity(LinePolarity polarity);

void enter_stop();
void enter_straight(int16_t speed = kStraightSpeed);
void enter_line_trace(int16_t speed = kLineTraceSpeed);
void enter_uturn(float deg = 180.0f, int16_t speed = kUTurnSpeed);
void enter_manual(int16_t base, int16_t steer);

LineInfo analyze_line(const uint8_t* pixels, int count = sensors::ccd::kPixels);
Output step(const sensors::state::State& s, const uint8_t* ccd_pixels, float dt);
Status status();

/** @brief 直行,PID 控方向,锁当前航向
 *  @param speed -1000~1000 */
void cmd_go(int16_t speed);

/** @brief 转向,PID 控,到位自动切直行
 *  @param deg   +90=右转, -90=左转
 *  @param speed >0=弧线掉头/转弯 */
void cmd_turn(float deg, int16_t speed = kUTurnSpeed);

/** @brief 直给,不走 PID
 *  @param base -1000~1000
 *  @param steer 转向量, >0 右转 */
void cmd_steer(int16_t base, int16_t steer);

/** @brief 停车 */
void cmd_stop();

Mode mode();
LineInfo last_line();
bool turning();      // true=正在转向, false=已到位
bool uturn_done();
const char* to_string(Mode mode);
void set_line_pid(float kp, float ki, float kd);
void get_line_pid(float& kp, float& ki, float& kd);

}
