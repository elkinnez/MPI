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
#define MOTOR_L_PWM_Pin GPIO_PIN_0
#define MOTOR_L_PWM_GPIO_Port GPIOA
#define MOTOR_R_PWM_Pin GPIO_PIN_1
#define MOTOR_R_PWM_GPIO_Port GPIOA
#define MOTOR_L_IN1_Pin GPIO_PIN_2
#define MOTOR_L_IN1_GPIO_Port GPIOA
#define MOTOR_R_IN1_Pin GPIO_PIN_3
#define MOTOR_R_IN1_GPIO_Port GPIOA
#define MOTOR_STBY_Pin GPIO_PIN_4
#define MOTOR_STBY_GPIO_Port GPIOA
#define LINEA_FR_Pin GPIO_PIN_5
#define LINEA_FR_GPIO_Port GPIOA
#define LINEA_RL_Pin GPIO_PIN_6
#define LINEA_RL_GPIO_Port GPIOA
#define LINEA_RR_Pin GPIO_PIN_7
#define LINEA_RR_GPIO_Port GPIOA
#define LINEA_FL_Pin GPIO_PIN_0
#define LINEA_FL_GPIO_Port GPIOB
#define ECHO_US_Pin GPIO_PIN_1
#define ECHO_US_GPIO_Port GPIOB
#define TRIG_US_Pin GPIO_PIN_8
#define TRIG_US_GPIO_Port GPIOA
#define DBG_TX_Pin GPIO_PIN_9
#define DBG_TX_GPIO_Port GPIOA
#define DBG_RX_Pin GPIO_PIN_10
#define DBG_RX_GPIO_Port GPIOA
#define TRIG_USB7_Pin GPIO_PIN_7
#define TRIG_USB7_GPIO_Port GPIOB
#define ECHO_USB8_Pin GPIO_PIN_8
#define ECHO_USB8_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
