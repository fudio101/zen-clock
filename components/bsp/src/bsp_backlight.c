// SPDX-License-Identifier: MIT
// BSP Backlight — Thin facade over lcd_backlight component

#include "bsp_priv.h"
#include "board_config.h"
#include "lcd_backlight.h"

#include <esp_log.h>

static const char *TAG = "bsp_backlight";
static lcd_backlight_handle_t s_bl_handle;

// ============================================================
// Init (called from bsp_display.c during bsp_display_init)
// ============================================================
void bsp_backlight_setup(void)
{
  ESP_LOGI(TAG, "Configuring backlight via lcd_backlight component...");

  const lcd_backlight_config_t bl_config = {
      .gpio_num = PIN_LCD_BL,
      .pwm_freq_hz = LCD_BL_PWM_FREQ_HZ,
      .timer_num = LCD_BL_LEDC_TIMER,
      .channel_num = LCD_BL_LEDC_CHANNEL,
  };
  ESP_ERROR_CHECK(lcd_backlight_init(&bl_config, &s_bl_handle));
}

// ============================================================
// Internal API (called via bsp_priv.h)
// ============================================================
void bsp_backlight_set(uint8_t percent, uint32_t fade_time_ms)
{
  if (s_bl_handle)
  {
    ESP_ERROR_CHECK(lcd_backlight_set_brightness(s_bl_handle, percent, fade_time_ms));
  }
}

uint8_t bsp_backlight_get(void)
{
  if (s_bl_handle)
  {
    return lcd_backlight_get_brightness(s_bl_handle);
  }
  return 0;
}
