#ifndef JSON_H
#define JSON_H

#include "stm32f1xx_hal.h"


#define CMD_READ        "READ"      /* Lectura única */
#define CMD_LOG_ON      "LOG ON"    /* Activar envío periódico */
#define CMD_LOG_OFF     "LOG OFF"   /* Desactivar envío periódico */
#define PERIODIC_MS     5000        /* Intervalo en milisegundos (5 segundos) */



/**
 * @brief Envía datos en formato JSON por UART - Ejercicio 5
 * @param huart Manejador UART
 * @param temperature Temperatura en °C
 * @param pressure Presión en Pa
 */
void JSON_SendData(UART_HandleTypeDef *huart, float temperature, float pressure);

/**
 * @brief Envía string por UART (función auxiliar)
 * @param huart Manejador UART
 * @param str String a enviar
 */
void UART_SendString(UART_HandleTypeDef *huart, const char *str);

/**
 * @brief Procesa comandos UART - Ejercicio 6
 * @param command Comando recibido
 * @param huart_debug UART para debug (USART1)
 * @param huart_esp UART para ESP (USART2)
 * @param read_function Función que lee el sensor
 */
void ProcessUARTCommand(char *command, UART_HandleTypeDef *huart_debug,
                        UART_HandleTypeDef *huart_esp,
                        void (*read_function)(UART_HandleTypeDef*, UART_HandleTypeDef*));

/**
 * @brief Maneja el envío periódico de datos
 * @param huart_debug UART para debug
 * @param huart_esp UART para ESP
 * @param read_function Función que lee el sensor
 */
void HandlePeriodicSend(UART_HandleTypeDef *huart_debug, UART_HandleTypeDef *huart_esp,
                        void (*read_function)(UART_HandleTypeDef*, UART_HandleTypeDef*));

/**
 * @brief Activa/desactiva el envío periódico
 * @param enable 1 para activar, 0 para desactivar
 */
void SetPeriodicEnable(uint8_t enable);

/**
 * @brief Obtiene el estado del envío periódico
 * @retval 1 si activado, 0 si desactivado
 */
uint8_t IsPeriodicEnabled(void);

#endif /* JSON_H */
