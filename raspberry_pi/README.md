# Raspberry Pi Bridge

## Overview
Acts as a communication bridge between laptop and STM32.

## Responsibilities
- Receive TCP control commands
- Forward commands to STM32 (UART)
- Receive telemetry from STM32
- Broadcast telemetry to clients

## Ports
- 5000 → control
- 5001 → telemetry

## Architecture
Laptop → TCP → Pi → UART → STM32

## Design Philosophy
- Keep simple
- No control logic here
- STM32 remains authority

## Run
```bash
python3 bridge.py
