# STUMPY

STUMPY is a real-time embedded control system for a 6-thruster underwater ROV built around an STM32 NUCLEO-F446RE, a Raspberry Pi 4 bridge, and laptop-side Python tools for control, telemetry, diagnostics, logging, and assist-mode development.

## Current Features
- 6-thruster bidirectional PWM control
- STM32 UART command parsing
- Raspberry Pi TCP-to-UART bridge
- Laptop manual controller
- Telemetry viewer with CSV logging
- Plotting utilities for telemetry logs
- MPU6050 IMU integration
- Gyro bias calibration at startup
- Complementary filter for roll/pitch
- Control modes: `MANUAL`, `TEST`, `ASSIST`
- Single-thruster diagnostics mode
- Basic leveling assist using IMU feedback

## Architecture
Laptop controller → TCP control (`:5000`) → Raspberry Pi bridge → UART → STM32 → ESCs / thrusters

STM32 telemetry → UART → Raspberry Pi bridge → TCP telemetry (`:5001`) → laptop telemetry / CSV logs / plotting

## Repository Layout
```text
STUMPY/
├── README.md
├── .gitignore
├── CMakeLists.txt
├── docs/
│   ├── firmware_config.md
│   └── images/
├── firmware/
│   ├── README.md
│   └── CMakeLists.txt
├── raspberry_pi/
│   ├── README.md
│   └── CMakeLists.txt
└── laptop/
    ├── README.md
    └── CMakeLists.txt
```

## Core Modes
### MANUAL
Normal controller-driven operation.

### TEST
Single-thruster diagnostic mode. One selected thruster is driven while all others remain neutral.

### ASSIST
Manual control remains active while STM32 applies limited roll/pitch stabilization corrections to the vertical thrusters.

## Command Protocol
Supported commands:
```text
ARM
DISARM
STOP
SET:T1,T2,T3,T4,T5,T6
TEST:Tn:value
TEST:STOP
MODE:MANUAL
MODE:ASSIST
```

## Resume Summary
STUMPY demonstrates embedded C firmware development, STM32 timer/PWM configuration, UART and I2C integration, Python control and telemetry tooling, TCP/serial bridging, IMU filtering and fusion, diagnostics tooling, and system integration/tuning.
