# Laptop Tools

## Overview
Provides control, telemetry, diagnostics, and analysis tools.

## Components

### controller.py
- Reads Xbox controller
- Sends SET commands
- Y toggles MANUAL / ASSIST

### telemetry_view.py
- Displays live telemetry
- Logs CSV data
- Organizes logs by date
- Deletes old logs automatically

### thruster_test.py
- Isolated thruster testing
- Safe diagnostics mode

### plot_telemetry_save.py
- Generates plots from logs
- Saves PNG images

## Usage
```bash
python telemetry_view.py
python controller.py
