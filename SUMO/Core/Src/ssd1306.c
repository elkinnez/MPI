#include "ssd1306.h"
#include "main.h"      // Para hi2c1 y HAL_Delay
#include <string.h>    // Para memset

// Dirección I2C de la OLED (si no está definida en ssd1306.h, la definimos aquí)
#ifndef SSD1306_ADDR
#define SSD1306_ADDR 0x78   // 0x3C << 1 (para escritura)
#endif

extern I2C_HandleTypeDef hi2c1;
static uint8_t buffer[1024];   // 128*64/8 = 1024 bytes

// Envía un comando a la OLED
static void ssd1306_WriteCommand(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};  // Co=0, D/C=0
    HAL_I2C_Master_Transmit(&hi2c1, SSD1306_ADDR, buf, 2, 100);
}

// Envía datos (píxeles) a la OLED
static void ssd1306_WriteData(uint8_t *data, uint16_t len) {
    // El primer byte es 0x40 (Co=0, D/C=1). El resto son datos.
    // El buffer debe tener espacio para len+1 bytes.
    if (len > 255) return;  // Protección: nuestro buffer local tiene 257 bytes máximo
    uint8_t buf[257];
    buf[0] = 0x40;
    for (uint16_t i = 0; i < len; i++) {
        buf[i+1] = data[i];
    }
    HAL_I2C_Master_Transmit(&hi2c1, SSD1306_ADDR, buf, len+1, 100);
}

// Inicializa la pantalla OLED
void ssd1306_Init(void) {
    HAL_Delay(100);
    ssd1306_WriteCommand(0xAE); // display off
    ssd1306_WriteCommand(0xD5); ssd1306_WriteCommand(0x80); // clock divide
    ssd1306_WriteCommand(0xA8); ssd1306_WriteCommand(0x3F); // multiplex
    ssd1306_WriteCommand(0xD3); ssd1306_WriteCommand(0x00); // offset
    ssd1306_WriteCommand(0x40); // start line
    ssd1306_WriteCommand(0x8D); ssd1306_WriteCommand(0x14); // charge pump
    ssd1306_WriteCommand(0x20); ssd1306_WriteCommand(0x00); // memory mode
    ssd1306_WriteCommand(0xA1); // segment remap
    ssd1306_WriteCommand(0xC8); // COM scan
    ssd1306_WriteCommand(0xDA); ssd1306_WriteCommand(0x12); // COM pins
    ssd1306_WriteCommand(0x81); ssd1306_WriteCommand(0xCF); // contrast
    ssd1306_WriteCommand(0xD9); ssd1306_WriteCommand(0xF1); // precharge
    ssd1306_WriteCommand(0xDB); ssd1306_WriteCommand(0x40); // vcom detect
    ssd1306_WriteCommand(0xA4); // resume
    ssd1306_WriteCommand(0xA6); // normal display
    ssd1306_WriteCommand(0x2E); // deactivate scroll
    ssd1306_WriteCommand(0xAF); // display on
    ssd1306_Fill(0);
    ssd1306_UpdateScreen();
}

// Rellena toda la pantalla con un color (0=negro, 1=blanco)
void ssd1306_Fill(uint8_t color) {
    memset(buffer, color ? 0xFF : 0x00, 1024);
}

// Envía todo el buffer a la pantalla (actualiza la OLED)
void ssd1306_UpdateScreen(void) {
    for (uint8_t page = 0; page < 8; page++) {
        ssd1306_WriteCommand(0xB0 + page); // set page address
        ssd1306_WriteCommand(0x00);        // column low nibble
        ssd1306_WriteCommand(0x10);        // column high nibble
        ssd1306_WriteData(&buffer[page * 128], 128);
    }
}

// Dibuja un píxel en coordenadas (x,y) con color (0=negro, 1=blanco)
void ssd1306_DrawPixel(uint8_t x, uint8_t y, uint8_t color) {
    if (x >= 128 || y >= 64) return;
    uint16_t idx = x + (y / 8) * 128;
    if (color)
        buffer[idx] |= (1 << (y % 8));
    else
        buffer[idx] &= ~(1 << (y % 8));
}

// Dibuja un bitmap de tamaño (w x h) en la posición (x,y)
// El bitmap debe estar en orientación vertical (cada byte son 8 píxeles en columna)
void ssd1306_DrawBitmap(uint8_t x, uint8_t y, const uint8_t *bitmap, uint8_t w, uint8_t h, uint8_t color) {
    for (uint8_t j = 0; j < h; j++) {
        for (uint8_t i = 0; i < w; i++) {
            // El índice en el bitmap: asumimos que está almacenado en columnas (vertical)
            uint16_t bitIndex = j * w + i;
            if (bitmap[bitIndex / 8] & (1 << (bitIndex % 8))) {
                ssd1306_DrawPixel(x + i, y + j, color);
            }
        }
    }
}

// Carga un bitmap completo de 128x64 píxeles (1024 bytes) directamente al buffer
// y actualiza la pantalla. Es mucho más rápido que DrawBitmap para imágenes completas.
void ssd1306_LoadFullBitmap(const uint8_t *bitmap) {
    memcpy(buffer, bitmap, 1024);   // usar memcpy es más eficiente que un bucle
    ssd1306_UpdateScreen();
}
