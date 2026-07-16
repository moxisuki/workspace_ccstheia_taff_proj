# taff_proj

MSPM0G3507 嵌入式控制工程(CCS Theia / TI ArmClang / no RTOS)

## 工程结构

```
taff_proj/
├── main.cpp / main.syscfg   # 入口 + SysConfig(4 个 UART)
├── app/                     # 调度层:tick() 控制 100Hz 控制 + 20Hz 打印
├── control/                 # 算法层:PID / heading
│   ├── pid/                 #   通用 PID
│   └── heading/             #   航向 PD 控制
├── sensors/                 # 传感器层
│   ├── imu/                 #   MPU6050 远端板,US2,11 字节 JY901 协议
│   └── state/               #   融合层:IMU + Lpf → State
├── drivers/                 # 硬件抽象
│   ├── uart/                #   4 个 UART 字节级 + printf 重定向 + ringbuf
│   ├── lpf/                 #   一阶低通
│   └── systick/             #   1ms SysTick
├── common/                  # 工具
│   ├── rate.h               #   RateGate 周期门控
│   └── math_util.h          #   wrap_180 / clamp
└── targetConfigs/MSPM0G3507.ccxml  # XDS110 调试配置
```

## 引脚分配

| 功能 | UART | TX | RX | 备注 |
|------|------|----|----|------|
| 调试 printf | USART0 | PA10 | PA11 | 115200,DAP-LINK |
| 四驱 | USART1 | PB4 | PB5 | 115200 |
| MPU6050 远端板 | USART2 | PB17 | PA22 | 115200,11 字节 JY901 帧 |
| K230 | USART3 | PB2 | PB3 | 115200 |