# STUMPY – 6-Thruster ROV Control System

STUMPY is a real-time embedded control system for a 6-thruster underwater ROV built using:

- STM32 NUCLEO-F446RE (real-time control)
- Raspberry Pi 4 (communication bridge)
- Python tools (control, telemetry, logging, analysis)

## Features
- 6-thruster PWM control
- UART command system
- IMU (MPU6050)
- Complementary filter
- Manual / Test / Assist modes
- CSV logging + plotting
- Failsafe system

## Modes
- MANUAL → direct control
- TEST → single thruster diagnostics
- ASSIST → stabilization

## Architecture
Laptop → Pi → STM32 → Thrusters  
STM32 → Pi → Laptop telemetry

## Repo Structure
- firmware/
- raspberry_pi/
- laptop/

## Status
Stable and functional. Ready for tuning and expansion.
