# control/

Higher-level control algorithms. Consumes data from `sensors/` and
writes to `drivers/` (actuators). Pure logic, no raw register access.

```
control/
├── pid/         # PID controllers
├── state_machine/
└── ...
```

Typical flow:
```
sensors/mpu -> control/pid -> drivers/gpio (motor driver)
```