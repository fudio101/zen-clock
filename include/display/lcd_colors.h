#ifndef LCD_COLORS_H
#define LCD_COLORS_H

#include <stdint.h>

// ============================================================
// RGB565 Color Helpers
// ============================================================

/**
 * @brief Convert 8-bit RGB components to a 16-bit RGB565 value.
 *
 * @param r  Red   (0-255)
 * @param g  Green (0-255)
 * @param b  Blue  (0-255)
 */
#define RGB565(r, g, b) \
  ((uint16_t)(((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3))

// ============================================================
// Basic Colors
// ============================================================
#define COLOR_BLACK 0x0000
#define COLOR_WHITE 0xFFFF
#define COLOR_RED 0xF800
#define COLOR_GREEN 0x07E0
#define COLOR_BLUE 0x001F
#define COLOR_YELLOW 0xFFE0
#define COLOR_CYAN 0x07FF

// ============================================================
// Extended Colors
// ============================================================
#define COLOR_MAGENTA 0xF81F
#define COLOR_ORANGE RGB565(255, 165, 0)
#define COLOR_DARK_GRAY RGB565(64, 64, 64)
#define COLOR_LIGHT_GRAY RGB565(192, 192, 192)
#define COLOR_DARK_GREEN RGB565(0, 128, 0)
#define COLOR_DARK_BLUE RGB565(0, 0, 128)
#define COLOR_PURPLE RGB565(128, 0, 128)
#define COLOR_PINK RGB565(255, 192, 203)
#define COLOR_BROWN RGB565(139, 69, 19)
#define COLOR_NAVY RGB565(0, 0, 80)
#define COLOR_TEAL RGB565(0, 128, 128)
#define COLOR_GOLD RGB565(255, 215, 0)

#endif // LCD_COLORS_H
