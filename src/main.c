#include <stdio.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp.h"
#include "lvgl.h"
#include "ui.h"

static const char *TAG = "ZenClock";

// ============================================================
// Brightness step for button control (10% per press)
// ============================================================
#define BRIGHTNESS_STEP 10

static void on_button_press(int btn_id, bool pressed)
{
  if (!pressed)
  {
    return; // only handle press, not release
  }

  uint8_t current = bsp_display_get_brightness();
  uint8_t target = current;

  if (btn_id == BSP_BTN_BOOT)
  {
    // BOOT button → brightness UP
    target = (current <= 100 - BRIGHTNESS_STEP) ? current + BRIGHTNESS_STEP : 100;
  }
  else if (btn_id == BSP_BTN_IO14)
  {
    // IO14 button → brightness DOWN
    target = (current >= BRIGHTNESS_STEP) ? current - BRIGHTNESS_STEP : 0;
  }

  bsp_display_set_brightness(target, 200);
  ESP_LOGI(TAG, "Brightness: %d%% → %d%%", current, target);
}

// ============================================================
// Application entry point
// ============================================================
void app_main(void)
{
  // Initialize BSP (LCD + LVGL port, backlight off initially)
  static lv_display_t *disp_handle;
  bsp_display_init(&disp_handle, false);

  // Initialize SquareLine Studio UI
  lvgl_port_lock(0);
  ui_init();
  lvgl_port_unlock();

  // Fade in backlight smoothly over 2 seconds
  bsp_display_set_brightness(100, 2000);

  // Initialize buttons for brightness control
  bsp_buttons_init(on_button_press);

  ESP_LOGI(TAG, "ZenClock started — BOOT=bright+, IO14=bright-");

  // app_main returns; FreeRTOS scheduler continues running
  // LVGL task and button task handle everything from here
}