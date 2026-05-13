#include "app_handlers.h"

#include <esp_log.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "bsp.h"
#include "wifi_manager.h"
#include "sntp_sync.h"
#include "status_bar.h"
#include "ble_provisioning.h"
#include "prov_screen.h"
#include "nav.h"

static const char *TAG = "ZenClock";

// ============================================================
// Wi-Fi reset action (shared by emergency button + nav settings)
// ============================================================

static void do_reset_wifi(void)
{
  wifi_manager_stop();
  wifi_manager_clear_credential();
  const esp_err_t ret = ble_provisioning_start();
  if (ret == ESP_ERR_INVALID_STATE)
  {
    ESP_LOGW(TAG, "BLE memory released — rebooting into provisioning mode");
    vTaskDelay(pdMS_TO_TICKS(200));
    esp_restart();
  }
}

// ============================================================
// Nav callback registration
// ============================================================

void app_handlers_register_nav_callbacks(void)
{
  nav_register_reset_wifi_cb(do_reset_wifi);
}

// ============================================================
// Button handler
// ============================================================

void on_button_press(int btn_id, bsp_btn_event_t event)
{
  // Emergency: IO14 held ≥ 3s → reset WiFi + BLE provisioning (bypasses nav)
  if (event == BSP_BTN_EMERGENCY && btn_id == BSP_BTN_IO14)
  {
    ESP_LOGW(TAG, "Emergency: resetting WiFi → BLE provisioning");
    do_reset_wifi();
    return;
  }

  // Map button + event → nav action
  nav_action_t action;
  if (btn_id == BSP_BTN_BOOT)
  {
    action = (event == BSP_BTN_SHORT) ? NAV_ACTION_UP : NAV_ACTION_SELECT;
  }
  else
  {
    action = (event == BSP_BTN_SHORT) ? NAV_ACTION_DOWN : NAV_ACTION_BACK;
  }

  lvgl_port_lock(0);
  nav_handle_action(action);
  lvgl_port_unlock();
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
    char prov_pass[9];
    ble_provisioning_get_device_name(dev_name, sizeof(dev_name));
    ble_provisioning_get_password(prov_pass, sizeof(prov_pass));
    ESP_LOGI(TAG, "BLE provisioning active: %s", dev_name);
    lvgl_port_lock(0);
    status_bar_set_wifi_status(WIFI_STATUS_PROVISIONING);
    prov_screen_show(dev_name, prov_pass);
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
