#ifndef BMP280_H
#define BMP280_H

#include "stm32f1xx_hal.h"


#define BMP280_CS_LOW()   HAL_GPIO_WritePin(SPI_BMP_CS_GPIO_Port, SPI_BMP_CS_Pin, GPIO_PIN_RESET)
#define BMP280_CS_HIGH()  HAL_GPIO_WritePin(SPI_BMP_CS_GPIO_Port, SPI_BMP_CS_Pin, GPIO_PIN_SET)


#define BMP280_REG_ID            0xD0  /* Chip ID (debe ser 0x58) */
#define BMP280_REG_RESET         0xE0  /* Reset */
#define BMP280_REG_STATUS        0xF3  /* Status */
#define BMP280_REG_CTRL_MEAS     0xF4  /* Configuración de medición */
#define BMP280_REG_CONFIG        0xF5  /* Configuración general */
#define BMP280_REG_PRESS_MSB     0xF7  /* Presión MSB */
#define BMP280_REG_TEMP_MSB      0xFA  /* Temperatura MSB */

/* Calibración - dirección inicial de los coeficientes */
#define BMP280_CALIB_START       0x88
#define BMP280_CALIB_LEN         24


typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
} bmp280_calib_t;



/**
 * @brief Inicializa el sensor BMP280
 * @param hspi Puntero al manejador SPI
 * @retval 1 si éxito, 0 si error
 */
uint8_t BMP280_Init(SPI_HandleTypeDef *hspi);

/**
 * @brief Lee el ID del chip (debe ser 0x58)
 * @retVal Valor del registro ID
 */
uint8_t BMP280_ReadID(void);

/**
 * @brief Lee los coeficientes de calibración
 */
void BMP280_ReadCalibration(void);

/**
 * @brief Lee valores RAW del sensor
 * @param raw_temp Puntero para temperatura RAW
 * @param raw_press Puntero para presión RAW
 */
void BMP280_ReadRaw(int32_t *raw_temp, int32_t *raw_press);

/**
 * @brief Compensa la temperatura RAW
 * @param raw_temp Valor RAW de temperatura
 * @retval Temperatura en grados Celsius
 */
float BMP280_CompensateTemp(int32_t raw_temp);

/**
 * @brief Compensa la presión RAW
 * @param raw_press Valor RAW de presión
 * @param raw_temp Valor RAW de temperatura
 * @retval Presión en Pascales (Pa)
 */
float BMP280_CompensatePress(int32_t raw_press, int32_t raw_temp);

/**
 * @brief Lee datos compensados del sensor
 * @param temp Puntero para temperatura (°C)
 * @param press Puntero para presión (Pa)
 */
void BMP280_ReadData(float *temp, float *press);

#endif /* BMP280_H */
