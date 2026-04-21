# Raspberry Pi Bridge

This folder contains the Raspberry Pi 4 bridge script used in STUMPY.

## Purpose

The bridge connects the laptop control and telemetry tools to the STM32 over UART.

## Responsibilities

- accept control commands over TCP on port 5000
- forward control data to STM32 over UART
- receive telemetry from STM32 over UART
- rebroadcast telemetry over TCP on port 5001

## Script

- `bridge.py`

## Typical usage

```bash
python3 bridge.py
```

## Notes

The Raspberry Pi serial port used in this project is:

```text
/dev/serial0
```

A typical serial setup command used during bring-up was:

```bash
stty -F /dev/serial0 115200 cs8 -cstopb -parenb -ixon -ixoff -echo raw
```
