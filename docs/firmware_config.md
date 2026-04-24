# Firmware Configuration (STM32CubeMX)

## Project
- **Project Name:** STUMPY
- **Board / MCU:** STM32 NUCLEO-F446RE
- **IDE:** STM32CubeIDE / CubeMX

## Functional Summary
The STM32 firmware is responsible for:
- generating six PWM outputs for ESC / thruster control
- receiving control commands over UART from the Raspberry Pi bridge
- reading MPU6050 IMU data over I2C
- calibrating gyro bias at startup
- computing filtered and fused roll / pitch estimates
- publishing telemetry back over UART
- managing `MANUAL`, `TEST`, and `ASSIST` modes
- enforcing arming, disarming, stop, and failsafe behavior

## PWM / Thruster Configuration
### PWM Range
- **Minimum PWM:** 1000
- **Neutral PWM:** 1500
- **Maximum PWM:** 2000

### Thruster Mapping
- **T1** → Front Left Vertical
- **T2** → Front Right Vertical
- **T3** → Left Horizontal
- **T4** → Right Horizontal
- **T5** → Rear Left Vertical
- **T6** → Rear Right Vertical

### Timer Assignment
#### TIM2
- **CH1 → PA0 → T1**
- **CH2 → PA1 → T2**

#### TIM3
- **CH1 → PA6 → T3**
- **CH2 → PA7 → T4**
- **CH3 → PB0 → T5**
- **CH4 → PB1 → T6**

### Timer Settings
- **Prescaler:** 15
- **Period:** 19999
- **Mode:** PWM mode 1

## UART Configuration
### USART1
- **TX → PA9**
- **RX → PA10**
- **Baud Rate:** 115200
- **Mode:** TX / RX

## I2C Configuration
### I2C1
- **SCL → PB8**
- **SDA → PB9**
- **Clock Speed:** 100 kHz
- **Device:** MPU6050

## GPIO
- **PA5** → onboard status LED

## IMU Processing
At boot, the firmware:
1. initializes the MPU6050
2. reads `WHO_AM_I`
3. performs gyro bias calibration while the board is still
4. initializes the complementary filter state

Telemetry includes:
- `WHO`
- `AX AY AZ`
- `GX GY GZ`
- `TP`
- `R P`
- `BX BY BZ`

## Modes
### MODE:MANUAL
Normal controller-driven mode.

### MODE:TEST
Single-thruster diagnostic mode. One selected thruster is driven while all others stay neutral.

### MODE:ASSIST
Manual operation remains active while firmware applies limited roll / pitch stabilization corrections to the vertical thrusters.

## Command Protocol
```text
ARM
DISARM
STOP
SET:T1,T2,T3,T4,T5,T6
TEST:Tn:value
TEST:STOP
MODE:MANUAL
MODE:ASSIST
```

Examples:
```text
SET:1500,1500,1600,1400,1500,1500
TEST:T2:1600
MODE:ASSIST
```

## Safety / Failsafe
- Thrusters only drive when armed
- `DISARM` returns outputs to neutral
- `STOP` returns commanded outputs to neutral
- command timeout disarms the system and neutralizes outputs
- assist corrections are clamped
- test mode forces all non-selected thrusters to neutral

## Recommended Startup Sequence
1. power STM32
2. allow IMU calibration while the board is still
3. power / connect the Raspberry Pi bridge
4. start telemetry
5. start controller or thruster test utility
6. arm only after confirming neutral outputs
