// SPDX-License-Identifier: MIT
// ZenClock — Status bar (SNTP indicator + WiFi indicator + battery icon + percentage)

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
static lv_obj_t *s_wifi_icon = NULL;
static lv_obj_t *s_sntp_icon = NULL;

// ============================================================
// Re-align the status bar chain (right-to-left)
//
// Layout: [SNTP] [WiFi] [BatIcon] [BatPct]  ← screen edge
// ============================================================
static void realign_chain(void)
{
  // Battery icon stays left of battery percentage
  lv_obj_align_to(s_bat_icon, s_bat_pct, LV_ALIGN_OUT_LEFT_MID, -4, 0);

  // WiFi icon stays left of battery icon
  if (s_wifi_icon)
  {
    lv_obj_align_to(s_wifi_icon, s_bat_icon, LV_ALIGN_OUT_LEFT_MID, -6, 0);
  }

  // SNTP icon stays left of WiFi icon
  if (s_sntp_icon && s_wifi_icon)
  {
    lv_obj_align_to(s_sntp_icon, s_wifi_icon, LV_ALIGN_OUT_LEFT_MID, -6, 0);
  }
}

// ============================================================
// Battery timer callback — runs inside lv_timer_handler()
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

  // Re-align entire chain (text width may have changed)
  realign_chain();
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

  // --- WiFi icon (left of battery icon) ---
  s_wifi_icon = lv_label_create(parent);
  lv_obj_set_width(s_wifi_icon, LV_SIZE_CONTENT);
  lv_obj_set_height(s_wifi_icon, LV_SIZE_CONTENT);
  lv_obj_align_to(s_wifi_icon, s_bat_icon, LV_ALIGN_OUT_LEFT_MID, -6, 0);
  lv_obj_set_style_text_opa(s_wifi_icon, LV_OPA_40, 0); // Dim initially
  lv_label_set_text(s_wifi_icon, LV_SYMBOL_WIFI);

  // --- SNTP icon (left of WiFi icon) ---
  s_sntp_icon = lv_label_create(parent);
  lv_obj_set_width(s_sntp_icon, LV_SIZE_CONTENT);
  lv_obj_set_height(s_sntp_icon, LV_SIZE_CONTENT);
  lv_obj_align_to(s_sntp_icon, s_wifi_icon, LV_ALIGN_OUT_LEFT_MID, -6, 0);
  lv_obj_set_style_text_opa(s_sntp_icon, LV_OPA_40, 0); // Dim initially
  lv_label_set_text(s_sntp_icon, LV_SYMBOL_REFRESH);

  // --- LVGL timer: update battery every 30 seconds ---
  lv_timer_t *timer = lv_timer_create(battery_timer_cb, 30000, NULL);
  lv_timer_ready(timer); // Fire immediately on first tick
}

void status_bar_set_wifi_status(wifi_status_t status)
{
  if (!s_wifi_icon)
  {
    return;
  }

  switch (status)
  {
  case WIFI_STATUS_DISCONNECTED:
    lv_label_set_text(s_wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_opa(s_wifi_icon, LV_OPA_40, 0);
    lv_obj_remove_local_style_prop(s_wifi_icon, LV_STYLE_TEXT_COLOR, 0);
    break;

  case WIFI_STATUS_SCANNING:
    lv_label_set_text(s_wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_opa(s_wifi_icon, LV_OPA_70, 0);
    lv_obj_remove_local_style_prop(s_wifi_icon, LV_STYLE_TEXT_COLOR, 0);
    break;

  case WIFI_STATUS_CONNECTING:
    lv_label_set_text(s_wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_opa(s_wifi_icon, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(s_wifi_icon, lv_palette_main(LV_PALETTE_ORANGE), 0);
    break;

  case WIFI_STATUS_VERIFYING:
    lv_label_set_text(s_wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_opa(s_wifi_icon, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(s_wifi_icon, lv_palette_main(LV_PALETTE_LIGHT_BLUE), 0);
    break;

  case WIFI_STATUS_CONNECTED:
    lv_label_set_text(s_wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_opa(s_wifi_icon, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(s_wifi_icon, lv_palette_main(LV_PALETTE_GREEN), 0);
    break;
  }

  // Re-align entire chain after icon change
  realign_chain();
}

void status_bar_set_sntp_status(sntp_status_t status)
{
  if (!s_sntp_icon)
  {
    return;
  }

  switch (status)
  {
  case SNTP_STATUS_IDLE:
    lv_label_set_text(s_sntp_icon, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_opa(s_sntp_icon, LV_OPA_40, 0);
    lv_obj_remove_local_style_prop(s_sntp_icon, LV_STYLE_TEXT_COLOR, 0);
    break;

  case SNTP_STATUS_SYNCING:
    lv_label_set_text(s_sntp_icon, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_opa(s_sntp_icon, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(s_sntp_icon, lv_palette_main(LV_PALETTE_ORANGE), 0);
    break;

  case SNTP_STATUS_SYNCED:
    lv_label_set_text(s_sntp_icon, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_opa(s_sntp_icon, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(s_sntp_icon, lv_palette_main(LV_PALETTE_GREEN), 0);
    break;

  case SNTP_STATUS_FAILED:
    lv_label_set_text(s_sntp_icon, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_opa(s_sntp_icon, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(s_sntp_icon, lv_palette_main(LV_PALETTE_RED), 0);
    break;
  }

  // Re-align entire chain after icon change
  realign_chain();
}
