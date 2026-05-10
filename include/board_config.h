#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// ============================================================
// Board: LilyGo T-Display-S3
// MCU:   ESP32-S3R8 (Dual-core Xtensa LX7, 240MHz)
// Display: 1.9" ST7789 320×170, I80 8-bit parallel
// ============================================================

#define BOARD_LILYGO_T_DISPLAY_S3

// --- LCD Data Bus (8-bit I80) ---
#define PIN_LCD_D0    39
#define PIN_LCD_D1    40
#define PIN_LCD_D2    41
#define PIN_LCD_D3    42
#define PIN_LCD_D4    45
#define PIN_LCD_D5    46
#define PIN_LCD_D6    47
#define PIN_LCD_D7    48

// --- LCD Control ---
#define PIN_LCD_WR    8   // Write clock (PCLK)
#define PIN_LCD_RD    9   // Read (input, pull-up)
#define PIN_LCD_CS    6
#define PIN_LCD_DC    7
#define PIN_LCD_RST   5
#define PIN_LCD_BL    38  // Backlight (PWM capable)
#define PIN_LCD_PWR   15  // LDO enable (HIGH = on)

// --- Display Parameters ---
#define LCD_H_RES     320 // Landscape width (after swap_xy)
#define LCD_V_RES     170 // Landscape height

// --- Battery ---
#define PIN_BAT_ADC   4   // GPIO4, ADC1_CH3, ×2 resistor divider

// --- User Buttons ---
#define PIN_BTN_BOOT  0   // BOOT button (active LOW)
#define PIN_BTN_IO14  14  // Side button (active LOW)

// --- Backlight Defaults ---
#define LCD_BL_PWM_FREQ_HZ   5000
#define LCD_BL_LEDC_TIMER     1  // LEDC_TIMER_1
#define LCD_BL_LEDC_CHANNEL   0  // LEDC_CHANNEL_0

#endif // BOARD_CONFIG_H
