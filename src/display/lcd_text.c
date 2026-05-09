#include "display/lcd_text.h"
#include "fonts/font_8x8.h"
#include "board_config.h"
#include "esp_heap_caps.h"
#include <string.h>

// ============================================================
// Default font instance: 8x8 glyphs rendered at 8x16 (2x Y scale)
// ============================================================
const lcd_font_t FONT_DEFAULT = {
    .data = font_8x8_data,
    .glyph_width = FONT_8X8_GLYPH_WIDTH,
    .glyph_height = FONT_8X8_GLYPH_HEIGHT,
    .render_width = FONT_8X8_GLYPH_WIDTH,       // 8px wide
    .render_height = FONT_8X8_GLYPH_HEIGHT * 2, // 16px tall (2x vertical)
    .first_char = FONT_8X8_FIRST_CHAR,
    .last_char = FONT_8X8_LAST_CHAR,
    .y_scale = 2,
};

void lcd_draw_char(esp_lcd_panel_handle_t panel, int x, int y, char ch,
                   const lcd_font_t *font, uint16_t fg, uint16_t bg)
{
  // Clamp to printable range
  if (ch < font->first_char || ch > font->last_char)
  {
    ch = ' ';
  }

  int rw = font->render_width;
  int rh = font->render_height;

  // Boundary check — don't draw outside screen
  if (x + rw > LCD_H_RES || y + rh > LCD_V_RES)
  {
    return;
  }

  uint16_t buf[rw * rh];
  const uint8_t *glyph = font->data[ch - font->first_char];

  for (int row = 0; row < rh; row++)
  {
    // Scale: each glyph row is used y_scale times
    uint8_t bits = glyph[row / font->y_scale];
    for (int col = 0; col < rw; col++)
    {
      // Reverse bit order to compensate for hardware mirror
      if (bits & (1 << col))
      {
        buf[row * rw + col] = fg;
      }
      else
      {
        buf[row * rw + col] = bg;
      }
    }
  }

  esp_lcd_panel_draw_bitmap(panel, x, y, x + rw, y + rh, buf);
}

void lcd_draw_string(esp_lcd_panel_handle_t panel, int x, int y,
                     const char *str, const lcd_font_t *font,
                     uint16_t fg, uint16_t bg)
{
  int cx = x;
  int cy = y;
  int rw = font->render_width;
  int rh = font->render_height;

  while (*str)
  {
    if (*str == '\n')
    {
      cx = x;
      cy += rh;
      str++;
      continue;
    }

    if (cx + rw > LCD_H_RES)
    {
      cx = 0;
      cy += rh;
    }
    if (cy + rh > LCD_V_RES)
    {
      break; // Off-screen
    }

    lcd_draw_char(panel, cx, cy, *str, font, fg, bg);
    cx += rw;
    str++;
  }
}

void lcd_draw_string_fast(esp_lcd_panel_handle_t panel, int x, int y,
                          const char *str, int max_len,
                          const lcd_font_t *font, uint16_t fg, uint16_t bg)
{
  int rw = font->render_width;
  int rh = font->render_height;
  int total_width = rw * max_len;

  // Clamp to screen width
  if (x + total_width > LCD_H_RES)
  {
    max_len = (LCD_H_RES - x) / rw;
    total_width = rw * max_len;
  }
  if (max_len <= 0 || y + rh > LCD_V_RES)
    return;

  // Allocate a single buffer for the entire string region
  size_t buf_size = total_width * rh * sizeof(uint16_t);
  uint16_t *buf = heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
  if (!buf)
  {
    // Fallback to char-by-char if DMA alloc fails
    int len = strlen(str);
    int cx = x;
    for (int i = 0; i < max_len; i++)
    {
      char ch = (i < len) ? str[i] : ' ';
      lcd_draw_char(panel, cx, y, ch, font, fg, bg);
      cx += rw;
    }
    return;
  }

  // Fill the buffer with background color
  for (int i = 0; i < total_width * rh; i++)
  {
    buf[i] = bg;
  }

  // Render each character into the buffer
  int len = strlen(str);
  for (int ci = 0; ci < max_len; ci++)
  {
    char ch = (ci < len) ? str[ci] : ' ';
    if (ch < font->first_char || ch > font->last_char)
      ch = ' ';
    const uint8_t *glyph = font->data[ch - font->first_char];

    int char_x_offset = ci * rw;
    for (int row = 0; row < rh; row++)
    {
      uint8_t bits = glyph[row / font->y_scale];
      for (int col = 0; col < rw; col++)
      {
        if (bits & (1 << col))
        {
          buf[row * total_width + char_x_offset + col] = fg;
        }
      }
    }
  }

  // Single DMA transfer for entire string
  esp_lcd_panel_draw_bitmap(panel, x, y, x + total_width, y + rh, buf);
  free(buf);
}
