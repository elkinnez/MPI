#include "bmp280.h"
#include "main.h"
#include <string.h>

/* Variables privadas estáticas */
static SPI_HandleTypeDef *bmp_spi = NULL;
static bmp280_calib_t calib;



/**
 * @brief Lee un registro del BMP280 vía SPI
 */
static uint8_t BMP280_ReadReg(uint8_t reg) {
    uint8_t tx = reg;
    uint8_t rx = 0;

    BMP280_CS_LOW();
    HAL_SPI_TransmitReceive(bmp_spi, &tx, &rx, 1, HAL_MAX_DELAY);
    BMP280_CS_HIGH();

    return rx;
}

/**
 * @brief Escribe un registro del BMP280 vía SPI
 */
static void BMP280_WriteReg(uint8_t reg, uint8_t value) {
    uint8_t tx[2] = {reg, value};

    BMP280_CS_LOW();
    HAL_SPI_Transmit(bmp_spi, tx, 2, HAL_MAX_DELAY);
    BMP280_CS_HIGH();
}

/**
 * @brief Lee múltiples bytes del BMP280
 */
static void BMP280_ReadBytes(uint8_t reg, uint8_t *buffer, uint8_t length) {
    BMP280_CS_LOW();
    HAL_SPI_Transmit(bmp_spi, &reg, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(bmp_spi, buffer, length, HAL_MAX_DELAY);
    BMP280_CS_HIGH();
}



/**
 * @brief Inicialización del BMP280
 */
uint8_t BMP280_Init(SPI_HandleTypeDef *hspi) {
    uint8_t chip_id;

    bmp_spi = hspi;

    /* Ejercicio 2: Leer ID del chip (registro 0xD0) */
    chip_id = BMP280_ReadReg(BMP280_REG_ID);

    if(chip_id != 0x58) {
        return 0;  /* Error: sensor no detectado */
    }

    /* Ejercicio 3: Leer coeficientes de calibración (24 bytes) */
    BMP280_ReadCalibration();

    /* Configurar sensor: oversampling x1, modo normal */
    BMP280_WriteReg(BMP280_REG_CTRL_MEAS, 0x27);

    /* Configurar: standby 1000ms, filter off */
    BMP280_WriteReg(BMP280_REG_CONFIG, 0xA0);

    return 1;
}

/**
 * @brief Lee el ID del chip
 */
uint8_t BMP280_ReadID(void) {
    return BMP280_ReadReg(BMP280_REG_ID);
}

/**
 * @brief Lee los 24 bytes de coeficientes de calibración
 */
void BMP280_ReadCalibration(void) {
    uint8_t calib_data[24];

    BMP280_ReadBytes(BMP280_CALIB_START, calib_data, BMP280_CALIB_LEN);

    calib.dig_T1 = (uint16_t)calib_data[1] << 8 | calib_data[0];
    calib.dig_T2 = (int16_t)calib_data[3] << 8 | calib_data[2];
    calib.dig_T3 = (int16_t)calib_data[5] << 8 | calib_data[4];
    calib.dig_P1 = (uint16_t)calib_data[7] << 8 | calib_data[6];
    calib.dig_P2 = (int16_t)calib_data[9] << 8 | calib_data[8];
    calib.dig_P3 = (int16_t)calib_data[11] << 8 | calib_data[10];
    calib.dig_P4 = (int16_t)calib_data[13] << 8 | calib_data[12];
    calib.dig_P5 = (int16_t)calib_data[15] << 8 | calib_data[14];
    calib.dig_P6 = (int16_t)calib_data[17] << 8 | calib_data[16];
    calib.dig_P7 = (int16_t)calib_data[19] << 8 | calib_data[18];
    calib.dig_P8 = (int16_t)calib_data[21] << 8 | calib_data[20];
    calib.dig_P9 = (int16_t)calib_data[23] << 8 | calib_data[22];
}

/**
 * @brief Lee valores RAW del sensor
 */
void BMP280_ReadRaw(int32_t *raw_temp, int32_t *raw_press) {
    uint8_t data[6];

    BMP280_ReadBytes(BMP280_REG_PRESS_MSB, data, 6);

    *raw_press = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) | ((int32_t)data[2] >> 4);
    *raw_temp  = ((int32_t)data[3] << 12) | ((int32_t)data[4] << 4) | ((int32_t)data[5] >> 4);
}

/**
 * @brief Compensa temperatura - Ejercicio 4
 */
float BMP280_CompensateTemp(int32_t raw_temp) {
    int32_t var1, var2, T;

    var1 = ((((raw_temp >> 3) - ((int32_t)calib.dig_T1 << 1))) * ((int32_t)calib.dig_T2)) >> 11;
    var2 = (((((raw_temp >> 4) - ((int32_t)calib.dig_T1)) *
              ((raw_temp >> 4) - ((int32_t)calib.dig_T1))) >> 12) *
              ((int32_t)calib.dig_T3)) >> 14;
    T = var1 + var2;

    return (float)T / 5120.0f;
}

/**
 * @brief Compensa presión - Ejercicio 4
 */
float BMP280_CompensatePress(int32_t raw_press, int32_t raw_temp) {
    int64_t var1, var2, p;

    var1 = ((int64_t)raw_temp) - 128000;
    var2 = var1 * var1 * (int64_t)calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)calib.dig_P3) >> 8) +
           ((var1 * (int64_t)calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calib.dig_P1) >> 33;

    if(var1 == 0) return 0.0f;

    p = 1048576 - raw_press;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)calib.dig_P7) << 4);

    return (float)p / 256.0f;
}

/**
 * @brief Lee datos compensados del sensor
 */
void BMP280_ReadData(float *temp, float *press) {
    int32_t raw_temp, raw_press;

    BMP280_ReadRaw(&raw_temp, &raw_press);
    *temp = BMP280_CompensateTemp(raw_temp);
    *press = BMP280_CompensatePress(raw_press, raw_temp);
}
