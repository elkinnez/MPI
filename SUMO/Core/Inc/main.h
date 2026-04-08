/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_ESTADO_Pin GPIO_PIN_13
#define LED_ESTADO_GPIO_Port GPIOC
#define LED_FRONTAL_Pin GPIO_PIN_1
#define LED_FRONTAL_GPIO_Port GPIOB
#define BT_TX_Pin GPIO_PIN_10
#define BT_TX_GPIO_Port GPIOB
#define BT_RX_Pin GPIO_PIN_11
#define BT_RX_GPIO_Port GPIOB
#define MOTOR_IZQ_DIR1_Pin GPIO_PIN_12
#define MOTOR_IZQ_DIR1_GPIO_Port GPIOB
#define MOTOR_IZQ_DIR2_Pin GPIO_PIN_13
#define MOTOR_IZQ_DIR2_GPIO_Port GPIOB
#define MOTOR_DER_DIR1_Pin GPIO_PIN_14
#define MOTOR_DER_DIR1_GPIO_Port GPIOB
#define MOTOR_DER_DIR2_Pin GPIO_PIN_15
#define MOTOR_DER_DIR2_GPIO_Port GPIOB
#define PWM_IZQ_Pin GPIO_PIN_8
#define PWM_IZQ_GPIO_Port GPIOA
#define PWM_DER_Pin GPIO_PIN_10
#define PWM_DER_GPIO_Port GPIOA
#define TRIG_US_Pin GPIO_PIN_7
#define TRIG_US_GPIO_Port GPIOB
#define ECHO_US_Pin GPIO_PIN_8
#define ECHO_US_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
