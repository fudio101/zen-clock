// SPDX-License-Identifier: MIT
// Based on: https://github.com/hiruna/esp-idf-t-display-s3
// Adapted for ZenClock project — uses lcd_backlight component instead of aw9364

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "esp_lvgl_port.h"

// ============================================================
// LilyGo T-Display-S3 Pin Definitions
// Refer to: https://github.com/Xinyuan-LilyGO/T-Display-S3
// ============================================================

// LCD control pins
#define LCD_PIN_NUM_PWR 15      // LCD_Power_On (LDO Enable)
#define LCD_PIN_NUM_BK_LIGHT 38 // LCD_BL (Backlight PWM)
#define LCD_PIN_NUM_RD 9        // LCD_RD
#define LCD_PIN_NUM_DC 7        // LCD_DC
#define LCD_PIN_NUM_CS 6        // LCD_CS
#define LCD_PIN_NUM_RST 5       // LCD_RES
#define LCD_PIN_NUM_PCLK 8      // LCD_WR

// LCD 8-bit parallel data bus
#define LCD_PIN_NUM_DATA0 39
#define LCD_PIN_NUM_DATA1 40
#define LCD_PIN_NUM_DATA2 41
#define LCD_PIN_NUM_DATA3 42
#define LCD_PIN_NUM_DATA4 45
#define LCD_PIN_NUM_DATA5 46
#define LCD_PIN_NUM_DATA6 47
#define LCD_PIN_NUM_DATA7 48

// Backlight LEDC channel
#define LCD_BK_LIGHT_LEDC_CH 0

// Battery voltage ADC
#define BAT_PIN_NUM_VOLT 4 // ADC_UNIT_1, ADC_CHANNEL_3

// Buttons
#define BTN_PIN_NUM_1 GPIO_NUM_0  // BOOT
#define BTN_PIN_NUM_2 GPIO_NUM_14 // IO14

// ============================================================
// LCD Configuration
// ============================================================

// Display resolution (landscape, after swap_xy)
#define LCD_H_RES 320
#define LCD_V_RES 170

// Power levels
#define LCD_PWR_ON_LEVEL 1
#define LCD_PWR_OFF_LEVEL (!LCD_PWR_ON_LEVEL)

// ST7789 command/param bit width
#define LCD_CMD_BITS 8
#define LCD_PARAM_BITS 8

// I80 bus configuration
#define LCD_I80_BUS_WIDTH 8
#define LCD_PIXEL_CLOCK_HZ (17 * 1000 * 1000) // 17 MHz
#define LCD_I80_TRANS_QUEUE_SIZE 20
#define LCD_I80_DC_CMD_LEVEL 0
#define LCD_I80_DC_DUMMY_LEVEL 0
#define LCD_I80_DC_DATA_LEVEL 1

// PSRAM alignment for DMA transfers
#define LCD_PSRAM_TRANS_ALIGN 64
#define LCD_SRAM_TRANS_ALIGN 4

// ============================================================
// LVGL Configuration
// ============================================================

// Buffer size: 1/10th of display + one extra row
#define LVGL_BUFFER_SIZE (((LCD_H_RES * LCD_V_RES) / 10) + LCD_H_RES)

// LVGL timer/task settings
#define LVGL_TICK_PERIOD_MS 5
#define LVGL_MAX_SLEEP_MS (LVGL_TICK_PERIOD_MS * 2)
#define LVGL_TASK_STACK_SIZE (4 * 1024)
#define LVGL_TASK_PRIORITY 2

  // ============================================================
  // Public API
  // ============================================================

  /**
   * @brief Initialize LCD hardware and LVGL display port.
   *
   * Initializes I80 bus, ST7789 panel, battery ADC, backlight,
   * and registers the display with LVGL via esp_lvgl_port.
   *
   * @param disp_handle  Output: LVGL display handle
   * @param backlight_on If true, turn on backlight immediately at 100%
   */
  void lcd_init(lv_disp_t **disp_handle, bool backlight_on);

  /**
   * @brief Set LCD brightness as percentage with optional fade.
   *
   * @param percent      Brightness 0-100%
   * @param fade_time_ms Fade duration in ms (0 = instant)
   */
  void lcd_set_brightness(uint8_t percent, uint32_t fade_time_ms);

  /**
   * @brief Get current brightness percentage.
   */
  uint8_t lcd_get_brightness(void);

  /**
   * @brief Get battery voltage in millivolts.
   */
  int get_battery_voltage(void);

  /**
   * @brief Convert voltage to battery percentage (0-100).
   */
  double volts_to_percentage(double volts);

  /**
   * @brief Get battery percentage (0-100).
   */
  int get_battery_percentage(void);

  /**
   * @brief Check if USB power is connected based on voltage reading.
   */
  bool usb_power_connected(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif
