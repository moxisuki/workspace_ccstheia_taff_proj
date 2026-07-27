# taff_proj

MSPM0G3507 嵌入式控制工程(CCS Theia / TI ArmClang / no RTOS)

## 工程结构

```
taff_proj/
├── main.cpp / main.syscfg   # 入口 + SysConfig(4 个 UART + WWDT)
├── app/                     # 应用层:全局模式状态机 / 调度
├── control/                 # 控制层:PID / heading / drive
│   ├── pid/                 #   通用 PID 算法
│   ├── heading/             #   MPU 航向控制
│   └── drive/               #   CCD 循迹 / 阿克曼混控输出
├── sensors/                 # 传感器层
│   ├── imu/                 #   MPU6050 远端板,US2,11 字节 JY901 协议
│   ├── ccd/                 #   TSL1401 线性 CCD
│   └── state/               #   融合层:IMU + Lpf → State
├── drivers/                 # 硬件抽象
│   ├── uart/                #   4 个 UART 字节级 + printf 重定向 + ringbuf
│   ├── motor/               #   电机驱动板串口协议
│   └── systick/             #   1ms SysTick
├── common/                  # 工具
│   ├── lpf/                 #   一阶低通
│   ├── pid_params.h         #   PID 调参参数集中配置
│   ├── drive_params.h       #   行车速度 / 转向 / 循迹阈值
│   ├── vehicle_params.h     #   车体几何 / 阿克曼混控参数
│   ├── rate.h               #   RateGate 周期门控
│   └── math.h               #   wrap_180 / clamp
├── task/                    # 周期任务:drive / debug / led
├── tools/                   # 上位机:CCD 波形 / PID 调参
└── targetConfigs/MSPM0G3507.ccxml  # XDS110 调试配置
```

## 上位机

```powershell
python tools/ccd_host.py --port COM11    # 看 CCD 原始 128 像素波形
python tools/tune_host.py --port COM11   # 轻量曲线 + PID 输入框 + MODE/SET/STOP 命令
python tools/tune_host.py --port COM11 --history 80 --interval 300  # 低配电脑更流畅
```

`tune_host.py` 命令输入示例:

```text
MODE LINE
SET LINE P:5 I:0 D:0.12
SET HEAD P:2 I:0.05 D:0.05
STATUS
STOP
```

上位机底部可以直接填写 `Line PID` / `Head PID` 的 `P/I/D` 并点击 Apply。LED 约定: 程序运行白灯常亮, Stop 额外亮红灯, 参数变更蓝灯短闪。

## 安全

工程启用 WWDT 看门狗，周期 8s。主循环完成传感器、drive、debug 调度后喂狗；复位后默认进入 `Stop`，电机输出 0。

## 引脚分配

| 功能 | UART | TX | RX | 备注 |
|------|------|----|----|------|
| 调试 printf | USART0 | PA10 | PA11 | 115200,DAP-LINK |
| 四驱 | USART1 | PB4 | PB5 | 115200 |
| MPU6050 远端板 | USART2 | PB17 | PA22 | 115200,11 字节 JY901 帧 |
| K230 | USART3 | PB2 | PB3 | 115200 |
