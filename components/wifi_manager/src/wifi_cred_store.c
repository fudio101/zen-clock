// SPDX-License-Identifier: MIT
// ZenClock — NVS-based WiFi credential storage + provisioning

#include "wifi_priv.h"
#include "wifi_manager.h"
#include "wifi_credentials.gen.h"

#include <esp_log.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <string.h>
#include <stdio.h>

static const char *TAG = "WiFiCreds";

#define NVS_NAMESPACE "wifi_creds"
#define NVS_KEY_COUNT "count"
#define NVS_KEY_HASH "cred_hash"

static nvs_handle_t s_nvs;

// ============================================================
// Init
// ============================================================

esp_err_t wifi_cred_store_init(void)
{
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_LOGW(TAG, "NVS partition needs erase");
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &s_nvs);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(ret));
  }
  return ret;
}

// ============================================================
// Provision — write compiled credentials to NVS if hash changed
// ============================================================

esp_err_t wifi_cred_store_provision(void)
{
  uint32_t stored_hash = 0;
  esp_err_t ret = nvs_get_u32(s_nvs, NVS_KEY_HASH, &stored_hash);

  if (ret == ESP_OK && stored_hash == WIFI_CRED_HASH && WIFI_CRED_COUNT > 0)
  {
    ESP_LOGI(TAG, "Credentials unchanged (hash=0x%08" PRIx32 "), skipping provision",
             (uint32_t)WIFI_CRED_HASH);
    return ESP_OK;
  }

  ESP_LOGI(TAG, "Provisioning %d credentials (hash=0x%08" PRIx32 ")",
           WIFI_CRED_COUNT, (uint32_t)WIFI_CRED_HASH);

  // Clear old entries
  uint8_t old_count = 0;
  nvs_get_u8(s_nvs, NVS_KEY_COUNT, &old_count);
  for (int i = 0; i < old_count; i++)
  {
    char key_s[12], key_p[12];
    snprintf(key_s, sizeof(key_s), "ssid_%d", i);
    snprintf(key_p, sizeof(key_p), "pass_%d", i);
    nvs_erase_key(s_nvs, key_s);
    nvs_erase_key(s_nvs, key_p);
  }

  // Write new entries
#if WIFI_CRED_COUNT > 0
  for (int i = 0; i < WIFI_CRED_COUNT; i++)
  {
    char key_s[12], key_p[12];
    snprintf(key_s, sizeof(key_s), "ssid_%d", i);
    snprintf(key_p, sizeof(key_p), "pass_%d", i);
    nvs_set_str(s_nvs, key_s, WIFI_CRED_LIST[i].ssid);
    nvs_set_str(s_nvs, key_p, WIFI_CRED_LIST[i].pass);
  }
#endif

  nvs_set_u8(s_nvs, NVS_KEY_COUNT, (uint8_t)WIFI_CRED_COUNT);
  nvs_set_u32(s_nvs, NVS_KEY_HASH, WIFI_CRED_HASH);
  nvs_commit(s_nvs);

  ESP_LOGI(TAG, "Provisioned %d credentials", WIFI_CRED_COUNT);
  return ESP_OK;
}

// ============================================================
// Load all from NVS into hash map
// ============================================================

int wifi_cred_store_load_all(ssid_map_t *map)
{
  ssid_map_init(map);

  uint8_t count = 0;
  esp_err_t ret = nvs_get_u8(s_nvs, NVS_KEY_COUNT, &count);
  if (ret != ESP_OK || count == 0)
  {
    ESP_LOGW(TAG, "No credentials in NVS");
    return 0;
  }

  int loaded = 0;
  for (int i = 0; i < count; i++)
  {
    char key_s[12], key_p[12];
    char ssid[SSID_MAX_LEN] = {0};
    char pass[PASS_MAX_LEN] = {0};
    size_t ssid_len = sizeof(ssid);
    size_t pass_len = sizeof(pass);

    snprintf(key_s, sizeof(key_s), "ssid_%d", i);
    snprintf(key_p, sizeof(key_p), "pass_%d", i);

    if (nvs_get_str(s_nvs, key_s, ssid, &ssid_len) == ESP_OK)
    {
      nvs_get_str(s_nvs, key_p, pass, &pass_len); // password may be empty
      ssid_map_put(map, ssid, pass);
      loaded++;
      ESP_LOGI(TAG, "  [%d] SSID: \"%s\"", i, ssid);
    }
  }

  ESP_LOGI(TAG, "Loaded %d credentials into hash map", loaded);
  return loaded;
}

// ============================================================
// Runtime add/remove
// ============================================================

esp_err_t wifi_cred_store_add(const char *ssid, const char *password)
{
  uint8_t count = 0;
  nvs_get_u8(s_nvs, NVS_KEY_COUNT, &count);

  if (count >= WIFI_MAX_CREDENTIALS)
  {
    ESP_LOGE(TAG, "Max credentials (%d) reached", WIFI_MAX_CREDENTIALS);
    return ESP_ERR_NO_MEM;
  }

  // Check if already exists
  for (int i = 0; i < count; i++)
  {
    char key_s[12];
    char existing[SSID_MAX_LEN] = {0};
    size_t len = sizeof(existing);
    snprintf(key_s, sizeof(key_s), "ssid_%d", i);
    if (nvs_get_str(s_nvs, key_s, existing, &len) == ESP_OK)
    {
      if (strcmp(existing, ssid) == 0)
      {
        // Update password
        char key_p[12];
        snprintf(key_p, sizeof(key_p), "pass_%d", i);
        esp_err_t ret = nvs_set_str(s_nvs, key_p, password ? password : "");
        if (ret != ESP_OK)
        {
          ESP_LOGE(TAG, "Failed to update password: %s", esp_err_to_name(ret));
          return ret;
        }
        nvs_commit(s_nvs);
        ESP_LOGI(TAG, "Updated password for \"%s\"", ssid);
        return ESP_OK;
      }
    }
  }

  // Add new
  char key_s[12], key_p[12];
  snprintf(key_s, sizeof(key_s), "ssid_%d", count);
  snprintf(key_p, sizeof(key_p), "pass_%d", count);

  esp_err_t ret = nvs_set_str(s_nvs, key_s, ssid);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to write SSID: %s", esp_err_to_name(ret));
    return ret;
  }
  ret = nvs_set_str(s_nvs, key_p, password ? password : "");
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to write password: %s", esp_err_to_name(ret));
    return ret;
  }
  nvs_set_u8(s_nvs, NVS_KEY_COUNT, count + 1);
  nvs_commit(s_nvs);

  ESP_LOGI(TAG, "Added credential [%d]: \"%s\"", count, ssid);
  return ESP_OK;
}

esp_err_t wifi_cred_store_remove(const char *ssid)
{
  uint8_t count = 0;
  nvs_get_u8(s_nvs, NVS_KEY_COUNT, &count);

  for (int i = 0; i < count; i++)
  {
    char key_s[12];
    char existing[SSID_MAX_LEN] = {0};
    size_t len = sizeof(existing);
    snprintf(key_s, sizeof(key_s), "ssid_%d", i);
    if (nvs_get_str(s_nvs, key_s, existing, &len) == ESP_OK && strcmp(existing, ssid) == 0)
    {
      // Mark as deleted — erase keys, decrement count, single commit
      char key_p[12];
      snprintf(key_p, sizeof(key_p), "pass_%d", i);
      nvs_erase_key(s_nvs, key_s);
      nvs_erase_key(s_nvs, key_p);

      esp_err_t ret = nvs_set_u8(s_nvs, NVS_KEY_COUNT, count - 1);
      if (ret != ESP_OK)
      {
        ESP_LOGE(TAG, "Failed to update count: %s", esp_err_to_name(ret));
        return ret;
      }
      ret = nvs_commit(s_nvs);
      if (ret != ESP_OK)
      {
        ESP_LOGE(TAG, "NVS commit failed: %s", esp_err_to_name(ret));
        return ret;
      }

      ESP_LOGI(TAG, "Removed credential: \"%s\"", ssid);
      return ESP_OK;
    }
  }

  ESP_LOGW(TAG, "Credential not found: \"%s\"", ssid);
  return ESP_ERR_NOT_FOUND;
}

void wifi_cred_store_deinit(void)
{
  nvs_close(s_nvs);
  ESP_LOGI(TAG, "NVS credential store closed");
}
