#include <stdlib.h>
#include <time.h>
#include <esp_log.h>
#include "bsp.h"
#include "ui.h"
#include "wifi_manager.h"
#include "sntp_sync.h"
#include "status_bar.h"
#include "settings.h"
#include "esp_timer.h"

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

  static int64_t last_boot_press_time = 0;
  int64_t now = esp_timer_get_time();

  uint8_t current = bsp_display_get_brightness();
  uint8_t target = current;

  if (btn_id == BSP_BTN_BOOT)
  {
    if (now - last_boot_press_time < 500000) // 500ms double click
    {
      bool current_theme_light = settings_get_theme_light();
      settings_set_theme_light(!current_theme_light);
      lvgl_port_lock(0);
      ui_set_theme(!current_theme_light);
      lvgl_port_unlock();
      ESP_LOGI(TAG, "Theme toggled to %s", !current_theme_light ? "Light" : "Dark");
      last_boot_press_time = 0; // reset
      return;
    }
    last_boot_press_time = now;

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
// SNTP sync callback (called from sntp task)
// ============================================================
static void on_sntp_sync(sntp_sync_event_t event)
{
  switch (event)
  {
  case SNTP_EVENT_SYNCING:
    ESP_LOGI(TAG, "NTP syncing...");
    lvgl_port_lock(0);
    status_bar_set_sntp_status(SNTP_STATUS_SYNCING);
    lvgl_port_unlock();
    break;

  case SNTP_EVENT_SYNCED:
    ESP_LOGI(TAG, "NTP time synchronized!");
    lvgl_port_lock(0);
    status_bar_set_sntp_status(SNTP_STATUS_SYNCED);
    lvgl_port_unlock();
    break;

  case SNTP_EVENT_FAILED:
    ESP_LOGW(TAG, "NTP sync failed — clock may show wrong time");
    lvgl_port_lock(0);
    status_bar_set_sntp_status(SNTP_STATUS_FAILED);
    lvgl_port_unlock();
    break;
  }
}

// ============================================================
// WiFi event callback (called from wifi_connect task)
// ============================================================
static void on_wifi_event(wifi_manager_event_t event)
{
  switch (event)
  {
  case WIFI_MGR_SCANNING:
    lvgl_port_lock(0);
    status_bar_set_wifi_status(WIFI_STATUS_SCANNING);
    lvgl_port_unlock();
    break;

  case WIFI_MGR_CONNECTING:
    lvgl_port_lock(0);
    status_bar_set_wifi_status(WIFI_STATUS_CONNECTING);
    lvgl_port_unlock();
    break;

  case WIFI_MGR_GOT_IP:
    ESP_LOGI(TAG, "WiFi got IP — verifying internet...");
    lvgl_port_lock(0);
    status_bar_set_wifi_status(WIFI_STATUS_VERIFYING);
    lvgl_port_unlock();
    break;

  case WIFI_MGR_CONNECTED:
    ESP_LOGI(TAG, "WiFi verified online — starting NTP sync...");
    lvgl_port_lock(0);
    status_bar_set_wifi_status(WIFI_STATUS_CONNECTED);
    lvgl_port_unlock();
    sntp_sync_start(on_sntp_sync);
    break;

  case WIFI_MGR_DISCONNECTED:
    ESP_LOGW(TAG, "WiFi disconnected");
    lvgl_port_lock(0);
    status_bar_set_wifi_status(WIFI_STATUS_DISCONNECTED);
    lvgl_port_unlock();
    break;

  case WIFI_MGR_RECONNECTING:
    ESP_LOGW(TAG, "WiFi reconnecting...");
    lvgl_port_lock(0);
    status_bar_set_wifi_status(WIFI_STATUS_CONNECTING);
    lvgl_port_unlock();
    break;

  case WIFI_MGR_SCAN_DONE:
    ESP_LOGI(TAG, "WiFi scan complete");
    break;

  case WIFI_MGR_NO_MATCH:
    ESP_LOGW(TAG, "No stored WiFi network found nearby");
    lvgl_port_lock(0);
    status_bar_set_wifi_status(WIFI_STATUS_DISCONNECTED);
    lvgl_port_unlock();
    break;

  case WIFI_MGR_ALL_FAILED:
    ESP_LOGW(TAG, "Scan cycle failed — retrying...");
    lvgl_port_lock(0);
    status_bar_set_wifi_status(WIFI_STATUS_DISCONNECTED);
    lvgl_port_unlock();
    break;
  }
}

// ============================================================
// Application entry point
// ============================================================
void app_main(void)
{
  // Set timezone to UTC+7 (POSIX convention: "UTC-7" = ahead of UTC)
  setenv("TZ", "UTC-7", 1);
  tzset();

  // Initialize BSP (LCD + LVGL port, backlight off initially)
  static lv_display_t *disp_handle;
  bsp_display_init(&disp_handle, false);

  // Initialize NVS and load settings
  settings_init();
  bool is_light = settings_get_theme_light();

  // Initialize UI (self-contained: creates all widgets + timers)
  lvgl_port_lock(0);
  ui_init(is_light);
  lvgl_port_unlock();

  // Fade in backlight smoothly over 2 seconds
  bsp_display_set_brightness(100, 2000);

  // Initialize buttons for brightness control
  bsp_buttons_init(on_button_press);

  // WiFi auto-connect (async: scan → match → connect → NTP sync)
  wifi_manager_init();
  wifi_manager_set_callback(on_wifi_event);
  wifi_manager_start();

  ESP_LOGI(TAG, "ZenClock started — BOOT=bright+, IO14=bright-");

  // app_main returns; FreeRTOS scheduler continues running
  // LVGL task handles rendering + clock/battery timer callbacks
  // WiFi task handles scan/connect in background
}