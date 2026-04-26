#include "json.h"
#include <stdio.h>
#include <string.h>

/* Variables estáticas */
static uint8_t periodic_enabled = 0;
static uint32_t last_send_time = 0;


/**
 * @brief Envía datos en formato JSON - Ejercicio 5
 */
void JSON_SendData(UART_HandleTypeDef *huart, float temperature, float pressure) {
    char json_buffer[64];
    int len;

    /* Formato JSON: {"T":24.3,"P":101325} */
    len = snprintf(json_buffer, sizeof(json_buffer), "{\"T\":%.1f,\"P\":%.0f}\r\n",
                   temperature, pressure);

    HAL_UART_Transmit(huart, (uint8_t*)json_buffer, len, HAL_MAX_DELAY);
}

/**
 * @brief Envía string por UART
 */
void UART_SendString(UART_HandleTypeDef *huart, const char *str) {
    HAL_UART_Transmit(huart, (uint8_t*)str, strlen(str), HAL_MAX_DELAY);
}

/**
 * @brief Procesa comandos UART - Ejercicio 6
 */
void ProcessUARTCommand(char *command, UART_HandleTypeDef *huart_debug,
                        UART_HandleTypeDef *huart_esp,
                        void (*read_function)(UART_HandleTypeDef*, UART_HandleTypeDef*)) {

    /* Comando READ: lectura única */
    if(strcmp(command, CMD_READ) == 0) {
        if(read_function != NULL) {
            read_function(huart_debug, huart_esp);
        }
    }
    /* Comando LOG ON: activar envío periódico */
    else if(strcmp(command, CMD_LOG_ON) == 0) {
        periodic_enabled = 1;
        UART_SendString(huart_debug, "LOG ON - Envío periódico activado (cada 5s)\r\n");
    }
    /* Comando LOG OFF: desactivar envío periódico */
    else if(strcmp(command, CMD_LOG_OFF) == 0) {
        periodic_enabled = 0;
        UART_SendString(huart_debug, "LOG OFF - Envío periódico desactivado\r\n");
    }
    /* Comando no reconocido */
    else if(strlen(command) > 0) {
        UART_SendString(huart_debug, "Comando no reconocido. Use: READ, LOG ON, LOG OFF\r\n");
    }
}

/**
 * @brief Maneja el envío periódico
 */
void HandlePeriodicSend(UART_HandleTypeDef *huart_debug, UART_HandleTypeDef *huart_esp,
                        void (*read_function)(UART_HandleTypeDef*, UART_HandleTypeDef*)) {
    uint32_t current_time = HAL_GetTick();

    if(periodic_enabled && (current_time - last_send_time) >= PERIODIC_MS) {
        last_send_time = current_time;
        if(read_function != NULL) {
            read_function(huart_debug, huart_esp);
        }
    }
}

/**
 * @brief Activa/desactiva envío periódico
 */
void SetPeriodicEnable(uint8_t enable) {
    periodic_enabled = enable;
    if(enable) {
        last_send_time = HAL_GetTick();
    }
}

/**
 * @brief Obtiene estado del envío periódico
 */
uint8_t IsPeriodicEnabled(void) {
    return periodic_enabled;
}
