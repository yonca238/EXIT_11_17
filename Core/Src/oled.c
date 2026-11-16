#include "oled.h"
#include "oled_fonts.h"
#include <stddef.h>
#include <string.h>

#ifndef OLED_X_OFFSET
#define OLED_X_OFFSET 0U
#endif

#define OLED_I2C_TIMEOUT 100U

static uint8_t OLED_Buffer[(OLED_WIDTH * OLED_HEIGHT) / 8U];

static void OLED_RecordStatus(OLED_t *oled, HAL_StatusTypeDef status)
{
  if (status != HAL_OK)
  {
    oled->error_count++;
  }
}

static void OLED_WriteCommand(OLED_t *oled, uint8_t command)
{
  HAL_StatusTypeDef status;

  if (oled == NULL || oled->handle == NULL)
  {
    return;
  }

  status = HAL_I2C_Mem_Write(oled->handle,
                             (uint16_t)(oled->address_7bit << 1),
                             0x00U,
                             1U,
                             &command,
                             1U,
                             OLED_I2C_TIMEOUT);
  OLED_RecordStatus(oled, status);
}

static void OLED_WriteData(OLED_t *oled,
                           const uint8_t *data,
                           uint16_t length)
{
  HAL_StatusTypeDef status;

  if (oled == NULL || oled->handle == NULL || data == NULL)
  {
    return;
  }

  status = HAL_I2C_Mem_Write(oled->handle,
                             (uint16_t)(oled->address_7bit << 1),
                             0x40U,
                             1U,
                             (uint8_t *)data,
                             length,
                             OLED_I2C_TIMEOUT);
  OLED_RecordStatus(oled, status);
}

static uint8_t OLED_Probe(I2C_HandleTypeDef *hi2c, uint8_t address_7bit)
{
  return HAL_I2C_IsDeviceReady(hi2c,
                               (uint16_t)(address_7bit << 1),
                               3U,
                               20U) == HAL_OK;
}

uint8_t OLED_InitAddress(OLED_t *oled,
                         I2C_HandleTypeDef *hi2c,
                         uint8_t address_7bit)
{
  static const uint8_t init_commands[] =
  {
    0xAE,
    0xD5, 0x80,
    0xA8, (OLED_HEIGHT == 32U) ? 0x1F : 0x3F,
    0xD3, 0x00,
    0x40,
    0x8D, 0x14,
    0x20, 0x00,
    0xA1,
    0xC8,
    0xDA, (OLED_HEIGHT == 32U) ? 0x02 : 0x12,
    0x81, 0xCF,
    0xD9, 0xF1,
    0xDB, 0x40,
    0xA4,
    0xA6,
    0x2E,
    0xAF
  };
  uint32_t i;

  if (oled == NULL || hi2c == NULL)
  {
    return 0U;
  }

  oled->handle = hi2c;
  oled->address_7bit = address_7bit;
  oled->ready = 0U;
  oled->cursor_x = 0U;
  oled->cursor_y = 0U;
  oled->error_count = 0U;

  if (OLED_Probe(hi2c, address_7bit) == 0U)
  {
    return 0U;
  }

  HAL_Delay(100U);

  for (i = 0U; i < sizeof(init_commands); i++)
  {
    OLED_WriteCommand(oled, init_commands[i]);
  }

  if (oled->error_count == 0U)
  {
    oled->ready = 1U;
    memset(OLED_Buffer, 0, sizeof(OLED_Buffer));
    OLED_Update(oled);
  }

  if (oled->error_count != 0U)
  {
    oled->ready = 0U;
  }

  return oled->ready;
}

uint8_t OLED_Init(OLED_t *oled, I2C_HandleTypeDef *hi2c)
{
  if (OLED_InitAddress(oled, hi2c, 0x3CU) != 0U)
  {
    return 1U;
  }

  return OLED_InitAddress(oled, hi2c, 0x3DU);
}

uint8_t OLED_IsReady(const OLED_t *oled)
{
  return (oled != NULL && oled->ready != 0U) ? 1U : 0U;
}

void OLED_Clear(OLED_t *oled)
{
  if (OLED_IsReady(oled) == 0U)
  {
    return;
  }

  memset(OLED_Buffer, 0, sizeof(OLED_Buffer));
  OLED_Update(oled);
}

void OLED_Update(OLED_t *oled)
{
  uint8_t page;
  uint8_t command[3];

  if (oled == NULL || oled->handle == NULL)
  {
    return;
  }

  for (page = 0U; page < (OLED_HEIGHT / 8U); page++)
  {
    command[0] = (uint8_t)(0xB0U + page);
    command[1] = (uint8_t)(OLED_X_OFFSET & 0x0FU);
    command[2] = (uint8_t)(0x10U | ((OLED_X_OFFSET >> 4U) & 0x07U));

    OLED_WriteCommand(oled, command[0]);
    OLED_WriteCommand(oled, command[1]);
    OLED_WriteCommand(oled, command[2]);
    OLED_WriteData(oled,
                   &OLED_Buffer[(uint32_t)page * OLED_WIDTH],
                   OLED_WIDTH);
  }
}

void OLED_SetCursor(OLED_t *oled, uint16_t x, uint16_t y)
{
  if (oled == NULL)
  {
    return;
  }

  oled->cursor_x = x;
  oled->cursor_y = y;
}

void OLED_DrawPixel(OLED_t *oled,
                    uint16_t x,
                    uint16_t y,
                    uint8_t color)
{
  uint32_t index;

  if (oled == NULL || x >= OLED_WIDTH || y >= OLED_HEIGHT)
  {
    return;
  }

  index = (uint32_t)x + ((uint32_t)y / 8U) * OLED_WIDTH;

  if (color != 0U)
  {
    OLED_Buffer[index] |= (uint8_t)(1U << (y % 8U));
  }
  else
  {
    OLED_Buffer[index] &= (uint8_t)~(1U << (y % 8U));
  }
}

void OLED_DrawLine(OLED_t *oled,
                   uint16_t x1,
                   uint16_t y1,
                   uint16_t x2,
                   uint16_t y2,
                   uint8_t color)
{
  int32_t dx = (x2 > x1) ? (int32_t)(x2 - x1) : (int32_t)(x1 - x2);
  int32_t dy = (y2 > y1) ? (int32_t)(y2 - y1) : (int32_t)(y1 - y2);
  int32_t sx = (x1 < x2) ? 1 : -1;
  int32_t sy = (y1 < y2) ? 1 : -1;
  int32_t error = dx - dy;

  while (1)
  {
    int32_t error2;

    OLED_DrawPixel(oled, x1, y1, color);

    if ((x1 == x2) && (y1 == y2))
    {
      break;
    }

    error2 = error * 2;

    if (error2 > -dy)
    {
      error -= dy;
      x1 = (uint16_t)((int32_t)x1 + sx);
    }

    if (error2 < dx)
    {
      error += dx;
      y1 = (uint16_t)((int32_t)y1 + sy);
    }
  }
}

void OLED_DrawRectangle(OLED_t *oled,
                        uint16_t x1,
                        uint16_t y1,
                        uint16_t x2,
                        uint16_t y2,
                        uint8_t color)
{
  OLED_DrawLine(oled, x1, y1, x2, y1, color);
  OLED_DrawLine(oled, x2, y1, x2, y2, color);
  OLED_DrawLine(oled, x2, y2, x1, y2, color);
  OLED_DrawLine(oled, x1, y2, x1, y1, color);
}

static uint8_t OLED_WriteChar(OLED_t *oled, char ch, uint8_t scale)
{
  const uint16_t *glyph;
  uint16_t row;
  uint16_t col;

  if (oled == NULL || ch < 32 || ch > 126)
  {
    return 0U;
  }

  if ((oled->cursor_x + (6U * scale) > OLED_WIDTH) ||
      (oled->cursor_y + (8U * scale) > OLED_HEIGHT))
  {
    return 0U;
  }

  glyph = &OLED_Font6x8[(uint16_t)(ch - 32) * 8U];

  for (row = 0U; row < 8U; row++)
  {
    uint16_t bits = glyph[row];

    for (col = 0U; col < 6U; col++)
    {
      uint8_t color = (((bits << col) & 0x8000U) != 0U) ? 1U : 0U;
      uint16_t px;
      uint16_t py;

      for (py = 0U; py < scale; py++)
      {
        for (px = 0U; px < scale; px++)
        {
          OLED_DrawPixel(oled,
                         (uint16_t)(oled->cursor_x + (col * scale) + px),
                         (uint16_t)(oled->cursor_y + (row * scale) + py),
                         color);
        }
      }
    }
  }

  oled->cursor_x = (uint16_t)(oled->cursor_x + (6U * scale));
  return 1U;
}

static void OLED_WriteString(OLED_t *oled, const char *text, uint8_t scale)
{
  if (oled == NULL || text == NULL)
  {
    return;
  }

  while (*text != '\0')
  {
    if (OLED_WriteChar(oled, *text, scale) == 0U)
    {
      break;
    }

    text++;
  }
}

void OLED_ShowText(OLED_t *oled,
                   uint16_t x,
                   uint16_t y,
                   const char *text)
{
  if (OLED_IsReady(oled) == 0U)
  {
    return;
  }

  OLED_SetCursor(oled, x, y);
  OLED_WriteString(oled, text, 1U);
}

void OLED_ShowTextLarge(OLED_t *oled,
                        uint16_t x,
                        uint16_t y,
                        const char *text)
{
  if (OLED_IsReady(oled) == 0U)
  {
    return;
  }

  OLED_SetCursor(oled, x, y);
  OLED_WriteString(oled, text, 2U);
}

void OLED_ShowTwoLines(OLED_t *oled,
                       const char *line1,
                       const char *line2)
{
  if (OLED_IsReady(oled) == 0U)
  {
    return;
  }

  memset(OLED_Buffer, 0, sizeof(OLED_Buffer));

  if (line1 != NULL)
  {
    OLED_ShowText(oled, 0U, 0U, line1);
  }

  if (line2 != NULL)
  {
    OLED_ShowText(oled, 0U, 12U, line2);
  }

  OLED_Update(oled);
}
