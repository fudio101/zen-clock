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
// Battery monitoring task — updates header every 30 seconds
// ============================================================
#define BATTERY_UPDATE_INTERVAL_MS 30000

static void battery_update_task(void *arg)
{
  char buf[16];

  // Short initial delay to let UI settle
  vTaskDelay(pdMS_TO_TICKS(2000));

  for (;;)
  {
    int pct = bsp_battery_get_percentage();
    bool usb = bsp_battery_usb_connected();

    lvgl_port_lock(0);

    if (pct >= 0)
    {
      // Update percentage text (number only)
      snprintf(buf, sizeof(buf), "%d%%", pct);
      lv_label_set_text(ui_bat_pct_label, buf);

      // Update icon based on USB / charge level
      if (usb)
      {
        lv_label_set_text(ui_bat_icon_label, LV_SYMBOL_CHARGE);
      }
      else if (pct > 75)
      {
        lv_label_set_text(ui_bat_icon_label, LV_SYMBOL_BATTERY_FULL);
      }
      else if (pct > 50)
      {
        lv_label_set_text(ui_bat_icon_label, LV_SYMBOL_BATTERY_3);
      }
      else if (pct > 25)
      {
        lv_label_set_text(ui_bat_icon_label, LV_SYMBOL_BATTERY_2);
      }
      else if (pct > 5)
      {
        lv_label_set_text(ui_bat_icon_label, LV_SYMBOL_BATTERY_1);
      }
      else
      {
        lv_label_set_text(ui_bat_icon_label, LV_SYMBOL_BATTERY_EMPTY);
      }

      ESP_LOGI(TAG, "Battery: %d%% (%s)", pct, usb ? "USB" : "BATT");
    }
    else
    {
      // No battery connected or ADC error
      lv_label_set_text(ui_bat_pct_label, "N/A");
      lv_label_set_text(ui_bat_icon_label, LV_SYMBOL_BATTERY_EMPTY);
    }

    // Re-align icon to stay left of percentage (text width may have changed)
    lv_obj_align_to(ui_bat_icon_label, ui_bat_pct_label, LV_ALIGN_OUT_LEFT_MID, -4, 0);

    lvgl_port_unlock();

    vTaskDelay(pdMS_TO_TICKS(BATTERY_UPDATE_INTERVAL_MS));
  }
}

// ============================================================
// Application entry point
// ============================================================
void app_main(void)
{
  // Initialize BSP (LCD + LVGL port, backlight off initially)
  static lv_display_t *disp_handle;
  bsp_display_init(&disp_handle, false);

  // Initialize UI (hand-written LVGL widgets)
  lvgl_port_lock(0);
  ui_init();
  lvgl_port_unlock();

  // Fade in backlight smoothly over 2 seconds
  bsp_display_set_brightness(100, 2000);

  // Initialize buttons for brightness control
  bsp_buttons_init(on_button_press);

  // Start battery monitoring task
  xTaskCreate(battery_update_task, "bat_mon", 4096, NULL, 2, NULL);

  ESP_LOGI(TAG, "ZenClock started — BOOT=bright+, IO14=bright-");

  // app_main returns; FreeRTOS scheduler continues running
  // LVGL task, button task, and battery task handle everything from here
}