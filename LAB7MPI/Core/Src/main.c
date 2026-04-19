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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
    MODE_AUTONOMOUS = 0,
    MODE_DEBUG
} SystemMode_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define CMD_BUFFER_SIZE     64
#define UART_RX_SIZE        1
#define LOG_PERIOD_MS       1000
#define DEBOUNCE_TIME_MS    50
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
volatile uint8_t uartRxData[UART_RX_SIZE];
volatile uint8_t cmdReady = 0;

char cmdBuffer[CMD_BUFFER_SIZE];
uint8_t cmdIndex = 0;

SystemMode_t currentMode = MODE_AUTONOMOUS;

uint8_t logEnabled = 0;
uint32_t lastLogTick = 0;

GPIO_PinState lastButtonReading = GPIO_PIN_SET;
GPIO_PinState stableButtonState = GPIO_PIN_SET;
uint32_t lastDebounceTime = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
void UART_SendString(const char *str);
void UART_SendPrompt(void);
void UART_StartReception(void);
void ProcessCommand(char *cmd);
void NormalizeCommand(char *cmd);
void CheckModeButton(void);
void EnterAutonomousMode(void);
void EnterDebugMode(void);
void AutonomousTask(void);
uint32_t ReadADCValue(void);
void SendSensorData(void);
void SetPWMDuty(uint8_t duty);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_TIM2_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
  SetPWMDuty(0);

  UART_SendString("System Ready\r\n");
  UART_SendString("Mode: AUTONOMOUS\r\n");

  UART_StartReception();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    CheckModeButton();

    if (currentMode == MODE_AUTONOMOUS)
    {
        AutonomousTask();
    }
    else
    {
        if (cmdReady)
        {
            NormalizeCommand(cmdBuffer);
            ProcessCommand(cmdBuffer);
            cmdReady = 0;
            UART_SendPrompt();
        }

        if (logEnabled && (HAL_GetTick() - lastLogTick >= LOG_PERIOD_MS))
        {
            SendSensorData();
            lastLogTick = HAL_GetTick();
        }
    }
    /* USER CODE END WHILE */

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
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 7199;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
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
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : BOTON_Pin */
  GPIO_InitStruct.Pin = BOTON_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BOTON_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void UART_StartReception(void)
{
    HAL_UART_Receive_IT(&huart2, (uint8_t *)uartRxData, UART_RX_SIZE);
}

void UART_SendString(const char *str)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)str, strlen(str), HAL_MAX_DELAY);
}

void UART_SendPrompt(void)
{
    UART_SendString("> ");
}

void NormalizeCommand(char *cmd)
{
    uint16_t i = 0;
    while (cmd[i] != '\0')
    {
        cmd[i] = toupper((unsigned char)cmd[i]);
        i++;
    }
}

void EnterAutonomousMode(void)
{
    currentMode = MODE_AUTONOMOUS;
    logEnabled = 0;
    UART_SendString("\r\nMode: AUTONOMOUS\r\n");
}

void EnterDebugMode(void)
{
    currentMode = MODE_DEBUG;
    UART_SendString("\r\nMode: DEBUG\r\n");
    UART_SendString("Available commands:\r\n");
    UART_SendString("READ\r\n");
    UART_SendString("LED ON\r\n");
    UART_SendString("LED OFF\r\n");
    UART_SendString("PWM X\r\n");
    UART_SendString("LOG ON\r\n");
    UART_SendString("LOG OFF\r\n");
    UART_SendPrompt();
}

void CheckModeButton(void)
{
    GPIO_PinState reading = HAL_GPIO_ReadPin(BOTON_GPIO_Port, BOTON_Pin);

    if (reading != lastButtonReading)
    {
        lastDebounceTime = HAL_GetTick();
    }

    if ((HAL_GetTick() - lastDebounceTime) > DEBOUNCE_TIME_MS)
    {
        if (reading != stableButtonState)
        {
            stableButtonState = reading;

            if (stableButtonState == GPIO_PIN_RESET)
            {
                if (currentMode == MODE_AUTONOMOUS)
                {
                    EnterDebugMode();
                }
                else
                {
                    EnterAutonomousMode();
                }
            }
        }
    }

    lastButtonReading = reading;
}

void AutonomousTask(void)
{
    static uint32_t lastBlink = 0;

    if (HAL_GetTick() - lastBlink >= 500)
    {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        lastBlink = HAL_GetTick();
    }
}

uint32_t ReadADCValue(void)
{
    uint32_t value = 0;

    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY) == HAL_OK)
    {
        value = HAL_ADC_GetValue(&hadc1);
    }
    HAL_ADC_Stop(&hadc1);

    return value;
}

void SendSensorData(void)
{
    char msg[100];
    uint32_t adcValue = ReadADCValue();

    float temperature = 24.3f;
    float light = (adcValue * 100.0f) / 4095.0f;
    float potentiometer = (adcValue * 100.0f) / 4095.0f;

    snprintf(msg, sizeof(msg), "T:%.1f L:%.0f P:%.0f\r\n", temperature, light, potentiometer);
    UART_SendString(msg);
}

void SetPWMDuty(uint8_t duty)
{
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim2);
    uint32_t compare = ((arr + 1) * duty) / 100;
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, compare);
}

void ProcessCommand(char *cmd)
{
    if (strcmp(cmd, "READ") == 0)
    {
        SendSensorData();
    }
    else if (strcmp(cmd, "LED ON") == 0)
    {
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
        UART_SendString("OK\r\n");
    }
    else if (strcmp(cmd, "LED OFF") == 0)
    {
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
        UART_SendString("OK\r\n");
    }
    else if (strncmp(cmd, "PWM ", 4) == 0)
    {
        int duty = atoi(&cmd[4]);

        if (duty >= 0 && duty <= 100)
        {
            SetPWMDuty((uint8_t)duty);
            UART_SendString("OK\r\n");
        }
        else
        {
            UART_SendString("ERROR: PWM range 0-100\r\n");
        }
    }
    else if (strcmp(cmd, "LOG ON") == 0)
    {
        logEnabled = 1;
        UART_SendString("OK\r\n");
    }
    else if (strcmp(cmd, "LOG OFF") == 0)
    {
        logEnabled = 0;
        UART_SendString("OK\r\n");
    }
    else
    {
        UART_SendString("ERROR: Unknown command\r\n");
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    uint8_t rx = uartRxData[0];

    if (huart->Instance == USART2)
    {
        HAL_UART_Transmit(&huart2, &rx, 1, HAL_MAX_DELAY);

        if (rx == '\r' || rx == '\n')
        {
            if (cmdIndex > 0)
            {
                cmdBuffer[cmdIndex] = '\0';
                cmdReady = 1;
                cmdIndex = 0;
                UART_SendString("\r\n");
            }
        }
        else
        {
            if (cmdIndex < (CMD_BUFFER_SIZE - 1))
            {
                cmdBuffer[cmdIndex++] = rx;
            }
            else
            {
                cmdIndex = 0;
                UART_SendString("\r\nERROR: Buffer overflow\r\n");
                UART_SendPrompt();
            }
        }

        UART_StartReception();
    }
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
