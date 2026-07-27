# Task

task 层只做周期逻辑编排，不直接写底层协议，也不初始化硬件。硬件和控制器初始化统一放在 `main.cpp`。

## 当前任务

| 任务 | 周期 | 职责 |
|------|------|------|
| `task::drive` | 100Hz | 按全局模式执行停车 / 循迹 / 直线 / 掉头，统一输出 `base + steer` 到阿克曼电机混控 |
| `task::debug` | 20Hz | 异步输出 `DBG` 调参帧，并接收上位机调参命令 |

## Drive

```cpp
#include "task/drive_task.h"

task::drive::init();
task::drive::loop(s, dt);
```

`drive_task` 会读取 `app::global_mode::mode()`:

```text
Stop      -> 停车等待, 电机输出 0
LineTrace -> CCD 线中心 PID + MPU yaw_rate 阻尼
Straight  -> MPU yaw PID 锁航向
UTurn     -> MPU yaw PID 转 180°, 完成后切回 LineTrace
```

输出约定:

```text
base  = 前进速度
steer = 转向量, >0 右转, <0 左转
```

## Debug

```cpp
#include "task/debug_task.h"

task::debug::init();
task::debug::loop(s);
```

输出一行一帧:

```text
DBG,t=123,app=LineTrace,drv=LineTrace,base=260,steer=-35,yaw10=12,yr10=-4,hd10=0,line=1,lc10=635,le10=0,m1=260,m2=258,m3=255,m4=257
```

字段约定:

```text
yaw10 / yr10 / hd10 / lc10 / le10 = 原值 x10
line=1 表示 CCD 找线有效
```

调参上位机:

```powershell
python tools/tune_host.py --port COM11
python tools/tune_host.py --port COM11 --history 80 --interval 300
```

常用命令:

```text
MODE LINE
MODE STRAIGHT
MODE UTURN
STOP
SET LINE P:5 I:0 D:0.12
SET HEAD P:2 I:0.05 D:0.05
STATUS
RESET
```

`DBG` 输出走 UART 异步队列，主循环会持续服务发送，避免长文本调试帧阻塞 100Hz 行车控制。

## LED

LED 不再有独立 task。硬件在 `main.cpp` 中初始化:

```cpp
#include "drivers/led3/led3.h"

drivers::led3::init();
```

当前业务指示由 `task::debug::poll_commands()` 维护: 程序运行白灯常亮, Stop 额外亮红灯, 参数变更蓝灯短闪。

需要状态指示时直接调用驱动:

```cpp
drivers::led3::blue(true);
drivers::led3::white(false);
drivers::led3::red(false);
drivers::led3::off();
```

## 新增任务模板

```cpp
#pragma once
#include "sensors/state/state.h"

namespace task::my_task {
void init();
void loop(const sensors::state::State& s, float dt);
}
```
