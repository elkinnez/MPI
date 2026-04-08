/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PWM_IZQ          TIM_CHANNEL_1   // PA8
#define PWM_DER          TIM_CHANNEL_3   // PA10
#define TIM_MOTORES      &htim1
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART3_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

uint16_t sensor_linea_izq = 0;
uint16_t sensor_linea_der = 0;
uint16_t sensor_corriente = 0;
uint32_t distancia_mm = 0;
uint32_t tiempo_ultimo_disparo = 0;

// ========== PROTOTIPOS ==========
void MoverMotores(int16_t vel_izq, int16_t vel_der);
void LeerSensores(void);
void MedirDistancia(void);

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
  MX_TIM3_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */


  HAL_TIM_IC_Start(&htim3, TIM_CHANNEL_1);


  for(int i = 0; i < 3; i++) {
      HAL_GPIO_WritePin(LED_ESTADO_GPIO_Port, LED_ESTADO_Pin, GPIO_PIN_SET);
      HAL_Delay(100);
      HAL_GPIO_WritePin(LED_ESTADO_GPIO_Port, LED_ESTADO_Pin, GPIO_PIN_RESET);
      HAL_Delay(100);
  }


  // LED frontal encendido = robot listo para combate
  HAL_GPIO_WritePin(LED_FRONTAL_GPIO_Port, LED_FRONTAL_Pin, GPIO_PIN_SET);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

	  LeerSensores();
	  MedirDistancia();

	    if(sensor_linea_izq > 3000 || sensor_linea_der > 3000) {
	      // Retroceder y girar para salvarse
	        MoverMotores(-700, 600);
	        HAL_Delay(120);
	    }
	    else if(distancia_mm < 150 && distancia_mm > 10) {
	        // Ataque frontal total
	        MoverMotores(950, 950);
	        // LED frontal parpadea rápido durante ataque
	        HAL_GPIO_TogglePin(LED_FRONTAL_GPIO_Port, LED_FRONTAL_Pin);
	        HAL_Delay(50);
	        HAL_GPIO_TogglePin(LED_FRONTAL_GPIO_Port, LED_FRONTAL_Pin);
	    }
	    else {
	        // Giro hacia la derecha }
	        MoverMotores(600, -400);
	    }


	    HAL_Delay(20);

    /* USER CODE BEGIN 3 */
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
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

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 3;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
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
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 999;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
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
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 71;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_IC_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim3, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 9600;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_ESTADO_GPIO_Port, LED_ESTADO_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED_FRONTAL_Pin|MOTOR_IZQ_DIR1_Pin|MOTOR_IZQ_DIR2_Pin|MOTOR_DER_DIR1_Pin
                          |MOTOR_DER_DIR2_Pin|TRIG_US_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED_ESTADO_Pin */
  GPIO_InitStruct.Pin = LED_ESTADO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_ESTADO_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LED_FRONTAL_Pin MOTOR_IZQ_DIR1_Pin MOTOR_IZQ_DIR2_Pin MOTOR_DER_DIR1_Pin
                           MOTOR_DER_DIR2_Pin TRIG_US_Pin */
  GPIO_InitStruct.Pin = LED_FRONTAL_Pin|MOTOR_IZQ_DIR1_Pin|MOTOR_IZQ_DIR2_Pin|MOTOR_DER_DIR1_Pin
                          |MOTOR_DER_DIR2_Pin|TRIG_US_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : ECHO_US_Pin */
  GPIO_InitStruct.Pin = ECHO_US_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(ECHO_US_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
// ===== FUNCIÓN PARA MOVER LOS MOTORES =====
void MoverMotores(int16_t vel_izq, int16_t vel_der) {
    // Limitar velocidades (rango válido: -999 a 999)
    if(vel_izq > 999) vel_izq = 999;
    if(vel_izq < -999) vel_izq = -999;
    if(vel_der > 999) vel_der = 999;
    if(vel_der < -999) vel_der = -999;

    // ----- MOTOR IZQUIERDO -----
    if(vel_izq >= 0) {
        // Adelante
        HAL_GPIO_WritePin(MOTOR_IZQ_DIR1_GPIO_Port, MOTOR_IZQ_DIR1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MOTOR_IZQ_DIR2_GPIO_Port, MOTOR_IZQ_DIR2_Pin, GPIO_PIN_RESET);
        __HAL_TIM_SET_COMPARE(TIM_MOTORES, PWM_IZQ, vel_izq);
    } else {
        // Atrás
        HAL_GPIO_WritePin(MOTOR_IZQ_DIR1_GPIO_Port, MOTOR_IZQ_DIR1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_IZQ_DIR2_GPIO_Port, MOTOR_IZQ_DIR2_Pin, GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(TIM_MOTORES, PWM_IZQ, -vel_izq);
    }

    // ----- MOTOR DERECHO -----
    if(vel_der >= 0) {
        // Adelante
        HAL_GPIO_WritePin(MOTOR_DER_DIR1_GPIO_Port, MOTOR_DER_DIR1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MOTOR_DER_DIR2_GPIO_Port, MOTOR_DER_DIR2_Pin, GPIO_PIN_RESET);
        __HAL_TIM_SET_COMPARE(TIM_MOTORES, PWM_DER, vel_der);
    } else {
        // Atrás
        HAL_GPIO_WritePin(MOTOR_DER_DIR1_GPIO_Port, MOTOR_DER_DIR1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_DER_DIR2_GPIO_Port, MOTOR_DER_DIR2_Pin, GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(TIM_MOTORES, PWM_DER, -vel_der);
    }
}

// ===== FUNCIÓN PARA LEER LOS SENSORES ANALÓGICOS (ADC) =====
void LeerSensores(void) {
    uint16_t adc_valores[3];

    HAL_ADC_Start(&hadc1);
    if(HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
        adc_valores[0] = HAL_ADC_GetValue(&hadc1);  // Canal 0 (PA0) - Línea Izq
        adc_valores[1] = HAL_ADC_GetValue(&hadc1);  // Canal 1 (PA1) - Línea Der
        adc_valores[2] = HAL_ADC_GetValue(&hadc1);  // Canal 2 (PA2) - Corriente
    }
    HAL_ADC_Stop(&hadc1);

    sensor_linea_izq = adc_valores[0];
    sensor_linea_der = adc_valores[1];
    sensor_corriente = adc_valores[2];
}

// ===== FUNCIÓN PARA MEDIR DISTANCIA CON ULTRASONIDO =====
void MedirDistancia(void) {
    uint32_t t_start, duracion;

    // Medir cada 50ms máximo (20Hz)
    if(HAL_GetTick() - tiempo_ultimo_disparo < 50) return;

    // Disparar trigger por 10 microsegundos
    HAL_GPIO_WritePin(TRIG_US_GPIO_Port, TRIG_US_Pin, GPIO_PIN_SET);
    for(int i = 0; i < 10; i++) { __NOP(); }  // Aprox 10us a 72MHz
    HAL_GPIO_WritePin(TRIG_US_GPIO_Port, TRIG_US_Pin, GPIO_PIN_RESET);

    // Esperar Echo con timeout de 100ms
    t_start = HAL_GetTick();
    while(HAL_GPIO_ReadPin(ECHO_US_GPIO_Port, ECHO_US_Pin) == GPIO_PIN_RESET) {
        if(HAL_GetTick() - t_start > 100) return;
    }

    t_start = HAL_GetTick();
    while(HAL_GPIO_ReadPin(ECHO_US_GPIO_Port, ECHO_US_Pin) == GPIO_PIN_SET) {
        if(HAL_GetTick() - t_start > 100) return;
    }
    duracion = HAL_GetTick() - t_start;

    // Convertir tiempo a distancia (mm) - Velocidad del sonido: 340 m/s
    distancia_mm = (duracion * 340) / 2;
    if(distancia_mm > 500) distancia_mm = 500;

    tiempo_ultimo_disparo = HAL_GetTick();
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
