# Laptop Tools

This folder contains the laptop-side Python tools for STUMPY.

## Typical Files
- `controller.py` — joystick-driven manual control
- `telemetry_view.py` — live telemetry viewer + CSV logger
- `thruster_test.py` — single-thruster diagnostics utility
- plotting scripts for saved telemetry logs

## Responsibilities
- send control commands to the Raspberry Pi bridge
- visualize live telemetry
- log telemetry to dated CSV folders
- clean up old logs
- test individual thrusters safely
- analyze logged data through generated plots

## Notes
The laptop tools are intentionally separated by role:
- controller for normal drive
- telemetry for display/logging
- diagnostics for test mode
