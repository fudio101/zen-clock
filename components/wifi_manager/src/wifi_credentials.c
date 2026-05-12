// SPDX-License-Identifier: MIT
// ZenClock WiFi Manager — Single credential NVS storage
//
// NVS namespace: "wifi_cred"
// Keys: "ssid" (string), "pass" (string)
//
// NVS must already be initialized by settings_init() before any call here.

#include "wifi_manager.h"
#include "wifi_priv.h"

#include <string.h>
#include <esp_log.h>
#include <nvs.h>

static const char *TAG = "WiFiCred";
#define NVS_NAMESPACE "wifi_cred"
#define NVS_KEY_SSID "ssid"
#define NVS_KEY_PASS "pass"

// ============================================================
// Internal loader — called only from wifi_manager.c (wifi_task)
// ============================================================

bool wifi_cred_load(char *out_ssid, size_t ssid_len, char *out_pass, size_t pass_len)
{
  nvs_handle_t h;
  if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) != ESP_OK)
  {
    return false;
  }

  size_t sl = ssid_len;
  bool ok = (nvs_get_str(h, NVS_KEY_SSID, out_ssid, &sl) == ESP_OK && sl > 1);

  if (ok)
  {
    size_t pl = pass_len;
    esp_err_t ret = nvs_get_str(h, NVS_KEY_PASS, out_pass, &pl);
    if (ret != ESP_OK)
    {
      out_pass[0] = '\0'; // open network
    }
  }

  nvs_close(h);
  return ok;
}

// ============================================================
// Public API
// ============================================================

bool wifi_manager_has_credential(void)
{
  nvs_handle_t h;
  if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) != ESP_OK)
  {
    return false;
  }
  size_t len = SSID_MAX_LEN;
  char ssid[SSID_MAX_LEN];
  bool has = (nvs_get_str(h, NVS_KEY_SSID, ssid, &len) == ESP_OK && len > 1);
  nvs_close(h);
  return has;
}

esp_err_t wifi_manager_set_credential(const char *ssid, const char *pass)
{
  if (!ssid || ssid[0] == '\0')
  {
    return ESP_ERR_INVALID_ARG;
  }

  nvs_handle_t h;
  esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
  if (ret != ESP_OK)
  {
    return ret;
  }

  ret = nvs_set_str(h, NVS_KEY_SSID, ssid);
  if (ret == ESP_OK)
  {
    ret = nvs_set_str(h, NVS_KEY_PASS, pass ? pass : "");
  }
  if (ret == ESP_OK)
  {
    ret = nvs_commit(h);
  }

  nvs_close(h);
  ESP_LOGI(TAG, "Credential saved: \"%s\"", ssid);
  return ret;
}

esp_err_t wifi_manager_clear_credential(void)
{
  nvs_handle_t h;
  esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
  if (ret != ESP_OK)
  {
    return ret;
  }

  nvs_erase_key(h, NVS_KEY_SSID);
  nvs_erase_key(h, NVS_KEY_PASS);
  ret = nvs_commit(h);
  nvs_close(h);
  ESP_LOGI(TAG, "Credential cleared");
  return ret;
}
