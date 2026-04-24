# Firmware

This folder contains the STM32 firmware project for STUMPY.

## Target
- STM32 NUCLEO-F446RE

## Responsibilities
- six-channel PWM output
- UART command parsing
- telemetry generation
- MPU6050 I2C reads
- gyro calibration
- complementary filter for roll / pitch
- mode management:
  - `MANUAL`
  - `TEST`
  - `ASSIST`

## Expected Project Contents
This folder should contain the actual STM32CubeIDE project files:
- `Core/`
- `Drivers/`
- `.ioc`
- `.project`
- `.cproject`
- `.mxproject`
- linker scripts

## Notes
Primary build workflow remains STM32CubeIDE / CubeMX generated project files.
