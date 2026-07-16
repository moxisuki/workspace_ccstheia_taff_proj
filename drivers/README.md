# drivers/

Low-level hardware abstraction. Each peripheral lives in its own folder:

```
drivers/
├── i2c/         # I2C bus master (used by sensors/mpu etc.)
├── spi/         # SPI bus master
├── uart/        # async serial
├── gpio/        # GPIO wrapper
└── ...
```

Rules:
- One peripheral per folder, no sensor-specific logic here.
- Header API takes opaque handles, not raw register names.
- Configuration (pin mux, clock, DMA) is done via `main.syscfg`.