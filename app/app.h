#pragma once

namespace app {

// 初始化所有模块,在 main 调一次
void init();

// 主循环体,放 while(1) 里空转
// 内部按 100Hz 跑控制、20Hz 打日志
void tick();

}