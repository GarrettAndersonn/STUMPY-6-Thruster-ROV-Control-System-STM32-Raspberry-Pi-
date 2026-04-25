# STUMPY – 6-Thruster ROV Control System

STUMPY is a real-time embedded control system for a 6-thruster underwater ROV built using:

- **STM32 NUCLEO-F446RE** (real-time control + PWM generation)
- **Raspberry Pi 4** (TCP ↔ UART communication bridge)
- **Python tools** (controller, telemetry, logging, plotting)

The system supports manual control, diagnostics, and IMU-based stabilization.

---

## 🚀 Key Features

### Control System
- 6-channel bidirectional PWM (1000–2000)
- Smooth ramped motor control
- Real-time UART command parsing
- Failsafe timeout + safe neutral behavior

### Modes
- **MANUAL** → direct user control
- **TEST** → single-thruster diagnostics
- **ASSIST** → manual control + roll/pitch stabilization

### IMU & Sensor Fusion
- MPU6050 (I2C)
- Gyro bias calibration at startup
- Low-pass filtering
- Complementary filter (gyro + accel fusion)
- Real-time roll and pitch estimation

### Telemetry & Analysis
- Live telemetry display
- CSV logging with timestamps
- Auto-organized log folders
- Auto-deletion of logs (7-day retention)
- Plot generation (roll, pitch, thrusters, IMU)

---

## 🧠 System Architecture


Laptop Controller
↓ TCP (port 5000)
Raspberry Pi Bridge
↓ UART
STM32 Firmware
↓ PWM
ESCs → Thrusters

STM32 Telemetry
↑ UART
Raspberry Pi Bridge
↑ TCP (port 5001)
Laptop Telemetry + Logging


---

## ⚙️ Control Modes

### MANUAL
- All thrusters controlled directly via controller input
- No automatic correction

---

### TEST
- Only one thruster is driven at a time
- All others forced to neutral (1500)
- Used for diagnostics and hardware verification

---

### ASSIST
- Manual control remains active
- STM32 applies stabilization corrections:
  - Roll → left/right balancing
  - Pitch → front/back balancing
- Uses complementary filter output

---

## 🎯 Control Strategy

Current implementation:
- **P-based stabilization (proportional control)**
- Correction applied only to vertical thrusters

Future upgrade path:
- PD (add damping)
- PID (if necessary)
- Depth hold
- Heading hold

---

## 📡 Command Protocol

### Core Commands

ARM
DISARM
STOP
SET:T1,T2,T3,T4,T5,T6


### Mode Commands

MODE:MANUAL
MODE:ASSIST


### Test Commands

TEST:Tn:value
TEST:STOP


Example:

SET:1500,1500,1600,1400,1500,1500
TEST:T3:1600
MODE:ASSIST


---

## 📊 Telemetry

Example output:


SEQ:120 ARM:1 MODE:ASSIST TEST:NONE TPWM:1500
T1:1500 T2:1500 ...
O1:1500 O2:1500 ...
IMU:1 WHO:104
AX:-200 AY:300 AZ:-980
GX:0 GY:-1 GZ:0
TP:28
R:2.1 P:-1.4
BX:0.5 BY:-0.3 BZ:0.1


---

## 🧪 Testing Workflow

1. Power STM32 (keep still for IMU calibration)
2. Start Raspberry Pi bridge
3. Run telemetry viewer
4. Run controller
5. ARM system
6. Test:
   - Manual control
   - Test mode
   - Assist mode

---

## 📁 Project Structure


STUMPY/
├── firmware/ → STM32 firmware
├── raspberry_pi/ → TCP ↔ UART bridge
├── laptop/ → control + telemetry tools
├── docs/ → configuration + diagrams
├── logs/ → telemetry logs
├── plots/ → generated plots


---

## 🔒 Safety Features

- Neutral output when disarmed
- Command timeout failsafe
- Test mode isolates thrusters
- Assist corrections are clamped
- Startup delay for ESC initialization
- IMU failure fallback

---

## 💡 What This Project Demonstrates

- Embedded systems (STM32 HAL, timers, PWM)
- Sensor fusion (complementary filter)
- Real-time control systems
- Networking (TCP ↔ UART bridge)
- Diagnostics + logging pipelines
- Safe robotics design

---

## 📌 Summary

STUMPY is a complete control system combining:
- embedded firmware
- real-time control logic
- sensor processing
- networked communication
- data logging and analysis

This project reflects real-world robotics system design, not ju
