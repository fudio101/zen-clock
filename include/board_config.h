#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// ============================================================
// Board: LilyGo T-Display-S3
// MCU:   ESP32-S3R8 (Dual-core Xtensa LX7, 240MHz)
// ============================================================

#define BOARD_LILYGO_T_DISPLAY_S3

// ============================================================
// ST7789 LCD — Intel 8080 (I80) 8-bit Parallel Interface
// ============================================================

// 8-bit parallel data bus
#define LCD_D0 39
#define LCD_D1 40
#define LCD_D2 41
#define LCD_D3 42
#define LCD_D4 45
#define LCD_D5 46
#define LCD_D6 47
#define LCD_D7 48

// Control pins
#define LCD_WR 8
#define LCD_RD 9
#define LCD_CS 6
#define LCD_DC 7
#define LCD_RST 5
#define LCD_BL 38    // Backlight (PWM capable)
#define LCD_POWER 15 // LDO Enable — must be HIGH to power LCD

// Display resolution (landscape after swap_xy)
#define LCD_H_RES 320
#define LCD_V_RES 170

// ============================================================
// Battery Monitoring
// ============================================================
#define BATTERY_ADC_PIN 4 // GPIO4 via resistor divider

// ============================================================
// Backlight PWM Configuration
// ============================================================
#define LCD_BL_PWM_FREQ_HZ 5000       // 5 kHz PWM frequency
#define LCD_BL_PWM_RESOLUTION 8       // 8-bit resolution (0-255)
#define LCD_BL_PWM_CHANNEL 0          // LEDC channel 0
#define LCD_BL_PWM_TIMER 0            // LEDC timer 0
#define LCD_BL_DEFAULT_BRIGHTNESS 200 // Default brightness (0-255)

#endif // BOARD_CONFIG_H
