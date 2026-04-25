# Firmware (STM32)

## Overview
This firmware runs on the STM32 NUCLEO-F446RE and handles all real-time control.

## Responsibilities
- Generate 6 PWM outputs for thrusters
- Parse UART commands
- Read IMU (MPU6050)
- Perform gyro calibration
- Compute roll/pitch (complementary filter)
- Apply assist corrections
- Enforce failsafe and safety behavior

## Modes
- MANUAL
- TEST
- ASSIST

## Key Concepts
- PWM range: 1000–2000
- Neutral: 1500
- Complementary filter for stability
- P-based assist control

## Build
- Open `.ioc` in STM32CubeIDE
- Generate code
- Build and flash

## Notes
- Keep board still during calibration
- Tune assist gains carefully
