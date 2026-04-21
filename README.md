# DOLPHIN Project GitHub Bundle

Stable working project bundle for:
- STM32 NUCLEO-F446RE firmware
- Raspberry Pi split-port bridge
- Laptop controller
- Laptop telemetry viewer

## Architecture

Laptop controller -> TCP port 5000 -> Raspberry Pi bridge -> UART -> STM32 -> PWM -> 6 ESCs -> thrusters

STM32 telemetry -> UART -> Raspberry Pi bridge -> TCP port 5001 -> laptop telemetry viewer

## Thruster layout

- T1 = VERT_FRONT_LEFT
- T2 = VERT_FRONT_RIGHT
- T3 = HORIZ_LEFT
- T4 = HORIZ_RIGHT
- T5 = VERT_REAR_LEFT
- T6 = VERT_REAR_RIGHT

## STM32 pin map

- PA0  -> TIM2_CH1 -> T1
- PA1  -> TIM2_CH2 -> T2
- PA6  -> TIM3_CH1 -> T3
- PA7  -> TIM3_CH2 -> T4
- PB0  -> TIM3_CH3 -> T5
- PB1  -> TIM3_CH4 -> T6
- PA9  -> USART1_TX
- PA10 -> USART1_RX
- PB8  -> I2C1_SCL
- PB9  -> I2C1_SDA
- PA5  -> LED

## ESC PWM settings

- Min = 1000
- Neutral = 1500
- Max = 2000

## Notes

This bundle includes the key working firmware `main.c` and host-side scripts. In CubeMX / CubeIDE, make sure the generated project matches the timers, UART, I2C, and pin mapping above.

## Raspberry Pi setup

```bash
sudo apt update
sudo apt install -y python3-pip
pip3 install pyserial
python3 bridge.py
```

## Laptop setup

```bash
pip install pygame
python telemetry_view.py
python controller.py
```
