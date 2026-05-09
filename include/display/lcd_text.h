#ifndef LCD_TEXT_H
#define LCD_TEXT_H

#include <stdint.h>
#include "esp_lcd_panel_ops.h"

// ============================================================
// Font Descriptor — allows swapping fonts at runtime
// ============================================================

/**
 * @brief Font descriptor for the text rendering engine.
 *
 * Decouples font data from rendering, allowing multiple fonts
 * (different sizes, styles) to be used with the same draw functions.
 */
typedef struct
{
  const uint8_t (*data)[8]; ///< Pointer to glyph data array
  uint8_t glyph_width;      ///< Raw glyph width (pixels)
  uint8_t glyph_height;     ///< Raw glyph height (pixels)
  uint8_t render_width;     ///< Rendered character width (after scaling)
  uint8_t render_height;    ///< Rendered character height (after scaling)
  uint8_t first_char;       ///< First printable character code
  uint8_t last_char;        ///< Last printable character code
  uint8_t y_scale;          ///< Vertical scale factor (1 = no scale, 2 = double)
} lcd_font_t;

/// Default font: 8x8 glyphs rendered at 8x16 (2x vertical scale)
extern const lcd_font_t FONT_DEFAULT;

// ============================================================
// Text Drawing API
// ============================================================

/**
 * @brief Draw a single ASCII character on the LCD.
 *
 * @param panel  Panel handle
 * @param x      X position (pixels from left)
 * @param y      Y position (pixels from top)
 * @param ch     ASCII character to draw
 * @param font   Font descriptor to use
 * @param fg     Foreground color (RGB565)
 * @param bg     Background color (RGB565)
 */
void lcd_draw_char(esp_lcd_panel_handle_t panel, int x, int y, char ch,
                   const lcd_font_t *font, uint16_t fg, uint16_t bg);

/**
 * @brief Draw a null-terminated string on the LCD.
 *
 * Characters wrap to the next line when reaching the screen edge.
 * Supports newline characters ('\n').
 *
 * @param panel  Panel handle
 * @param x      Starting X position
 * @param y      Starting Y position
 * @param str    Null-terminated ASCII string
 * @param font   Font descriptor to use
 * @param fg     Foreground color (RGB565)
 * @param bg     Background color (RGB565)
 */
void lcd_draw_string(esp_lcd_panel_handle_t panel, int x, int y,
                     const char *str, const lcd_font_t *font,
                     uint16_t fg, uint16_t bg);

/**
 * @brief Draw a string using a single batched DMA transfer.
 *
 * Renders the entire string (padded to max_len) into one buffer and
 * sends it in a single esp_lcd_panel_draw_bitmap() call. Much faster
 * than drawing character by character.
 *
 * Ideal for updating status/counter text in-place without flicker.
 *
 * @param panel    Panel handle
 * @param x        X position
 * @param y        Y position
 * @param str      Null-terminated string
 * @param max_len  Maximum character width (pads with bg color)
 * @param font     Font descriptor to use
 * @param fg       Foreground color (RGB565)
 * @param bg       Background color (RGB565)
 */
void lcd_draw_string_fast(esp_lcd_panel_handle_t panel, int x, int y,
                          const char *str, int max_len,
                          const lcd_font_t *font, uint16_t fg, uint16_t bg);

#endif // LCD_TEXT_H
