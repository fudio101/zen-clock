#ifndef LCD_DRIVER_H
#define LCD_DRIVER_H

#include <stdint.h>
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"

/**
 * @brief Initialize the ST7789 LCD display via I80 parallel bus.
 *
 * Powers on the LDO, configures I80 bus, initializes the ST7789 panel
 * controller, and sets up backlight PWM. Returns a panel handle for drawing.
 *
 * @return esp_lcd_panel_handle_t  Handle to use with draw functions.
 */
esp_lcd_panel_handle_t lcd_driver_init(void);

/**
 * @brief Get the LCD panel IO handle.
 *
 * Needed for low-level panel IO operations (e.g., sending raw commands).
 * Only valid after lcd_driver_init() has been called.
 *
 * @return esp_lcd_panel_io_handle_t  IO handle, or NULL if not initialized.
 */
esp_lcd_panel_io_handle_t lcd_driver_get_io_handle(void);

/**
 * @brief Set backlight brightness via PWM (percentage)
 *
 * @param percent Brightness level (0 = off, 100 = max).
 * @param fade_ms Fade duration in milliseconds.
 */
void lcd_driver_set_backlight(uint8_t percent, uint32_t fade_ms);

#endif // LCD_DRIVER_H
