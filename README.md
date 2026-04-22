# STUMPY – 6-Thruster ROV Control System

STUMPY is a real-time embedded control system for a 6-thruster underwater ROV built around an STM32 NUCLEO-F446RE, a Raspberry Pi 4 bridge, and a laptop-based controller and telemetry interface.

The system supports bidirectional thruster control, TCP-to-UART communication, live telemetry, and minimal IMU feedback through a modular multi-device architecture.

---

## Overview

This project was developed to control and monitor a 6-thruster ROV using:

- **STM32 NUCLEO-F446RE** for low-level real-time firmware
- **Raspberry Pi 4** as a TCP ↔ UART bridge
- **Laptop Python tools** for controller input and telemetry display
- **Xbox 360 controller** for manual control input
- **MPU6050 IMU** for basic onboard feedback

---

## System Architecture

Laptop Controller  
→ TCP (Control Port 5000)  
→ Raspberry Pi Bridge  
→ UART  
→ STM32 (NUCLEO-F446RE)  
→ PWM Outputs  
→ 6 ESCs  
→ Thrusters  

Telemetry Path:  
STM32  
→ UART  
→ Raspberry Pi Bridge  
→ TCP (Telemetry Port 5001)  
→ Laptop Telemetry Viewer  

---

## Features

- 6-channel bidirectional thruster control
- PWM output range of **1000–2000**
- Neutral throttle at **1500**
- Xbox 360 controller support through `pygame`
- Raspberry Pi 4 TCP/UART bridge
- Split-port architecture for control and telemetry
- Live telemetry dashboard
- MPU6050 IMU integration
- Arming, disarming, stop, and failsafe logic
- STM32CubeIDE / CubeMX firmware project included

---

## Hardware

- STM32 NUCLEO-F446RE
- Raspberry Pi 4
- 6x bidirectional BLHeli_S ESCs
- 6x thrusters
- MPU6050 IMU
- Xbox 360 controller

---

## Thruster Mapping

- **T1** → Front Left Vertical
- **T2** → Front Right Vertical
- **T3** → Left Horizontal
- **T4** → Right Horizontal
- **T5** → Rear Left Vertical
- **T6** → Rear Right Vertical

---

## Command Protocol

The final working control protocol uses:

```text
ARM
DISARM
STOP
SET:1500,1500,1600,1400,1500,1500
```

Where:

```text
SET:T1,T2,T3,T4,T5,T6
```

Example:

```text
SET:1500,1500,1700,1300,1500,1500
```

---

## Repository Structure

```text
STUMPY/
├── README.md
├── .gitignore
├── CMakeLists.txt
├── docs/
│   └── images/
|   └── firmware_config.md
├── firmware/
│   ├── README.md
│   ├── CMakeLists.txt
│   ├── .settings/
│   ├── Core/
│   ├── Drivers/
│   ├── .project
│   ├── .cproject
│   ├── .mxproject
│   ├── F446RE_cubeide.ioc
│   ├── STM32F446RETX_FLASH.ld
│   └── STM32F446RETX_RAM.ld
├── raspberry_pi/
│   ├── README.md
│   └── bridge.py
└── laptop/
    ├── README.md
    ├── controller.py
    └── telemetry_view.py
```

---

## Demo Description

STUMPY supports real-time manual control of all six thrusters through an Xbox 360 controller while providing live telemetry and minimal IMU feedback. The system demonstrates embedded firmware development, PWM motor control, UART/TCP communication, and multi-device integration across STM32, Raspberry Pi, and laptop software.

---

## Challenges and Debugging

This project involved solving several real embedded systems integration issues, including:

- UART communication problems between Pi and STM32
- startup state differences after shutdown and reboot
- ST-LINK target detection failures
- ESC startup and arming timing behavior
- command protocol mismatches during firmware iteration
- synchronization between firmware, bridge, and controller software

---

## Future Improvements

- PID stabilization
- depth control
- more advanced IMU filtering / sensor fusion
- GUI control station
- more robust startup and boot-state handling
- autonomous or semi-autonomous behaviors

---

## Summary

This project demonstrates:

- embedded C firmware development
- PWM motor control
- UART and I2C integration
- TCP socket programming
- Python tooling for control and telemetry
- hardware/software debugging across multiple devices
- real-world system integration

---

## Author

Garrett Anderson
