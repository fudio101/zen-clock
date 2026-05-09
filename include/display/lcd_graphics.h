#ifndef LCD_GRAPHICS_H
#define LCD_GRAPHICS_H

#include <stdint.h>
#include "esp_lcd_panel_ops.h"

/**
 * @brief Fill the entire screen with a single RGB565 color.
 *
 * @param panel  Panel handle from lcd_driver_init()
 * @param color  16-bit RGB565 color value
 */
void lcd_fill_color(esp_lcd_panel_handle_t panel, uint16_t color);

/**
 * @brief Fill a rectangular area with a solid color.
 *
 * @param panel  Panel handle
 * @param x      Top-left X coordinate
 * @param y      Top-left Y coordinate
 * @param w      Width in pixels
 * @param h      Height in pixels
 * @param color  16-bit RGB565 color value
 */
void lcd_fill_rect(esp_lcd_panel_handle_t panel, int x, int y, int w, int h,
                   uint16_t color);

/**
 * @brief Draw a horizontal line.
 *
 * @param panel  Panel handle
 * @param x      Starting X coordinate
 * @param y      Y coordinate
 * @param len    Length in pixels
 * @param color  16-bit RGB565 color value
 */
void lcd_draw_hline(esp_lcd_panel_handle_t panel, int x, int y, int len,
                    uint16_t color);

/**
 * @brief Draw a vertical line.
 *
 * @param panel  Panel handle
 * @param x      X coordinate
 * @param y      Starting Y coordinate
 * @param len    Length in pixels
 * @param color  16-bit RGB565 color value
 */
void lcd_draw_vline(esp_lcd_panel_handle_t panel, int x, int y, int len,
                    uint16_t color);

/**
 * @brief Draw a single pixel.
 *
 * @param panel  Panel handle
 * @param x      X coordinate
 * @param y      Y coordinate
 * @param color  16-bit RGB565 color value
 */
void lcd_draw_pixel(esp_lcd_panel_handle_t panel, int x, int y,
                    uint16_t color);

/**
 * @brief Draw an unfilled rectangle outline.
 *
 * @param panel  Panel handle
 * @param x      Top-left X coordinate
 * @param y      Top-left Y coordinate
 * @param w      Width in pixels
 * @param h      Height in pixels
 * @param color  16-bit RGB565 color value
 */
void lcd_draw_rect(esp_lcd_panel_handle_t panel, int x, int y, int w, int h,
                   uint16_t color);

#endif // LCD_GRAPHICS_H
