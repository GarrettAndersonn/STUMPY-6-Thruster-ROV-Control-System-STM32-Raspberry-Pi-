# Raspberry Pi Bridge

This folder contains the Raspberry Pi 4 bridge utilities for STUMPY.

## Responsibilities
- accept TCP control commands on port `5000`
- forward controller data to STM32 over UART
- receive STM32 telemetry over UART
- broadcast telemetry to laptop clients on port `5001`

## Main Script
- `bridge.py`

## Typical Serial Port
- `/dev/serial0`

## Notes
The bridge should remain lightweight and transparent. STM32 remains the source of truth for motor output behavior.
