# Laptop Tools

This folder contains the laptop-side Python tools used for controlling and monitoring STUMPY.

## Files

- `controller.py`  
  Reads Xbox 360 controller input and sends real-time control commands to the Raspberry Pi bridge.

- `telemetry_view.py`  
  Connects to the telemetry port and displays live system feedback in the terminal.

## Responsibilities

- convert user input into thruster commands
- send control packets to the Raspberry Pi bridge
- display live telemetry and IMU data

## Typical usage

Run telemetry viewer:

```bash
python telemetry_view.py
```

Run controller:

```bash
python controller.py
```

## Notes

The final working control protocol used a packed `SET:` command format rather than individual `T1:` commands.
