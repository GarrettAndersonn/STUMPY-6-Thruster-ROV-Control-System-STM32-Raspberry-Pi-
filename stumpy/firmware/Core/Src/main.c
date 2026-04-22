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

#define TELEMETRY_MS       200
#define FAILSAFE_MS        1200

#define MPU6050_ADDR             (0x68 << 1)
#define MPU6050_REG_PWR_MGMT_1   0x6B
#define MPU6050_REG_WHO_AM_I     0x75
#define MPU6050_REG_ACCEL_XOUT_H 0x3B

static uint8_t armed = 0;

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

static uint8_t imu_ok = 0;
static uint8_t imu_who = 0;

static int16_t imu_ax_mg = 0;
static int16_t imu_ay_mg = 0;
static int16_t imu_az_mg = 0;
static int16_t imu_gx_dps = 0;
static int16_t imu_gy_dps = 0;
static int16_t imu_gz_dps = 0;
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
static void Set_All_Thrusters(uint16_t value);
static void Apply_Thruster_Outputs(void);
static void Send_Line(const char *text);
static void Send_Telemetry(void);
static void Process_Command(char *cmd);
static uint8_t MPU6050_Init_Minimal(void);
static uint8_t MPU6050_Read_Minimal(void);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
static uint16_t Clamp_PWM(int32_t value)
{
  if (value < PWM_MIN) return PWM_MIN;
  if (value > PWM_MAX) return PWM_MAX;
  return (uint16_t)value;
}

static void Set_All_Thrusters(uint16_t value)
{
  uint16_t v = Clamp_PWM(value);
  for (int i = 0; i < THRUSTER_COUNT; i++)
  {
    thr_cmd[i] = v;
  }
}

static void Apply_Thruster_Outputs(void)
{
  if (armed)
  {
    for (int i = 0; i < THRUSTER_COUNT; i++)
    {
      thr_out[i] = Clamp_PWM(thr_cmd[i]);
    }
  }
  else
  {
    for (int i = 0; i < THRUSTER_COUNT; i++)
    {
      thr_out[i] = PWM_NEUTRAL;
    }
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

static uint8_t MPU6050_Read_Minimal(void)
{
  uint8_t buf[14];

  if (HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, MPU6050_REG_ACCEL_XOUT_H, 1, buf, 14, 100) != HAL_OK)
  {
    return 0;
  }

  int16_t ax = (int16_t)((buf[0] << 8) | buf[1]);
  int16_t ay = (int16_t)((buf[2] << 8) | buf[3]);
  int16_t az = (int16_t)((buf[4] << 8) | buf[5]);
  int16_t gx = (int16_t)((buf[8] << 8) | buf[9]);
  int16_t gy = (int16_t)((buf[10] << 8) | buf[11]);
  int16_t gz = (int16_t)((buf[12] << 8) | buf[13]);

  imu_ax_mg = (int16_t)((ax * 1000L) / 16384L);
  imu_ay_mg = (int16_t)((ay * 1000L) / 16384L);
  imu_az_mg = (int16_t)((az * 1000L) / 16384L);

  imu_gx_dps = (int16_t)(gx / 131);
  imu_gy_dps = (int16_t)(gy / 131);
  imu_gz_dps = (int16_t)(gz / 131);

  return 1;
}

static void Send_Telemetry(void)
{
  char msg[320];

  imu_ok = MPU6050_Read_Minimal();

  snprintf(msg, sizeof(msg),
      "ARM:%u "
      "T1:%u O1:%u "
      "T2:%u O2:%u "
      "T3:%u O3:%u "
      "T4:%u O4:%u "
      "T5:%u O5:%u "
      "T6:%u O6:%u "
      "IMU:%u AXmg:%d AYmg:%d AZmg:%d GXdps:%d GYdps:%d GZdps:%d\r\n",
      armed,
      thr_cmd[0], thr_out[0],
      thr_cmd[1], thr_out[1],
      thr_cmd[2], thr_out[2],
      thr_cmd[3], thr_out[3],
      thr_cmd[4], thr_out[4],
      thr_cmd[5], thr_out[5],
      imu_ok,
      imu_ax_mg, imu_ay_mg, imu_az_mg,
      imu_gx_dps, imu_gy_dps, imu_gz_dps);

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
    last_cmd_tick = HAL_GetTick();
    Send_Line("ACK:DISARM\r\n");
    return;
  }

  if (strcmp(cmd, "STOP") == 0)
  {
    Set_All_Thrusters(PWM_NEUTRAL);
    last_cmd_tick = HAL_GetTick();
    Send_Line("ACK:STOP\r\n");
    return;
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
      Send_Line("ACK:SET\r\n");
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

  Send_Line("BOOT:STUMPY_READY\r\n");

  last_telem_tick = HAL_GetTick();
  last_cmd_tick = HAL_GetTick();

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
    }

    Apply_Thruster_Outputs();

    if ((HAL_GetTick() - last_telem_tick) >= TELEMETRY_MS)
    {
      last_telem_tick = HAL_GetTick();
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

/**
  * @brief I2C1 Initialization Function
  * @retval None
  */
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

/**
  * @brief TIM2 Initialization Function
  * @retval None
  */
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

/**
  * @brief TIM3 Initialization Function
  * @retval None
  */
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

/**
  * @brief USART1 Initialization Function
  * @retval None
  */
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

/**
  * @brief GPIO Initialization Function
  * @retval None
  */
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

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    HAL_Delay(100);
  }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif
