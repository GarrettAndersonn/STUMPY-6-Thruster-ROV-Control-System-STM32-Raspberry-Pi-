# Firmware

This folder contains the STM32 firmware project for STUMPY.

## Target

- STM32 NUCLEO-F446RE

## Responsibilities

- PWM generation for 6 ESCs / thrusters
- UART command parsing
- Telemetry generation
- I2C IMU communication
- Arming, disarming, stop, and failsafe logic

## Project Files

This folder is intended to contain the full STM32CubeIDE project, including:

- `.ioc` CubeMX configuration
- `Core/`
- `Drivers/`
- `.project`
- `.cproject`
- `.mxproject`
- linker script files

## Notes

This project is primarily intended to be opened in **STM32CubeIDE**.

The included `CMakeLists.txt` is mainly provided to document the structure and support future standalone firmware builds.
