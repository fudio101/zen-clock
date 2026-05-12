#include "app_handlers.h"

#include <esp_log.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "bsp.h"
#include "ui.h"
#include "wifi_manager.h"
#include "sntp_sync.h"
#include "status_bar.h"
#include "settings.h"
#include "ble_provisioning.h"
#include "prov_screen.h"
#include "esp_timer.h"

static const char *TAG = "ZenClock";

#define BRIGHTNESS_STEP 10

// ============================================================
// Button handler
// ============================================================

void on_button_press(int btn_id, bool pressed)
{
  if (!pressed)
  {
    return;
  }

  static int64_t last_boot_press_time = 0;
  int64_t now = esp_timer_get_time();

  uint8_t current = bsp_display_get_brightness();
  uint8_t target = current;

  if (btn_id == BSP_BTN_BOOT)
  {
    if (now - last_boot_press_time < 500000) // 500ms double-click
    {
      bool current_theme_light = settings_get_theme_light();
      settings_set_theme_light(!current_theme_light);
      lvgl_port_lock(0);
      ui_set_theme(!current_theme_light);
      lvgl_port_unlock();
      ESP_LOGI(TAG, "Theme toggled to %s", !current_theme_light ? "Light" : "Dark");
      last_boot_press_time = 0;
      return;
    }
    last_boot_press_time = now;

    target = (current <= 100 - BRIGHTNESS_STEP) ? current + BRIGHTNESS_STEP : 100;
  }
  else if (btn_id == BSP_BTN_IO14)
  {
    static int64_t last_io14_press_time = 0;

    if (now - last_io14_press_time < 500000) // 500ms double-click
    {
      ESP_LOGW(TAG, "IO14 double-click: resetting WiFi credentials -> BLE provisioning");
      last_io14_press_time = 0;
      wifi_manager_stop();
      wifi_manager_clear_credential();
      esp_err_t ret = ble_provisioning_start();
      if (ret == ESP_ERR_INVALID_STATE)
      {
        ESP_LOGW(TAG, "BLE memory released — rebooting into provisioning mode");
        vTaskDelay(pdMS_TO_TICKS(200));
        esp_restart();
      }
      return;
    }
    last_io14_press_time = now;

    target = (current >= BRIGHTNESS_STEP) ? current - BRIGHTNESS_STEP : 0;
  }

  bsp_display_set_brightness(target, 200);
  ESP_LOGI(TAG, "Brightness: %d%% -> %d%%", current, target);
}

// ============================================================
// SNTP sync callback
// ============================================================

void on_sntp_sync(sntp_sync_event_t event)
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
// BLE provisioning callback
// ============================================================

void on_ble_prov_event(ble_prov_event_t event, const char *ssid, const char *pass)
{
  switch (event)
  {
  case BLE_PROV_STARTED:
  {
    char dev_name[32];
    ble_provisioning_get_device_name(dev_name, sizeof(dev_name));
    ESP_LOGI(TAG, "BLE provisioning active: %s", dev_name);
    lvgl_port_lock(0);
    status_bar_set_wifi_status(WIFI_STATUS_PROVISIONING);
    prov_screen_show(dev_name);
    lvgl_port_unlock();
    break;
  }

  case BLE_PROV_CRED_RECEIVED:
    ESP_LOGI(TAG, "BLE credentials received: SSID=\"%s\"", ssid ? ssid : "");
    wifi_manager_set_credential(ssid, pass);
    break;

  case BLE_PROV_SUCCESS:
    ESP_LOGI(TAG, "BLE provisioning complete — starting WiFi");
    lvgl_port_lock(0);
    prov_screen_hide();
    lvgl_port_unlock();
    ble_provisioning_stop();
    ble_provisioning_release_memory();
    wifi_manager_start();
    break;

  case BLE_PROV_FAILED:
    ESP_LOGW(TAG, "BLE provisioning failed — bad credentials, waiting for retry");
    wifi_manager_clear_credential();
    {
      char dev_name[32];
      ble_provisioning_get_device_name(dev_name, sizeof(dev_name));
      lvgl_port_lock(0);
      prov_screen_show(dev_name);
      lvgl_port_unlock();
    }
    break;

  default:
    break;
  }
}

// ============================================================
// WiFi event callback
// ============================================================

void on_wifi_event(wifi_manager_event_t event)
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

  case WIFI_MGR_SCAN_DONE:
    ESP_LOGI(TAG, "WiFi scan complete");
    break;

  case WIFI_MGR_NO_CRED:
    ESP_LOGW(TAG, "No WiFi credential stored — starting BLE provisioning");
    wifi_manager_stop();
    lvgl_port_lock(0);
    status_bar_set_wifi_status(WIFI_STATUS_PROVISIONING);
    lvgl_port_unlock();
    ble_provisioning_start();
    break;

  case WIFI_MGR_DISCONNECTED:
  case WIFI_MGR_NO_MATCH:
  case WIFI_MGR_ALL_FAILED:
    ESP_LOGW(TAG, "WiFi unavailable (event=%d) — starting BLE provisioning", (int)event);
    wifi_manager_stop();
    lvgl_port_lock(0);
    status_bar_set_wifi_status(WIFI_STATUS_PROVISIONING);
    lvgl_port_unlock();
    ble_provisioning_start();
    break;
  }
}
