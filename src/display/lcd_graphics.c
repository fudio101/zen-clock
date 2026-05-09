#include "display/lcd_graphics.h"
#include "board_config.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

static const char *TAG = "lcd_graphics";

void lcd_fill_color(esp_lcd_panel_handle_t panel, uint16_t color)
{
  const int lines_per_batch = 40;
  size_t buf_size = LCD_H_RES * lines_per_batch * sizeof(uint16_t);
  uint16_t *buf = heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
  if (!buf)
  {
    ESP_LOGE(TAG, "Failed to allocate fill buffer");
    return;
  }
  for (int i = 0; i < LCD_H_RES * lines_per_batch; i++)
  {
    buf[i] = color;
  }
  for (int y = 0; y < LCD_V_RES; y += lines_per_batch)
  {
    int y_end = y + lines_per_batch;
    if (y_end > LCD_V_RES)
      y_end = LCD_V_RES;
    esp_lcd_panel_draw_bitmap(panel, 0, y, LCD_H_RES, y_end, buf);
  }
  free(buf);
}

void lcd_fill_rect(esp_lcd_panel_handle_t panel, int x, int y, int w, int h, uint16_t color)
{
  if (w <= 0 || h <= 0)
    return;
  if (x < 0)
  {
    w += x;
    x = 0;
  }
  if (y < 0)
  {
    h += y;
    y = 0;
  }
  if (x + w > LCD_H_RES)
    w = LCD_H_RES - x;
  if (y + h > LCD_V_RES)
    h = LCD_V_RES - y;
  if (w <= 0 || h <= 0)
    return;

  const int max_batch = 40;
  int batch = (h < max_batch) ? h : max_batch;
  size_t buf_size = w * batch * sizeof(uint16_t);
  uint16_t *buf = heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
  if (!buf)
  {
    ESP_LOGE(TAG, "rect alloc fail");
    return;
  }
  for (int i = 0; i < w * batch; i++)
    buf[i] = color;
  for (int row = y; row < y + h; row += batch)
  {
    int row_end = row + batch;
    if (row_end > y + h)
      row_end = y + h;
    esp_lcd_panel_draw_bitmap(panel, x, row, x + w, row_end, buf);
  }
  free(buf);
}

void lcd_draw_hline(esp_lcd_panel_handle_t panel, int x, int y, int len, uint16_t color)
{
  if (len <= 0 || y < 0 || y >= LCD_V_RES)
    return;
  if (x < 0)
  {
    len += x;
    x = 0;
  }
  if (x + len > LCD_H_RES)
    len = LCD_H_RES - x;
  if (len <= 0)
    return;
  uint16_t *buf = heap_caps_malloc(len * sizeof(uint16_t), MALLOC_CAP_DMA);
  if (!buf)
    return;
  for (int i = 0; i < len; i++)
    buf[i] = color;
  esp_lcd_panel_draw_bitmap(panel, x, y, x + len, y + 1, buf);
  free(buf);
}

void lcd_draw_vline(esp_lcd_panel_handle_t panel, int x, int y, int len, uint16_t color)
{
  if (len <= 0 || x < 0 || x >= LCD_H_RES)
    return;
  if (y < 0)
  {
    len += y;
    y = 0;
  }
  if (y + len > LCD_V_RES)
    len = LCD_V_RES - y;
  if (len <= 0)
    return;
  uint16_t *buf = heap_caps_malloc(len * sizeof(uint16_t), MALLOC_CAP_DMA);
  if (!buf)
    return;
  for (int i = 0; i < len; i++)
    buf[i] = color;
  esp_lcd_panel_draw_bitmap(panel, x, y, x + 1, y + len, buf);
  free(buf);
}

void lcd_draw_pixel(esp_lcd_panel_handle_t panel, int x, int y, uint16_t color)
{
  if (x < 0 || x >= LCD_H_RES || y < 0 || y >= LCD_V_RES)
    return;
  uint16_t pixel = color;
  esp_lcd_panel_draw_bitmap(panel, x, y, x + 1, y + 1, &pixel);
}

void lcd_draw_rect(esp_lcd_panel_handle_t panel, int x, int y, int w, int h, uint16_t color)
{
  if (w <= 0 || h <= 0)
    return;
  lcd_draw_hline(panel, x, y, w, color);
  lcd_draw_hline(panel, x, y + h - 1, w, color);
  lcd_draw_vline(panel, x, y, h, color);
  lcd_draw_vline(panel, x + w - 1, y, h, color);
}
