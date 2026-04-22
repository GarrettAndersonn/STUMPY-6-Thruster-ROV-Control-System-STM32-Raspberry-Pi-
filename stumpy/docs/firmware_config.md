# Firmware Configuration (STM32CubeMX)

## Project
- Project Name: STUMPY
- MCU: STM32 NUCLEO-F446RE
- IDE: STM32CubeIDE / CubeMX

## PWM Configuration

TIM2:
- CH1 → PA0 → T1
- CH2 → PA1 → T2

TIM3:
- CH1 → PA6 → T3
- CH2 → PA7 → T4
- CH3 → PB0 → T5
- CH4 → PB1 → T6

PWM Range:
- Min: 1000
- Neutral: 1500
- Max: 2000

## UART

USART1:
- TX: PA9
- RX: PA10
- Baud: 115200

## I2C

I2C1:
- SCL: PB8
- SDA: PB9
- Device: MPU6050

## Command Protocol

ARM
DISARM
STOP
SET:T1,T2,T3,T4,T5,T6

Example:
SET:1500,1500,1600,1400,1500,1500

## Notes

- ESCs require neutral at startup
- IMU is non-blocking
- Failsafe returns to neutral
