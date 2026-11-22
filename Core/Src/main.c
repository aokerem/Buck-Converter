/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd.h"
volatile uint16_t adc_gerilim=0, dutyscale=0, adc_akim=0;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PI_CONTROL_ENABLED 1U
#define CONTROL_PERIOD_MS 1U
#define DISPLAY_SAMPLE_MS 10U
#define DISPLAY_REFRESH_MS 500U
#define DISPLAY_FILTER_ALPHA 0.125f
#define CURRENT_DISPLAY_FILTER_ALPHA 0.015625f // 1/64: stronger current smoothing
/* Measured calibration: no load = 2256 counts; 1.15 A = 2200 counts.
 * Verify at another load before relying on absolute accuracy. */
#define CURRENT_ZERO_ADC 2256.0f
#define CURRENT_COUNTS_PER_A (56.0f / 1.15f)
#define ADC_REFERENCE_V 3.3f
#define VOLTAGE_SCALE 10.3136f
#define TARGET_VOLTAGE 5.00f
#define TARGET_RAMP_V_PER_S 2.0f
#define VOLTAGE_TRIP_V 5.50f
#define PI_OUTPUT_MIN 0.0f
#define PI_OUTPUT_MAX 0.46f // 5 V test: maximum 46% duty
#define OPEN_LOOP_DUTY_COUNTS 8U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;


TIM_HandleTypeDef htim1;

/* USER CODE BEGIN PV */

volatile uint16_t present_value, akim_deger;
/* Error in volts; controller output is duty fraction, not voltage.
 * Initial bench-test gains; dynamic stability is not yet hardware-verified. */
float kp = 0.03f, ki = 0.20f;
float proportional_e = 0.0f, integrator_e = 0.0f, PI_error = 0.0f;
float error = 0.0f, ramped_target_v = 0.0f;
TIM_HandleTypeDef htim2;
volatile uint32_t control_fault = 0U;
volatile uint32_t control_max_cycles = 0U;
static uint32_t control_last_ms;
static uint8_t control_has_tick;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM1_Init(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void PWM_sinyal(void){
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, dutyscale );
}

static void Control_Stop(uint32_t fault)
{
    control_fault = fault;
    dutyscale = 0U;
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0U);
    __HAL_TIM_MOE_DISABLE(&htim1);
}

static uint16_t ADC_read_channel(uint32_t channel)
{
    ADC_ChannelConfTypeDef config = {0};
    config.Channel = channel;
    config.Rank = ADC_REGULAR_RANK_1;
    config.SingleDiff = ADC_SINGLE_ENDED;
    config.SamplingTime = ADC_SAMPLETIME_601CYCLES_5;
    config.OffsetNumber = ADC_OFFSET_NONE;
    if (HAL_ADC_ConfigChannel(&hadc1, &config) != HAL_OK) {
        Control_Stop(1U);
        return 0U;
    }
    if (HAL_ADC_Start(&hadc1) != HAL_OK) {
        Control_Stop(1U);
        return 0U;
    }
    if (HAL_ADC_PollForConversion(&hadc1, 1U) != HAL_OK) {
        HAL_ADC_Stop(&hadc1);
        Control_Stop(1U);
        return 0U;
    }
    uint16_t value = (uint16_t)HAL_ADC_GetValue(&hadc1);
    if (HAL_ADC_Stop(&hadc1) != HAL_OK) {
        Control_Stop(1U);
        return 0U;
    }
    return value;
}

void ADC_oku(void)
{
    uint32_t voltage_sum = 0U, current_sum = 0U;
    for (uint8_t i = 0; i < 16U; ++i) {
        /* One software-triggered conversion per channel: no scan overrun. */
        adc_gerilim = ADC_read_channel(ADC_CHANNEL_1); // PA0
        adc_akim = ADC_read_channel(ADC_CHANNEL_4);    // PA3
        voltage_sum += adc_gerilim;
        current_sum += adc_akim;
    }
    present_value = (uint16_t)(voltage_sum / 16U);
    akim_deger = (uint16_t)(current_sum / 16U);
}

static float PI_clamp(float value)
{
    if (value > PI_OUTPUT_MAX) return PI_OUTPUT_MAX;
    if (value < PI_OUTPUT_MIN) return PI_OUTPUT_MIN;
    return value;
}

void PID_fonk(float dt)
{
    if (dt <= 0.0f || control_fault) return;
    float measured_v = (float)present_value * ADC_REFERENCE_V / 4095.0f * VOLTAGE_SCALE;
    if (measured_v > VOLTAGE_TRIP_V) {
        Control_Stop(2U);
        return;
    }
    ramped_target_v += TARGET_RAMP_V_PER_S * dt;
    if (ramped_target_v > TARGET_VOLTAGE) ramped_target_v = TARGET_VOLTAGE;
    error = ramped_target_v - measured_v;
    proportional_e = kp * error;
    float candidate = PI_clamp(integrator_e + ki * error * dt);
    float candidate_output = proportional_e + candidate;
    if (!((candidate_output > PI_OUTPUT_MAX && error > 0.0f) ||
          (candidate_output < PI_OUTPUT_MIN && error < 0.0f))) {
        integrator_e = candidate;
    }
    PI_error = PI_clamp(proportional_e + integrator_e);
    uint32_t period_counts = __HAL_TIM_GET_AUTORELOAD(&htim1) + 1U;
    uint32_t max_counts = (uint32_t)(PI_OUTPUT_MAX * (float)period_counts + 0.5f);
    uint32_t counts = (uint32_t)(PI_error * (float)period_counts + 0.5f);
    if (counts > max_counts) counts = max_counts;
    dutyscale = (uint16_t)counts;
    PWM_sinyal();
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM2 || control_fault) return;
    uint32_t started = DWT->CYCCNT;
    uint32_t now = HAL_GetTick();
    uint32_t elapsed_ms = control_has_tick ? now - control_last_ms : CONTROL_PERIOD_MS;
    control_last_ms = now;
    control_has_tick = 1U;
    /* Do not integrate across a delayed/missed control deadline. */
    if (elapsed_ms > 2U) {
        Control_Stop(3U);
        return;
    }
    if (elapsed_ms == 0U) elapsed_ms = CONTROL_PERIOD_MS;
    /* ADC is owned exclusively by this ISR while closed loop is enabled.
     * Short average for control; the heavier LCD filtering stays in main. */
    uint32_t sum = 0U;
    for (uint8_t i = 0U; i < 4U; ++i) {
        sum += ADC_read_channel(ADC_CHANNEL_1);
        if (control_fault) return;
    }
    present_value = (uint16_t)((sum + 2U) / 4U);
    akim_deger = ADC_read_channel(ADC_CHANNEL_4);
    if (control_fault) return;
    PID_fonk((float)elapsed_ms * 0.001f);
    uint32_t cycles = DWT->CYCCNT - started;
    if (cycles > control_max_cycles) control_max_cycles = cycles;
    if (cycles > SystemCoreClock / 1250U) Control_Stop(3U); // 800 us budget
}

static void Control_Timer_Init(void)
{
    /* APB1 timer clock = 64 MHz with the current clock tree.
     * Derive the prescaler so clock-tree changes do not silently change rate. */
    uint32_t timer_clock = HAL_RCC_GetPCLK1Freq();
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != 0U) timer_clock *= 2U;
    __HAL_RCC_TIM2_CLK_ENABLE();
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = timer_clock / 1000000U - 1U;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = CONTROL_PERIOD_MS * 1000U - 1U;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK) Error_Handler();
    /* SysTick must preempt ADC polling so HAL timeouts remain bounded. */
    HAL_NVIC_SetPriority(SysTick_IRQn, 0U, 0U);
    HAL_NVIC_SetPriority(TIM2_IRQn, 1U, 0U);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */



  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
    /* Calibrate the single-ended ADC before starting conversions/PWM. */
    if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK) {
        Error_Handler();
    }


    dutyscale = PI_CONTROL_ENABLED ? 0U : OPEN_LOOP_DUTY_COUNTS;
    if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK) Error_Handler();

	// Lcd_PortType ports[] = {D4_GPIO_Port, D5_GPIO_Port, D6_GPIO_Port, D7_GPIO_Port};
	Lcd_PortType ports[] = { GPIOA, GPIOA, GPIOA, GPIOA };
	// Lcd_PinType pins[] = {D4_Pin, D5_Pin, D6_Pin, D7_Pin};
	Lcd_PinType pins[] = {GPIO_PIN_10, GPIO_PIN_7, GPIO_PIN_6, GPIO_PIN_5};
	Lcd_HandleTypeDef lcd;
	// Lcd_create(ports, pins, RS_GPIO_Port, RS_Pin, EN_GPIO_Port, EN_Pin, LCD_4_BIT_MODE);
	lcd = Lcd_create(ports, pins, GPIOB, GPIO_PIN_0, GPIOB, GPIO_PIN_1, LCD_4_BIT_MODE);

	const float adim = ADC_REFERENCE_V / 4095.0f; //adim gerilim seviyesi
	float okunangerilim = 0.0f;
    uint32_t display_sample_last = HAL_GetTick();
    uint32_t display_refresh_last = display_sample_last;
    if (PI_CONTROL_ENABLED) {
        Control_Timer_Init();
        control_last_ms = HAL_GetTick();
        if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK) Error_Handler();
    }
    float display_voltage_adc = 0.0f, display_current_adc = 0.0f;
    uint8_t display_filter_ready = 0U;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {



    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */




      uint32_t display_now = HAL_GetTick();
      if ((uint32_t)(display_now - display_sample_last) >= DISPLAY_SAMPLE_MS) {
          display_sample_last = display_now;
          if (!PI_CONTROL_ENABLED) ADC_oku();
          /* Atomic snapshot of both readings; no ADC access from main
           * while the control interrupt is running. */
          uint32_t saved_primask = __get_PRIMASK();
          __disable_irq();
          uint16_t voltage_snapshot = present_value;
          uint16_t current_snapshot = akim_deger;
          __set_PRIMASK(saved_primask);
          if (!display_filter_ready) {
              display_voltage_adc = (float)voltage_snapshot;
              display_current_adc = (float)current_snapshot;
              display_filter_ready = 1U;
          } else {
              /* Display-only EMA after the existing 16-sample average.
               * Raw values used by PI are not filtered here. */
              display_voltage_adc += DISPLAY_FILTER_ALPHA *
                                     ((float)voltage_snapshot - display_voltage_adc);
              display_current_adc += CURRENT_DISPLAY_FILTER_ALPHA *
                                     ((float)current_snapshot - display_current_adc);
          }
      }
      if (!display_filter_ready ||
          (uint32_t)(display_now - display_refresh_last) < DISPLAY_REFRESH_MS) {
          continue;
      }
      display_refresh_last = display_now;
      uint16_t display_v = (uint16_t)(display_voltage_adc + 0.5f);
      okunangerilim = display_voltage_adc * adim * VOLTAGE_SCALE;
      unsigned voltage_hundredths = (unsigned)(okunangerilim * 100.0f + 0.5f);
      float current_a = (CURRENT_ZERO_ADC - display_current_adc) /
                        CURRENT_COUNTS_PER_A;
      char current_sign = current_a < 0.0f ? '-' : ' ';
      float current_magnitude = current_a < 0.0f ? -current_a : current_a;
      unsigned current_hundredths = (unsigned)(current_magnitude * 100.0f + 0.5f);

      /* Fit all calibration fields on the 16x2 display. U: volts, I: amps. */
      char top_line[17], bottom_line[17];
      snprintf(top_line, sizeof top_line, "D:%2u  AD_V:%4u ",
               (unsigned)dutyscale % 100U, (unsigned)display_v % 4096U);
      snprintf(bottom_line, sizeof bottom_line, "U:%2u.%02u I:%c%u.%02u",
               (voltage_hundredths / 100U) % 100U, voltage_hundredths % 100U,
               current_sign, (current_hundredths / 100U) % 100U,
               current_hundredths % 100U);
      /* Clear leftover digits without clearing/flashing the entire LCD. */
      for (size_t i = strlen(bottom_line); i < 16U; ++i) bottom_line[i] = ' ';
      bottom_line[16] = '\0';
      if (control_fault) {
          snprintf(top_line, sizeof top_line, "FAULT:%lu PWM OFF", (unsigned long)control_fault);
      }
      Lcd_cursor(&lcd, 0, 0);
      Lcd_string(&lcd, top_line);
      Lcd_cursor(&lcd, 1, 0);
      Lcd_string(&lcd, bottom_line);

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_TIM1|RCC_PERIPHCLK_ADC12;
  PeriphClkInit.Adc12ClockSelection = RCC_ADC12PLLCLK_DIV1;
  PeriphClkInit.Tim1ClockSelection = RCC_TIM1CLK_HCLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.SamplingTime = ADC_SAMPLETIME_601CYCLES_5;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 16-1;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 99; // 100 timer counts: 40 kHz PWM
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = PI_CONTROL_ENABLED ? 0U : OPEN_LOOP_DUTY_COUNTS;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_ENABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
  sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
  sBreakDeadTimeConfig.Break2Filter = 0;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, lcd_d5_Pin|lcd_d6_Pin|lcd_d7_Pin|lcd_d4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, lcd_rs_Pin|lcd_en_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : lcd_d5_Pin lcd_d6_Pin lcd_d7_Pin lcd_d4_Pin */
  GPIO_InitStruct.Pin = lcd_d5_Pin|lcd_d6_Pin|lcd_d7_Pin|lcd_d4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : lcd_rs_Pin lcd_en_Pin */
  GPIO_InitStruct.Pin = lcd_rs_Pin|lcd_en_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  if (htim1.Instance == TIM1) Control_Stop(4U);
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
