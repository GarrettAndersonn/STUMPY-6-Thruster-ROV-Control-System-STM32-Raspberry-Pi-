/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : STUMPY main firmware
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
#define PWM_MIN            1000
#define PWM_NEUTRAL        1500
#define PWM_MAX            2000
#define THRUSTER_COUNT     6

#define TELEMETRY_MS       700
#define FAILSAFE_MS        1200
#define ASSIST_IMU_MS      20

#define MPU6050_ADDR             (0x68 << 1)
#define MPU6050_REG_PWR_MGMT_1   0x6B
#define MPU6050_REG_WHO_AM_I     0x75
#define MPU6050_REG_ACCEL_XOUT_H 0x3B

#define FILTER_ALPHA       0.98f

/* Assist tuning */
#define ASSIST_TARGET_ROLL_DEG   0.0f
#define ASSIST_TARGET_PITCH_DEG  0.0f
#define ASSIST_KP_ROLL           4.0f
#define ASSIST_KP_PITCH          4.0f
#define ASSIST_MAX_DELTA         120.0f

/* Flip these to -1.0f if correction goes the wrong direction */
#define ASSIST_ROLL_SIGN         1.0f
#define ASSIST_PITCH_SIGN        1.0f

typedef enum
{
  MODE_MANUAL = 0,
  MODE_TEST   = 1,
  MODE_ASSIST = 2
} control_mode_t;

static uint8_t armed = 0;
static control_mode_t control_mode = MODE_MANUAL;
static int8_t test_thruster_index = -1;
static uint16_t test_pwm = PWM_NEUTRAL;

static uint16_t thr_cmd[THRUSTER_COUNT] = {
    PWM_NEUTRAL, PWM_NEUTRAL, PWM_NEUTRAL,
    PWM_NEUTRAL, PWM_NEUTRAL, PWM_NEUTRAL
};

static uint16_t thr_out[THRUSTER_COUNT] = {
    PWM_NEUTRAL, PWM_NEUTRAL, PWM_NEUTRAL,
    PWM_NEUTRAL, PWM_NEUTRAL, PWM_NEUTRAL
};

static uint32_t last_telem_tick = 0;
static uint32_t last_cmd_tick = 0;
static uint32_t last_imu_tick = 0;
static uint32_t telem_seq = 0;

static uint8_t imu_ok = 0;
static uint8_t imu_who = 0;

static int16_t imu_ax_mg = 0;
static int16_t imu_ay_mg = 0;
static int16_t imu_az_mg = 0;
static int16_t imu_gx_dps = 0;
static int16_t imu_gy_dps = 0;
static int16_t imu_gz_dps = 0;
static int16_t imu_temp_c = 0;

static float filt_ax_mg = 0.0f;
static float filt_ay_mg = 0.0f;
static float filt_az_mg = 0.0f;
static float filt_gx_dps = 0.0f;
static float filt_gy_dps = 0.0f;
static float filt_gz_dps = 0.0f;
static float filt_temp_c = 0.0f;

static float imu_roll_deg = 0.0f;
static float imu_pitch_deg = 0.0f;

/* Complementary filter state */
static float comp_roll_deg = 0.0f;
static float comp_pitch_deg = 0.0f;
static uint32_t last_filter_tick = 0;

/* Gyro bias calibration */
static float gyro_bias_x = 0.0f;
static float gyro_bias_y = 0.0f;
static float gyro_bias_z = 0.0f;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART1_UART_Init(void);

/* USER CODE BEGIN PFP */
static uint16_t Clamp_PWM(int32_t value);
static float Clamp_Float(float value, float min_val, float max_val);
static void Set_All_Thrusters(uint16_t value);
static void Apply_Thruster_Outputs(void);
static void Send_Line(const char *text);
static void Send_Telemetry(void);
static void Process_Command(char *cmd);
static uint8_t MPU6050_Init_Minimal(void);
static uint8_t MPU6050_Calibrate_Gyro(void);
static uint8_t MPU6050_Read_And_Filter(void);
static const char *Mode_String(void);
static const char *Test_String(void);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
static uint16_t Clamp_PWM(int32_t value)
{
  if (value < PWM_MIN) return PWM_MIN;
  if (value > PWM_MAX) return PWM_MAX;
  return (uint16_t)value;
}

static float Clamp_Float(float value, float min_val, float max_val)
{
  if (value < min_val) return min_val;
  if (value > max_val) return max_val;
  return value;
}

static void Set_All_Thrusters(uint16_t value)
{
  uint16_t v = Clamp_PWM(value);
  for (int i = 0; i < THRUSTER_COUNT; i++)
  {
    thr_cmd[i] = v;
  }
}

static const char *Mode_String(void)
{
  if (control_mode == MODE_TEST) return "TEST";
  if (control_mode == MODE_ASSIST) return "ASSIST";
  return "MANUAL";
}

static const char *Test_String(void)
{
  static char name[8];

  if (control_mode != MODE_TEST || test_thruster_index < 0 || test_thruster_index >= THRUSTER_COUNT)
  {
    return "NONE";
  }

  snprintf(name, sizeof(name), "T%d", test_thruster_index + 1);
  return name;
}

static void Apply_Thruster_Outputs(void)
{
  int i;

  if (!armed)
  {
    for (i = 0; i < THRUSTER_COUNT; i++)
    {
      thr_out[i] = PWM_NEUTRAL;
    }
  }
  else if (control_mode == MODE_MANUAL)
  {
    for (i = 0; i < THRUSTER_COUNT; i++)
    {
      thr_out[i] = Clamp_PWM(thr_cmd[i]);
    }
  }
  else if (control_mode == MODE_TEST)
  {
    for (i = 0; i < THRUSTER_COUNT; i++)
    {
      thr_out[i] = PWM_NEUTRAL;
    }

    if (test_thruster_index >= 0 && test_thruster_index < THRUSTER_COUNT)
    {
      thr_out[test_thruster_index] = Clamp_PWM(test_pwm);
    }
  }
  else /* MODE_ASSIST */
  {
    float base_heave;
    float roll_error;
    float pitch_error;
    float roll_corr = 0.0f;
    float pitch_corr = 0.0f;

    /* Horizontal thrusters remain manual */
    thr_out[2] = Clamp_PWM(thr_cmd[2]);
    thr_out[3] = Clamp_PWM(thr_cmd[3]);

    /* Average vertical command as base heave */
    base_heave = ((float)thr_cmd[0] + (float)thr_cmd[1] +
                  (float)thr_cmd[4] + (float)thr_cmd[5]) * 0.25f;

    if (imu_ok)
    {
      roll_error = ASSIST_TARGET_ROLL_DEG - imu_roll_deg;
      pitch_error = ASSIST_TARGET_PITCH_DEG - imu_pitch_deg;

      roll_corr = ASSIST_ROLL_SIGN * ASSIST_KP_ROLL * roll_error;
      pitch_corr = ASSIST_PITCH_SIGN * ASSIST_KP_PITCH * pitch_error;

      roll_corr = Clamp_Float(roll_corr, -ASSIST_MAX_DELTA, ASSIST_MAX_DELTA);
      pitch_corr = Clamp_Float(pitch_corr, -ASSIST_MAX_DELTA, ASSIST_MAX_DELTA);
    }

    /* Vertical layout:
       T1 front-left
       T2 front-right
       T5 rear-left
       T6 rear-right
    */
    thr_out[0] = Clamp_PWM((int32_t)lroundf(base_heave + pitch_corr + roll_corr));
    thr_out[1] = Clamp_PWM((int32_t)lroundf(base_heave + pitch_corr - roll_corr));
    thr_out[4] = Clamp_PWM((int32_t)lroundf(base_heave - pitch_corr + roll_corr));
    thr_out[5] = Clamp_PWM((int32_t)lroundf(base_heave - pitch_corr - roll_corr));
  }

  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, thr_out[0]);
  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, thr_out[1]);

  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, thr_out[2]);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, thr_out[3]);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, thr_out[4]);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, thr_out[5]);
}

static void Send_Line(const char *text)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)text, strlen(text), 100);
}

static uint8_t MPU6050_Init_Minimal(void)
{
  uint8_t data;

  if (HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, MPU6050_REG_WHO_AM_I, 1, &data, 1, 100) != HAL_OK)
  {
    imu_who = 0;
    return 0;
  }

  imu_who = data;

  data = 0x00;
  if (HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, MPU6050_REG_PWR_MGMT_1, 1, &data, 1, 100) != HAL_OK)
  {
    return 0;
  }

  HAL_Delay(50);
  return 1;
}

static uint8_t MPU6050_Calibrate_Gyro(void)
{
  const int samples = 300;
  float sum_x = 0.0f;
  float sum_y = 0.0f;
  float sum_z = 0.0f;

  for (int i = 0; i < samples; i++)
  {
    uint8_t buf[14];

    if (HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, MPU6050_REG_ACCEL_XOUT_H, 1, buf, 14, 100) != HAL_OK)
    {
      return 0;
    }

    int16_t gx_raw = (int16_t)((buf[8] << 8) | buf[9]);
    int16_t gy_raw = (int16_t)((buf[10] << 8) | buf[11]);
    int16_t gz_raw = (int16_t)((buf[12] << 8) | buf[13]);

    sum_x += gx_raw / 131.0f;
    sum_y += gy_raw / 131.0f;
    sum_z += gz_raw / 131.0f;

    HAL_Delay(5);
  }

  gyro_bias_x = sum_x / samples;
  gyro_bias_y = sum_y / samples;
  gyro_bias_z = sum_z / samples;

  return 1;
}

static uint8_t MPU6050_Read_And_Filter(void)
{
  uint8_t buf[14];

  if (HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, MPU6050_REG_ACCEL_XOUT_H, 1, buf, 14, 100) != HAL_OK)
  {
    return 0;
  }

  int16_t ax_raw = (int16_t)((buf[0] << 8) | buf[1]);
  int16_t ay_raw = (int16_t)((buf[2] << 8) | buf[3]);
  int16_t az_raw = (int16_t)((buf[4] << 8) | buf[5]);
  int16_t temp_raw = (int16_t)((buf[6] << 8) | buf[7]);
  int16_t gx_raw = (int16_t)((buf[8] << 8) | buf[9]);
  int16_t gy_raw = (int16_t)((buf[10] << 8) | buf[11]);
  int16_t gz_raw = (int16_t)((buf[12] << 8) | buf[13]);

  float ax_mg = (ax_raw * 1000.0f) / 16384.0f;
  float ay_mg = (ay_raw * 1000.0f) / 16384.0f;
  float az_mg = (az_raw * 1000.0f) / 16384.0f;

  float gx_dps = (gx_raw / 131.0f) - gyro_bias_x;
  float gy_dps = (gy_raw / 131.0f) - gyro_bias_y;
  float gz_dps = (gz_raw / 131.0f) - gyro_bias_z;

  float temp_c = (temp_raw / 340.0f) + 36.53f;

  filt_ax_mg = 0.8f * filt_ax_mg + 0.2f * ax_mg;
  filt_ay_mg = 0.8f * filt_ay_mg + 0.2f * ay_mg;
  filt_az_mg = 0.8f * filt_az_mg + 0.2f * az_mg;

  filt_gx_dps = 0.85f * filt_gx_dps + 0.15f * gx_dps;
  filt_gy_dps = 0.85f * filt_gy_dps + 0.15f * gy_dps;
  filt_gz_dps = 0.85f * filt_gz_dps + 0.15f * gz_dps;

  filt_temp_c = 0.9f * filt_temp_c + 0.1f * temp_c;

  imu_ax_mg = (int16_t)filt_ax_mg;
  imu_ay_mg = (int16_t)filt_ay_mg;
  imu_az_mg = (int16_t)filt_az_mg;

  imu_gx_dps = (int16_t)filt_gx_dps;
  imu_gy_dps = (int16_t)filt_gy_dps;
  imu_gz_dps = (int16_t)filt_gz_dps;

  imu_temp_c = (int16_t)filt_temp_c;

  float accel_roll_deg = atan2f(filt_ay_mg, filt_az_mg) * 57.29578f;
  float accel_pitch_deg = atan2f(-filt_ax_mg,
                                 sqrtf(filt_ay_mg * filt_ay_mg + filt_az_mg * filt_az_mg)) * 57.29578f;

  uint32_t now = HAL_GetTick();
  float dt = (now - last_filter_tick) / 1000.0f;

  if (last_filter_tick == 0 || dt <= 0.0f || dt > 0.5f)
  {
    comp_roll_deg = accel_roll_deg;
    comp_pitch_deg = accel_pitch_deg;
  }
  else
  {
    comp_roll_deg =
        FILTER_ALPHA * (comp_roll_deg + filt_gx_dps * dt) +
        (1.0f - FILTER_ALPHA) * accel_roll_deg;

    comp_pitch_deg =
        FILTER_ALPHA * (comp_pitch_deg + filt_gy_dps * dt) +
        (1.0f - FILTER_ALPHA) * accel_pitch_deg;
  }

  last_filter_tick = now;

  imu_roll_deg = comp_roll_deg;
  imu_pitch_deg = comp_pitch_deg;

  return 1;
}

static void Send_Telemetry(void)
{
  char msg[440];
  telem_seq++;

  snprintf(msg, sizeof(msg),
      "SEQ:%lu "
      "ARM:%u "
      "MODE:%s "
      "TEST:%s "
      "TPWM:%u "
      "T1:%u T2:%u T3:%u T4:%u T5:%u T6:%u "
      "O1:%u O2:%u O3:%u O4:%u O5:%u O6:%u "
      "IMU:%u WHO:%u "
      "AX:%d AY:%d AZ:%d "
      "GX:%d GY:%d GZ:%d "
      "TP:%d "
      "R:%.1f P:%.1f "
      "BX:%.1f BY:%.1f BZ:%.1f\r\n",
      (unsigned long)telem_seq,
      armed,
      Mode_String(),
      Test_String(),
      test_pwm,
      thr_cmd[0], thr_cmd[1], thr_cmd[2], thr_cmd[3], thr_cmd[4], thr_cmd[5],
      thr_out[0], thr_out[1], thr_out[2], thr_out[3], thr_out[4], thr_out[5],
      imu_ok, imu_who,
      imu_ax_mg, imu_ay_mg, imu_az_mg,
      imu_gx_dps, imu_gy_dps, imu_gz_dps,
      imu_temp_c,
      imu_roll_deg, imu_pitch_deg,
      gyro_bias_x, gyro_bias_y, gyro_bias_z);

  Send_Line(msg);
}

static void Process_Command(char *cmd)
{
  char ack[96];

  if (strcmp(cmd, "ARM") == 0)
  {
    armed = 1;
    last_cmd_tick = HAL_GetTick();
    Send_Line("ACK:ARM\r\n");
    return;
  }

  if (strcmp(cmd, "DISARM") == 0)
  {
    armed = 0;
    Set_All_Thrusters(PWM_NEUTRAL);
    test_thruster_index = -1;
    test_pwm = PWM_NEUTRAL;
    last_cmd_tick = HAL_GetTick();
    Send_Line("ACK:DISARM\r\n");
    return;
  }

  if (strcmp(cmd, "STOP") == 0)
  {
    Set_All_Thrusters(PWM_NEUTRAL);
    test_thruster_index = -1;
    test_pwm = PWM_NEUTRAL;
    last_cmd_tick = HAL_GetTick();
    Send_Line("ACK:STOP\r\n");
    return;
  }

  if (strcmp(cmd, "TEST:STOP") == 0)
  {
    control_mode = MODE_TEST;
    test_thruster_index = -1;
    test_pwm = PWM_NEUTRAL;
    last_cmd_tick = HAL_GetTick();
    Send_Line("ACK:TEST:STOP\r\n");
    return;
  }

  if (strcmp(cmd, "MODE:MANUAL") == 0)
  {
    control_mode = MODE_MANUAL;
    test_thruster_index = -1;
    test_pwm = PWM_NEUTRAL;
    last_cmd_tick = HAL_GetTick();
    Send_Line("ACK:MODE:MANUAL\r\n");
    return;
  }

  if (strcmp(cmd, "MODE:ASSIST") == 0)
  {
    control_mode = MODE_ASSIST;
    test_thruster_index = -1;
    test_pwm = PWM_NEUTRAL;
    last_cmd_tick = HAL_GetTick();
    Send_Line("ACK:MODE:ASSIST\r\n");
    return;
  }

  if (strncmp(cmd, "TEST:T", 6) == 0)
  {
    int index = 0;
    uint32_t value = 0;

    if (sscanf(cmd, "TEST:T%d:%lu", &index, &value) == 2)
    {
      if (index >= 1 && index <= THRUSTER_COUNT)
      {
        control_mode = MODE_TEST;
        test_thruster_index = (int8_t)(index - 1);
        test_pwm = Clamp_PWM((int32_t)value);
        last_cmd_tick = HAL_GetTick();

        snprintf(ack, sizeof(ack), "ACK:TEST:T%d:%lu\r\n", index, value);
        Send_Line(ack);
        return;
      }
    }
  }

  if (strncmp(cmd, "SET:", 4) == 0)
  {
    int t1, t2, t3, t4, t5, t6;

    if (sscanf(cmd + 4, "%d,%d,%d,%d,%d,%d", &t1, &t2, &t3, &t4, &t5, &t6) == 6)
    {
      thr_cmd[0] = Clamp_PWM(t1);
      thr_cmd[1] = Clamp_PWM(t2);
      thr_cmd[2] = Clamp_PWM(t3);
      thr_cmd[3] = Clamp_PWM(t4);
      thr_cmd[4] = Clamp_PWM(t5);
      thr_cmd[5] = Clamp_PWM(t6);

      last_cmd_tick = HAL_GetTick();
      return;
    }
  }

  snprintf(ack, sizeof(ack), "ACK:BAD:%s\r\n", cmd);
  Send_Line(ack);
}
/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();

  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);

  Set_All_Thrusters(PWM_NEUTRAL);
  Apply_Thruster_Outputs();

  HAL_Delay(4000);

  imu_ok = MPU6050_Init_Minimal();
  if (imu_ok)
  {
    Send_Line("IMU:CALIBRATING\r\n");
    imu_ok = MPU6050_Calibrate_Gyro();

    if (imu_ok)
    {
      MPU6050_Read_And_Filter();
      last_filter_tick = HAL_GetTick();
      Send_Line("IMU:CAL_OK\r\n");
    }
    else
    {
      Send_Line("IMU:CAL_FAIL\r\n");
    }
  }

  Send_Line("BOOT:STUMPY_READY\r\n");

  last_telem_tick = HAL_GetTick();
  last_cmd_tick = HAL_GetTick();
  last_imu_tick = HAL_GetTick();

  char rx_buf[96];
  uint8_t rx_idx = 0;
  uint8_t ch = 0;

  while (1)
  {
    if (HAL_UART_Receive(&huart1, &ch, 1, 1) == HAL_OK)
    {
      if (ch == '\r' || ch == '\n')
      {
        if (rx_idx > 0)
        {
          rx_buf[rx_idx] = '\0';
          Process_Command(rx_buf);
          rx_idx = 0;
        }
      }
      else
      {
        if (rx_idx < sizeof(rx_buf) - 1)
        {
          rx_buf[rx_idx++] = (char)ch;
        }
        else
        {
          rx_idx = 0;
        }
      }
    }

    if (armed && ((HAL_GetTick() - last_cmd_tick) > FAILSAFE_MS))
    {
      armed = 0;
      Set_All_Thrusters(PWM_NEUTRAL);
      test_thruster_index = -1;
      test_pwm = PWM_NEUTRAL;
    }

    if (control_mode == MODE_ASSIST && (HAL_GetTick() - last_imu_tick) >= ASSIST_IMU_MS)
    {
      last_imu_tick = HAL_GetTick();
      imu_ok = MPU6050_Read_And_Filter();
    }

    Apply_Thruster_Outputs();

    if ((HAL_GetTick() - last_telem_tick) >= TELEMETRY_MS)
    {
      last_telem_tick = HAL_GetTick();

      if (control_mode != MODE_ASSIST)
      {
        imu_ok = MPU6050_Read_And_Filter();
      }

      Send_Telemetry();
      HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    }
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_TIM2_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 15;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 19999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = PWM_NEUTRAL;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_TIM_MspPostInit(&htim2);
}

static void MX_TIM3_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 15;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 19999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = PWM_NEUTRAL;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_TIM_MspPostInit(&htim3);
}

static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;

  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    HAL_Delay(100);
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif
