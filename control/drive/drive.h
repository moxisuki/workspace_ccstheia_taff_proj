#pragma once
#include "sensors/state/state.h"
#include <stdint.h>

namespace control::drive {

struct Output { int16_t base; int16_t turn; };

enum class Mode : uint8_t { Idle, Heading, Steer };

void init();
Output step(const sensors::state::State& s, float dt);

/** @brief 直行,PID 控方向,锁当前航向
 *  @param speed -1000~1000 */
void cmd_go(int16_t speed);

/** @brief 转向,PID 控,到位自动切直行
 *  @param deg   +90=右转, -90=左转
 *  @param speed 0=原地转, >0=弧线 */
void cmd_turn(float deg, int16_t speed = 0);

/** @brief 直给,不走 PID
 *  @param base -1000~1000
 *  @param turn 差速量 */
void cmd_steer(int16_t base, int16_t turn);

/** @brief 停车 */
void cmd_stop();

Mode mode();
bool turning();   // true=正在转向, false=已到位

}