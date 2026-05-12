/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdio.h>
#include <string.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// Canales PWM de TIM2
#define PWM_IZQ_CH        TIM_CHANNEL_1   // PA0 -> MOTOR_L_PWM
#define PWM_DER_CH        TIM_CHANNEL_2   // PA1 -> MOTOR_R_PWM
#define TIM_MOTORES       &htim2

// Factor de conversión para ultrasonido (mm)
#define US_FACTOR_MM      0.1715f

// Umbral para detectar línea blanca (ajustar según pruebas)
// En un dohyo típico, negro ~ 500-1000, blanco > 2500 (escala 0-4095)
#define UMBRAL_LINEA      2500
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
// Variables para ultrasonido (interrupción)
volatile uint32_t IC_Val1 = 0;
volatile uint32_t IC_Val2 = 0;
volatile uint8_t  Is_First_Captured = 0;
volatile uint8_t  Distance_Ready = 0;
volatile float    distancia_mm = 0.0f;
uint32_t tiempo_ultimo_disparo = 0;

// Buffer para los 4 sensores de línea (llenado por DMA circular)
uint16_t adc_linea[4] = {0};

// Variables para lectura actual de línea
uint16_t linea_fl = 0, linea_fr = 0, linea_rl = 0, linea_rr = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM2_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
void MoverMotores(int16_t vel_izq, int16_t vel_der);
void DispararUltrasonido(void);
void LeerSensoresLinea(void);
uint8_t BordeDetectado(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief  Control de motores con TB6612FNG
 */
void MoverMotores(int16_t vel_izq, int16_t vel_der)
{
    if(vel_izq > 3599) vel_izq = 3599;
    if(vel_izq < -3599) vel_izq = -3599;
    if(vel_der > 3599) vel_der = 3599;
    if(vel_der < -3599) vel_der = -3599;

    // Motor izquierdo
    if(vel_izq >= 0) {
        HAL_GPIO_WritePin(MOTOR_L_IN1_GPIO_Port, MOTOR_L_IN1_Pin, GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(TIM_MOTORES, PWM_IZQ_CH, vel_izq);
    } else {
        HAL_GPIO_WritePin(MOTOR_L_IN1_GPIO_Port, MOTOR_L_IN1_Pin, GPIO_PIN_RESET);
        __HAL_TIM_SET_COMPARE(TIM_MOTORES, PWM_IZQ_CH, -vel_izq);
    }

    // Motor derecho
    if(vel_der >= 0) {
        HAL_GPIO_WritePin(MOTOR_R_IN1_GPIO_Port, MOTOR_R_IN1_Pin, GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(TIM_MOTORES, PWM_DER_CH, vel_der);
    } else {
        HAL_GPIO_WritePin(MOTOR_R_IN1_GPIO_Port, MOTOR_R_IN1_Pin, GPIO_PIN_RESET);
        __HAL_TIM_SET_COMPARE(TIM_MOTORES, PWM_DER_CH, -vel_der);
    }
}

/**
 * @brief  Pulso de 10 us en TRIG_US (ahora en PA8)
 */
void DispararUltrasonido(void)
{
    HAL_GPIO_WritePin(TRIG_US_GPIO_Port, TRIG_US_Pin, GPIO_PIN_SET);
    for(volatile int i = 0; i < 72; i++) { __NOP(); }
    HAL_GPIO_WritePin(TRIG_US_GPIO_Port, TRIG_US_Pin, GPIO_PIN_RESET);
}

/**
 * @brief  Callback de TIM3: mide ancho de pulso ECHO
 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM3 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4)
    {
        if(Is_First_Captured == 0)
        {
            IC_Val1 = HAL_TIM_ReadCapturedValue(&htim3, TIM_CHANNEL_4);
            Is_First_Captured = 1;
            __HAL_TIM_SET_CAPTUREPOLARITY(&htim3, TIM_CHANNEL_4, TIM_INPUTCHANNELPOLARITY_FALLING);
        }
        else
        {
            IC_Val2 = HAL_TIM_ReadCapturedValue(&htim3, TIM_CHANNEL_4);
            Is_First_Captured = 0;
            __HAL_TIM_SET_CAPTUREPOLARITY(&htim3, TIM_CHANNEL_4, TIM_INPUTCHANNELPOLARITY_RISING);

            uint32_t pulso_us;
            if(IC_Val2 > IC_Val1)
                pulso_us = IC_Val2 - IC_Val1;
            else
                pulso_us = (0xFFFF - IC_Val1) + IC_Val2 + 1;

            if(pulso_us > 0 && pulso_us < 38000)
                distancia_mm = pulso_us * US_FACTOR_MM;
            else
                distancia_mm = 4000.0f;

            Distance_Ready = 1;
        }
    }
}

/**
 * @brief  Copia los valores del buffer DMA a variables locales
 */
void LeerSensoresLinea(void)
{
    // El orden del buffer corresponde al orden configurado en la secuencia ADC:
    // Rank1: LINEA_FL (PB0) -> adc_linea[0]
    // Rank2: LINEA_FR (PA5) -> adc_linea[1]
    // Rank3: LINEA_RL (PA6) -> adc_linea[2]
    // Rank4: LINEA_RR (PA7) -> adc_linea[3]
    linea_fl = adc_linea[0];
    linea_fr = adc_linea[1];
    linea_rl = adc_linea[2];
    linea_rr = adc_linea[3];
}

/**
 * @brief  Retorna 1 si algún sensor ve línea blanca (Valor > UMBRAL)
 */
uint8_t BordeDetectado(void)
{
    LeerSensoresLinea();
    if(linea_fl > UMBRAL_LINEA || linea_fr > UMBRAL_LINEA ||
       linea_rl > UMBRAL_LINEA || linea_rr > UMBRAL_LINEA)
        return 1;
    return 0;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  */
int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM3_Init();
  MX_TIM2_Init();
  MX_ADC1_Init();
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */
  // Arrancar PWM de motores
  HAL_TIM_PWM_Start(TIM_MOTORES, PWM_IZQ_CH);
  HAL_TIM_PWM_Start(TIM_MOTORES, PWM_DER_CH);

  // Habilitar driver (STBY = HIGH)
  HAL_GPIO_WritePin(MOTOR_STBY_GPIO_Port, MOTOR_STBY_Pin, GPIO_PIN_SET);

  // Iniciar captura de ultrasonido (interrupción)
  HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_4);

  // Iniciar ADC en modo DMA circular (llenará continuamente adc_linea[])
  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_linea, 4);

  // Parpadeo de arranque
  for(int i = 0; i < 3; i++)
  {
      HAL_GPIO_TogglePin(LED_ESTADO_GPIO_Port, LED_ESTADO_Pin);
      HAL_Delay(150);
  }
  HAL_GPIO_WritePin(LED_ESTADO_GPIO_Port, LED_ESTADO_Pin, GPIO_PIN_SET); // apagado

  char msg[60];
  sprintf(msg, "SumoBot - Sensores OK\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
  /* USER CODE END 2 */

  /* Infinite loop */
  while (1)
  {
      // Disparar ultrasonido cada 200 ms
      if(HAL_GetTick() - tiempo_ultimo_disparo >= 200)
      {
          DispararUltrasonido();
          tiempo_ultimo_disparo = HAL_GetTick();
      }

      // Verificar borde ANTES de cualquier movimiento (prioridad máxima)
      if(BordeDetectado())
      {
          // Maniobra de evasión: retroceder y girar
          MoverMotores(-2000, -2000);  // retroceder
          HAL_Delay(300);
          MoverMotores(-2000, 2000);   // girar 180° aproximadamente
          HAL_Delay(600);
          HAL_GPIO_WritePin(LED_ESTADO_GPIO_Port, LED_ESTADO_Pin, GPIO_PIN_RESET); // LED ON
          sprintf(msg, "BORDE! Evadiendo\r\n");
          HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
          continue; // saltar ciclo para evitar otra acción
      }

      // Si hay nueva distancia del ultrasónico
      if(Distance_Ready == 1)
      {
          Distance_Ready = 0;

          if(distancia_mm < 100 && distancia_mm > 0)  // menos de 10 cm → parar
          {
              MoverMotores(0, 0);   // detenerse
              HAL_GPIO_WritePin(LED_ESTADO_GPIO_Port, LED_ESTADO_Pin, GPIO_PIN_SET); // LED OFF
              sprintf(msg, "FRENO! Dist=%.0f mm\r\n", distancia_mm);
          }
          else if(distancia_mm < 300)  // entre 10 y 30 cm → atacar
          {
              MoverMotores(2500, 2500);   // avanzar rápido
              HAL_GPIO_WritePin(LED_ESTADO_GPIO_Port, LED_ESTADO_Pin, GPIO_PIN_RESET); // LED ON
              sprintf(msg, "ATACANDO! Dist=%.0f mm\r\n", distancia_mm);
          }
          else  // no hay oponente cerca → buscar
          {
              MoverMotores(-1500, 1500);  // giro a la derecha
              HAL_GPIO_WritePin(LED_ESTADO_GPIO_Port, LED_ESTADO_Pin, GPIO_PIN_SET); // LED OFF
              sprintf(msg, "BUSCANDO... Dist=%.0f mm\r\n", distancia_mm);
          }
          HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
      }

      HAL_Delay(10);
  }
}

/* Funciones de inicialización (se generan automáticamente, no modificar) */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) Error_Handler();
}

/* Las funciones MX_TIM2_Init, MX_TIM3_Init, MX_USART1_UART_Init se dejan igual */
static void MX_TIM2_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 3599;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK) Error_Handler();
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK) Error_Handler();
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK) Error_Handler();
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK) Error_Handler();
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) Error_Handler();
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK) Error_Handler();
  HAL_TIM_MspPostInit(&htim2);
}

static void MX_TIM3_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 71;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK) Error_Handler();
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK) Error_Handler();
  if (HAL_TIM_IC_Init(&htim3) != HAL_OK) Error_Handler();
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK) Error_Handler();
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 4;
  if (HAL_TIM_IC_ConfigChannel(&htim3, &sConfigIC, TIM_CHANNEL_4) != HAL_OK) Error_Handler();
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
  if (HAL_UART_Init(&huart1) != HAL_OK) Error_Handler();
}

static void MX_ADC1_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 4;
  if (HAL_ADC_Init(&hadc1) != HAL_OK) Error_Handler();

  sConfig.Channel = ADC_CHANNEL_8;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) Error_Handler();

  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) Error_Handler();

  sConfig.Channel = ADC_CHANNEL_6;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) Error_Handler();

  sConfig.Channel = ADC_CHANNEL_7;
  sConfig.Rank = ADC_REGULAR_RANK_4;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) Error_Handler();
}

static void MX_DMA_Init(void)
{
  __HAL_RCC_DMA1_CLK_ENABLE();
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOA, MOTOR_L_IN1_Pin|MOTOR_R_IN1_Pin|MOTOR_STBY_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(TRIG_US_GPIO_Port, TRIG_US_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_ESTADO_GPIO_Port, LED_ESTADO_Pin, GPIO_PIN_SET);

  GPIO_InitStruct.Pin = MOTOR_L_IN1_Pin|MOTOR_R_IN1_Pin|MOTOR_STBY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = TRIG_US_Pin;
  HAL_GPIO_Init(TRIG_US_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LED_ESTADO_Pin;
  HAL_GPIO_Init(LED_ESTADO_GPIO_Port, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif
