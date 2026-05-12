// SPDX-License-Identifier: MIT
// ZenClock — BLE Provisioning via espressif/network_provisioning ^1.2.4
//
// Security 1 (ECDH + SHA-256), device name "PROV_ZenClock_XXYY".
// Empty PoP (no PIN required) — works with Espressif BLE Prov app.
//
// ⚠️  IMPORTANT: Always call wifi_manager_stop() before ble_provisioning_start()
//     to avoid conflict with network_prov_mgr's internal esp_wifi_connect() calls.
//
// ⚠️  IMPORTANT: Verify NETWORK_PROV_CRED_RECV event data type against the
//     installed network_provisioning/manager.h header before building.

#include "ble_provisioning.h"

#include <string.h>
#include <esp_log.h>
#include <esp_event.h>
#include <esp_wifi.h>
#include <esp_mac.h>
#include <esp_bt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// network_provisioning ^1.2.4 — IDF v6 replacement for wifi_provisioning
#include "network_provisioning/manager.h"
#include "network_provisioning/scheme_ble.h"

static const char *TAG = "BLEProv";

static ble_prov_cb_t s_callback = NULL;
static bool s_active = false;
static bool s_mem_freed = false;

// ============================================================
// Device name builder
// ============================================================

static void build_device_name(char *buf, size_t len)
{
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  snprintf(buf, len, "PROV_ZenClock_%02X%02X", mac[4], mac[5]);
}

// ============================================================
// network_provisioning event handler
// ============================================================

static void prov_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
  if (base != NETWORK_PROV_EVENT)
  {
    return;
  }

  switch (id)
  {
  case NETWORK_PROV_START:
    ESP_LOGI(TAG, "BLE advertisement started");
    s_active = true;
    if (s_callback)
      s_callback(BLE_PROV_STARTED, NULL, NULL);
    break;

  case NETWORK_PROV_WIFI_CRED_RECV:
  {
    wifi_sta_config_t *cfg = (wifi_sta_config_t *)data;
    const char *ssid = (const char *)cfg->ssid;
    const char *pass = (const char *)cfg->password;
    ESP_LOGI(TAG, "Credentials received: SSID=\"%s\"", ssid);
    if (s_callback)
      s_callback(BLE_PROV_CRED_RECEIVED, ssid, pass);
    break;
  }

  case NETWORK_PROV_WIFI_CRED_SUCCESS:
    ESP_LOGI(TAG, "Credential verification succeeded");
    break;

  case NETWORK_PROV_WIFI_CRED_FAIL:
  {
    network_prov_wifi_sta_fail_reason_t *reason = (network_prov_wifi_sta_fail_reason_t *)data;
    ESP_LOGW(TAG, "Credential verification failed (reason=%d)", reason ? (int)*reason : -1);
    if (s_callback)
      s_callback(BLE_PROV_FAILED, NULL, NULL);
    break;
  }

  case NETWORK_PROV_END:
    ESP_LOGI(TAG, "Provisioning ended");
    s_active = false;
    if (s_callback)
      s_callback(BLE_PROV_SUCCESS, NULL, NULL);
    break;

  default:
    break;
  }
}

// ============================================================
// Public API
// ============================================================

esp_err_t ble_provisioning_init(ble_prov_cb_t callback)
{
  s_callback = callback;

  esp_err_t ret = esp_event_handler_register(NETWORK_PROV_EVENT, ESP_EVENT_ANY_ID, prov_event_handler, NULL);

  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to register prov event handler: %s", esp_err_to_name(ret));
  }
  return ret;
}

esp_err_t ble_provisioning_start(void)
{
  if (s_mem_freed)
  {
    ESP_LOGE(TAG, "BLE memory already released — cannot start provisioning again");
    return ESP_ERR_INVALID_STATE;
  }

  char device_name[32];
  build_device_name(device_name, sizeof(device_name));
  ESP_LOGI(TAG, "Starting BLE provisioning: device_name=\"%s\"", device_name);

  network_prov_mgr_config_t config = {
      .scheme = network_prov_scheme_ble,
      .scheme_event_handler = NETWORK_PROV_EVENT_HANDLER_NONE,
  };

  esp_err_t ret = network_prov_mgr_init(config);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "network_prov_mgr_init failed: %s", esp_err_to_name(ret));
    return ret;
  }

  // Security 1 (ECDH + SHA-256), empty PoP — no PIN required.
  // Security 2 (SRP6a) requires pre-generated salt/verifier; Security 1 is
  // sufficient for home provisioning and is fully supported by the Espressif app.
  ret = network_prov_mgr_start_provisioning(NETWORK_PROV_SECURITY_1,
                                            (void *)"",  // PoP: empty string = no PIN
                                            device_name, // service_name: BLE advertisement name
                                            NULL);       // service_key: not used for BLE

  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "network_prov_mgr_start_provisioning failed: %s", esp_err_to_name(ret));
    network_prov_mgr_deinit();
    return ret;
  }

  s_active = true;
  return ESP_OK;
}

esp_err_t ble_provisioning_stop(void)
{
  if (!s_active)
  {
    return ESP_OK;
  }

  network_prov_mgr_stop_provisioning();
  network_prov_mgr_deinit();
  s_active = false;
  ESP_LOGI(TAG, "BLE provisioning stopped");
  return ESP_OK;
}

bool ble_provisioning_is_active(void)
{
  return s_active;
}

void ble_provisioning_get_device_name(char *buf, size_t len)
{
  build_device_name(buf, len);
}

void ble_provisioning_release_memory(void)
{
  if (s_mem_freed)
  {
    return;
  }

  // Allow BLE stack to fully settle after deinit before releasing memory.
  // Releasing too early causes heap corruption.
  vTaskDelay(pdMS_TO_TICKS(200));

  ESP_LOGI(TAG, "Releasing BLE controller memory (~110KB)...");
  esp_bt_controller_disable();
  esp_bt_controller_deinit();
  esp_bt_mem_release(ESP_BT_MODE_BLE);
  s_mem_freed = true;
  ESP_LOGI(TAG, "BLE memory released. Free heap: %lu bytes", (unsigned long)esp_get_free_heap_size());
}
