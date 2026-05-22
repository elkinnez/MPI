/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Control de motores vía Bluetooth (HC-05) con PWM
  *                   USART1, TIM2 (Period=999), pines TB6612.
  *                   Convierte comandos a mayúsculas y tolera CR/LF.
  *
  * CORRECCIONES APLICADAS:
  *   1. Incluye <string.h>
  *   2. Bucle principal con while(1) corregido
  *   3. Macros de pines unificadas en MX_GPIO_Init()
  ******************************************************************************
  */
/* USER CODE END Header */
#include "main.h"
#include <string.h>          // <-- AÑADIDO para strlen()

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PWM_MAX            999

#define PWM_IZQ_CH         TIM_CHANNEL_1   // PA0
#define PWM_DER_CH         TIM_CHANNEL_2   // PA1
#define TIM_MOTORES        &htim2

#define VEL_ADELANTE       850
#define VEL_ATRAS          850
#define VEL_GIRO           800

// Motor izquierdo (AIN1, AIN2)
#define MOTOR_L_IN1_Pin        GPIO_PIN_12
#define MOTOR_L_IN1_GPIO_Port  GPIOB
#define MOTOR_L_IN2_Pin        GPIO_PIN_13
#define MOTOR_L_IN2_GPIO_Port  GPIOB

// Motor derecho (BIN1, BIN2)
#define MOTOR_R_IN1_Pin        GPIO_PIN_8
#define MOTOR_R_IN1_GPIO_Port  GPIOB
#define MOTOR_R_IN2_Pin        GPIO_PIN_9
#define MOTOR_R_IN2_GPIO_Port  GPIOB

// STBY (habilita el driver TB6612)
#define MOTOR_STBY_Pin         GPIO_PIN_15
#define MOTOR_STBY_GPIO_Port   GPIOB

// LED de estado en PC13 (activo bajo)
#define LED_ESTADO_Pin         GPIO_PIN_13
#define LED_ESTADO_GPIO_Port   GPIOC
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
volatile uint8_t led_blink_flag = 0;
uint32_t         led_timer      = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
void MoverMotores(int16_t vel_izq, int16_t vel_der);
void PruebaMotores(void);
void ProcesarComando(uint8_t cmd);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief Control de motores con TB6612FNG.
 *        vel > 0 → adelante | vel < 0 → atrás | vel = 0 → freno
 */
void MoverMotores(int16_t vel_izq, int16_t vel_der)
{
    // Saturación
    if(vel_izq >  PWM_MAX) vel_izq =  PWM_MAX;
    if(vel_izq < -PWM_MAX) vel_izq = -PWM_MAX;
    if(vel_der >  PWM_MAX) vel_der =  PWM_MAX;
    if(vel_der < -PWM_MAX) vel_der = -PWM_MAX;

    /* ── MOTOR IZQUIERDO ── */
    if(vel_izq >= 0)
    {
        HAL_GPIO_WritePin(MOTOR_L_IN1_GPIO_Port, MOTOR_L_IN1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MOTOR_L_IN2_GPIO_Port, MOTOR_L_IN2_Pin, GPIO_PIN_RESET);
        __HAL_TIM_SET_COMPARE(TIM_MOTORES, PWM_IZQ_CH, vel_izq);
    }
    else
    {
        HAL_GPIO_WritePin(MOTOR_L_IN1_GPIO_Port, MOTOR_L_IN1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_L_IN2_GPIO_Port, MOTOR_L_IN2_Pin, GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(TIM_MOTORES, PWM_IZQ_CH, -vel_izq);
    }

    /* ── MOTOR DERECHO ── */
    if(vel_der >= 0)
    {
        HAL_GPIO_WritePin(MOTOR_R_IN1_GPIO_Port, MOTOR_R_IN1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MOTOR_R_IN2_GPIO_Port, MOTOR_R_IN2_Pin, GPIO_PIN_RESET);
        __HAL_TIM_SET_COMPARE(TIM_MOTORES, PWM_DER_CH, vel_der);
    }
    else
    {
        HAL_GPIO_WritePin(MOTOR_R_IN1_GPIO_Port, MOTOR_R_IN1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_R_IN2_GPIO_Port, MOTOR_R_IN2_Pin, GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(TIM_MOTORES, PWM_DER_CH, -vel_der);
    }
}

/**
 * @brief Prueba automática al encender: verifica que ambos motores respondan.
 */
void PruebaMotores(void)
{
    // 5 parpadeos rápidos = inicio de prueba
    for(int i = 0; i < 5; i++)
    {
        HAL_GPIO_WritePin(LED_ESTADO_GPIO_Port, LED_ESTADO_Pin, GPIO_PIN_RESET);
        HAL_Delay(100);
        HAL_GPIO_WritePin(LED_ESTADO_GPIO_Port, LED_ESTADO_Pin, GPIO_PIN_SET);
        HAL_Delay(100);
    }

    MoverMotores( 850,  850); HAL_Delay(1500); // Adelante
    MoverMotores(   0,    0); HAL_Delay(500);
    MoverMotores(-850, -850); HAL_Delay(1500); // Atrás
    MoverMotores(   0,    0); HAL_Delay(500);
    MoverMotores(-800,  800); HAL_Delay(1200); // Giro izquierda
    MoverMotores(   0,    0); HAL_Delay(500);
    MoverMotores( 800, -800); HAL_Delay(1200); // Giro derecha
    MoverMotores(   0,    0); HAL_Delay(500);

    // 3 parpadeos = fin de prueba
    for(int i = 0; i < 3; i++)
    {
        HAL_GPIO_WritePin(LED_ESTADO_GPIO_Port, LED_ESTADO_Pin, GPIO_PIN_RESET);
        HAL_Delay(200);
        HAL_GPIO_WritePin(LED_ESTADO_GPIO_Port, LED_ESTADO_Pin, GPIO_PIN_SET);
        HAL_Delay(200);
    }
}

/**
 * @brief Procesa un carácter recibido por Bluetooth.
 *        Acepta minúsculas y tolera CR/LF.
 */
void ProcesarComando(uint8_t cmd)
{
    // Ignorar retorno de carro y salto de línea
    if(cmd == '\r' || cmd == '\n') return;

    // Convertir a mayúscula
    if(cmd >= 'a' && cmd <= 'z') cmd = cmd - 32;

    // Eco al celular
    uint8_t echo[3] = { cmd, '\r', '\n' };
    HAL_UART_Transmit(&huart1, echo, 3, 100);

    // Ejecutar movimiento
    switch(cmd)
    {
        case 'W': MoverMotores( VEL_ADELANTE,  VEL_ADELANTE); break;
        case 'S': MoverMotores(-VEL_ATRAS,    -VEL_ATRAS);    break;
        case 'A': MoverMotores(-VEL_GIRO,      VEL_GIRO);     break;
        case 'D': MoverMotores( VEL_GIRO,     -VEL_GIRO);     break;
        default:  MoverMotores(0, 0);                          break;
    }

    // Encender LED y programar apagado en el bucle principal
    HAL_GPIO_WritePin(LED_ESTADO_GPIO_Port, LED_ESTADO_Pin, GPIO_PIN_RESET);
    led_blink_flag = 1;
    led_timer = HAL_GetTick();
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
    MX_TIM2_Init();
    MX_USART1_UART_Init();

    /* USER CODE BEGIN 2 */
    // Iniciar canales PWM de TIM2
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);   // PA0 — motor izquierdo
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);   // PA1 — motor derecho

    // Activar driver TB6612 (STBY = HIGH)
    HAL_GPIO_WritePin(MOTOR_STBY_GPIO_Port, MOTOR_STBY_Pin, GPIO_PIN_SET);

    // Motores detenidos al arrancar
    MoverMotores(0, 0);

    // LED apagado (activo bajo → SET = apagado)
    HAL_GPIO_WritePin(LED_ESTADO_GPIO_Port, LED_ESTADO_Pin, GPIO_PIN_SET);

    // Mensaje de inicio por UART
    char msg[] = "Robot Bluetooth HC05 listo\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);

    // Prueba automática (comenta esta línea si no la necesitas)
    PruebaMotores();

    /* USER CODE END 2 */

    /* Infinite loop */
    while (1)
    {
        /* USER CODE BEGIN WHILE */
        // Recibir 1 byte por Bluetooth (timeout 100 ms, no bloqueante largo)
        uint8_t data;
        if(HAL_UART_Receive(&huart1, &data, 1, 100) == HAL_OK)
        {
            ProcesarComando(data);
        }

        // Apagar LED 100 ms después de que se encendió
        if(led_blink_flag)
        {
            if((HAL_GetTick() - led_timer) >= 100)
            {
                HAL_GPIO_WritePin(LED_ESTADO_GPIO_Port, LED_ESTADO_Pin, GPIO_PIN_SET);
                led_blink_flag = 0;
            }
        }
        /* USER CODE END WHILE */
    }
    /* USER CODE BEGIN 3 */
    /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState       = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;
    if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) Error_Handler();
}

/**
  * @brief TIM2 Initialization (1 kHz PWM)
  */
static void MX_TIM2_Init(void)
{
    TIM_ClockConfigTypeDef  sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig      = {0};
    TIM_OC_InitTypeDef      sConfigOC          = {0};

    htim2.Instance               = TIM2;
    htim2.Init.Prescaler         = 71;
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = 999;
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if(HAL_TIM_Base_Init(&htim2) != HAL_OK) Error_Handler();

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if(HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK) Error_Handler();

    if(HAL_TIM_PWM_Init(&htim2) != HAL_OK) Error_Handler();

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    if(HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK) Error_Handler();

    sConfigOC.OCMode     = TIM_OCMODE_PWM1;
    sConfigOC.Pulse      = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if(HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) Error_Handler();
    if(HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK) Error_Handler();

    HAL_TIM_MspPostInit(&htim2);
}

/**
  * @brief USART1 Initialization (9600 baud)
  */
static void MX_USART1_UART_Init(void)
{
    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = 9600;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if(HAL_UART_Init(&huart1) != HAL_OK) Error_Handler();
}

/**
  * @brief GPIO Initialization
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Configurar pines de motores y STBY */
    HAL_GPIO_WritePin(GPIOB,
                      MOTOR_L_IN1_Pin | MOTOR_L_IN2_Pin |
                      MOTOR_R_IN1_Pin | MOTOR_R_IN2_Pin |
                      MOTOR_STBY_Pin,
                      GPIO_PIN_RESET);      // Todos en 0 inicialmente

    GPIO_InitStruct.Pin   = MOTOR_L_IN1_Pin | MOTOR_L_IN2_Pin |
                            MOTOR_R_IN1_Pin | MOTOR_R_IN2_Pin |
                            MOTOR_STBY_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* LED PC13 (activo bajo) */
    HAL_GPIO_WritePin(LED_ESTADO_GPIO_Port, LED_ESTADO_Pin, GPIO_PIN_SET); // apagado
    GPIO_InitStruct.Pin   = LED_ESTADO_Pin;
    HAL_GPIO_Init(LED_ESTADO_GPIO_Port, &GPIO_InitStruct);
}

void Error_Handler(void)
{
    __disable_irq();
    while(1) { }
}
