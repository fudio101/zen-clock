#ifndef FONT_8X8_H
#define FONT_8X8_H

#include <stdint.h>

// ============================================================
// 8x8 Bitmap Font — ASCII 0x20 (space) to 0x7E (~)
// Each character = 8 bytes, 1 byte per row, MSB = leftmost pixel
// ============================================================

#define FONT_8X8_GLYPH_WIDTH 8  // Raw glyph width in pixels
#define FONT_8X8_GLYPH_HEIGHT 8 // Raw glyph height in pixels
#define FONT_8X8_FIRST_CHAR 0x20
#define FONT_8X8_LAST_CHAR 0x7E
#define FONT_8X8_GLYPH_COUNT (FONT_8X8_LAST_CHAR - FONT_8X8_FIRST_CHAR + 1)

/**
 * @brief 8x8 font bitmap data array.
 *
 * 95 glyphs covering printable ASCII characters.
 * Stored in Flash (const) to save RAM.
 */
extern const uint8_t font_8x8_data[FONT_8X8_GLYPH_COUNT][FONT_8X8_GLYPH_HEIGHT];

#endif // FONT_8X8_H
