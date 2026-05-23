#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
void ssd1306_LoadFullBitmap(const uint8_t *bitmap);
#define SSD1306_ADDR 0x78

void ssd1306_Init(void);
void ssd1306_Fill(uint8_t color);
void ssd1306_UpdateScreen(void);
void ssd1306_DrawPixel(uint8_t x, uint8_t y, uint8_t color);
void ssd1306_DrawBitmap(uint8_t x, uint8_t y, const uint8_t *bitmap, uint8_t w, uint8_t h, uint8_t color);

#endif
