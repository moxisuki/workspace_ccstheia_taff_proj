#pragma once
#include <stdint.h>
#include "common/pid_params.h"
#include "common/vehicle_params.h"

namespace drivers::motor {

// 通道映射:车前进方向(调这)
constexpr int kChL1 = 1, kChL2 = 2;  // 左侧 M1(前),M2(后)
constexpr int kChR1 = 3, kChR2 = 4;  // 右侧 M3(前),M4(后)

// 单轮方向系数(调这): 1=该轮正转为整车前进, -1=该轮反转为整车前进
constexpr int kDirL1 = 1;
constexpr int kDirL2 = 1;
constexpr int kDirR1 = 1;
constexpr int kDirR2 = 1;

// 520 电机参数(调这)
constexpr int     kType      = 1;
constexpr int     kPhase     = 30;     // 减速比
constexpr int     kLine      = 11;     // 磁环线数
constexpr float   kWheelDia  = 70.00f; // 轮径 mm
constexpr int     kDeadzone  = 1900;
// 车体几何 Ackermann 修正
constexpr float   kTrackWidth = common::vehicle_params::ackermann::kTrackWidthMm;
constexpr float   kWheelBase  = common::vehicle_params::ackermann::kWheelBaseMm;
constexpr int16_t kSpeedLimit = common::vehicle_params::ackermann::kSpeedLimit;
constexpr float   kSpeedP    = common::pid_params::motor_speed::kP;
constexpr float   kSpeedI    = common::pid_params::motor_speed::kI;
constexpr float   kSpeedD    = common::pid_params::motor_speed::kD;

struct WheelSpeeds {
    int16_t left_front;
    int16_t left_rear;
    int16_t right_front;
    int16_t right_rear;
};

void init();

// 裸通道输出: 直接发给驱动板, 不做方向修正
void set(int16_t m1, int16_t m2, int16_t m3, int16_t m4);

// 语义轮位输出: 自动映射到通道并套用单轮方向系数
void set_wheels(const WheelSpeeds& wheels);

// 阿克曼近似: steer > 0 为右转; 前轮按转向侧向分量做合速度补偿
WheelSpeeds mix_ackermann(int16_t base, int16_t steer);
void set_ackermann(int16_t base, int16_t steer);

}
