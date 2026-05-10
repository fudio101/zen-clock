// SPDX-License-Identifier: MIT
// ZenClock — Status bar (battery icon + percentage)

#include "status_bar.h"
#include "bsp.h"

#include <stdio.h>
#include <esp_log.h>

static const char *TAG = "StatusBar";

// ============================================================
// Private state
// ============================================================
static lv_obj_t *s_bat_icon = NULL;
static lv_obj_t *s_bat_pct = NULL;

// ============================================================
// Timer callback — runs inside lv_timer_handler() on LVGL task
// ============================================================
static void battery_timer_cb(lv_timer_t *timer)
{
  (void)timer;
  char buf[16];

  int pct = bsp_battery_get_percentage();
  bool usb = bsp_battery_usb_connected();

  if (pct >= 0)
  {
    // Update percentage text (number only)
    snprintf(buf, sizeof(buf), "%d%%", pct);
    lv_label_set_text(s_bat_pct, buf);

    // Update icon based on USB / charge level
    if (usb)
    {
      lv_label_set_text(s_bat_icon, LV_SYMBOL_CHARGE);
    }
    else if (pct > 75)
    {
      lv_label_set_text(s_bat_icon, LV_SYMBOL_BATTERY_FULL);
    }
    else if (pct > 50)
    {
      lv_label_set_text(s_bat_icon, LV_SYMBOL_BATTERY_3);
    }
    else if (pct > 25)
    {
      lv_label_set_text(s_bat_icon, LV_SYMBOL_BATTERY_2);
    }
    else if (pct > 5)
    {
      lv_label_set_text(s_bat_icon, LV_SYMBOL_BATTERY_1);
    }
    else
    {
      lv_label_set_text(s_bat_icon, LV_SYMBOL_BATTERY_EMPTY);
    }

    ESP_LOGI(TAG, "Battery: %d%% (%s)", pct, usb ? "USB" : "BATT");
  }
  else
  {
    // No battery connected or ADC error
    lv_label_set_text(s_bat_pct, "N/A");
    lv_label_set_text(s_bat_icon, LV_SYMBOL_BATTERY_EMPTY);
  }

  // Re-align icon to stay left of percentage (text width may have changed)
  lv_obj_align_to(s_bat_icon, s_bat_pct, LV_ALIGN_OUT_LEFT_MID, -4, 0);
}

// ============================================================
// Public API
// ============================================================
void status_bar_create(lv_obj_t *parent)
{
  // --- Battery percentage (top-right corner) ---
  s_bat_pct = lv_label_create(parent);
  lv_obj_set_width(s_bat_pct, LV_SIZE_CONTENT);
  lv_obj_set_height(s_bat_pct, LV_SIZE_CONTENT);
  lv_obj_align(s_bat_pct, LV_ALIGN_TOP_RIGHT, -8, 4);
  lv_label_set_text(s_bat_pct, "--%");

  // --- Battery icon (left of percentage, with gap) ---
  s_bat_icon = lv_label_create(parent);
  lv_obj_set_width(s_bat_icon, LV_SIZE_CONTENT);
  lv_obj_set_height(s_bat_icon, LV_SIZE_CONTENT);
  lv_obj_align_to(s_bat_icon, s_bat_pct, LV_ALIGN_OUT_LEFT_MID, -4, 0);
  lv_label_set_text(s_bat_icon, LV_SYMBOL_BATTERY_FULL);

  // --- LVGL timer: update every 30 seconds ---
  lv_timer_t *timer = lv_timer_create(battery_timer_cb, 30000, NULL);
  lv_timer_ready(timer); // Fire immediately on first tick
}
