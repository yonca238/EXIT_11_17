#ifndef __OLED_H__
#define __OLED_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#ifndef OLED_WIDTH
#define OLED_WIDTH 128U
#endif

#ifndef OLED_HEIGHT
#define OLED_HEIGHT 64U
#endif

typedef struct
{
  I2C_HandleTypeDef *handle;
  uint8_t address_7bit;
  uint8_t ready;
  uint16_t cursor_x;
  uint16_t cursor_y;
  uint32_t error_count;
} OLED_t;

uint8_t OLED_Init(OLED_t *oled, I2C_HandleTypeDef *hi2c);
uint8_t OLED_InitAddress(OLED_t *oled,
                         I2C_HandleTypeDef *hi2c,
                         uint8_t address_7bit);
uint8_t OLED_IsReady(const OLED_t *oled);

void OLED_Clear(OLED_t *oled);
void OLED_Update(OLED_t *oled);
void OLED_SetCursor(OLED_t *oled, uint16_t x, uint16_t y);
void OLED_DrawPixel(OLED_t *oled,
                    uint16_t x,
                    uint16_t y,
                    uint8_t color);
void OLED_DrawLine(OLED_t *oled,
                   uint16_t x1,
                   uint16_t y1,
                   uint16_t x2,
                   uint16_t y2,
                   uint8_t color);
void OLED_DrawRectangle(OLED_t *oled,
                        uint16_t x1,
                        uint16_t y1,
                        uint16_t x2,
                        uint16_t y2,
                        uint8_t color);

void OLED_ShowText(OLED_t *oled,
                   uint16_t x,
                   uint16_t y,
                   const char *text);
void OLED_ShowTextLarge(OLED_t *oled,
                        uint16_t x,
                        uint16_t y,
                        const char *text);
void OLED_ShowTwoLines(OLED_t *oled,
                       const char *line1,
                       const char *line2);

#ifdef __cplusplus
}
#endif

#endif
