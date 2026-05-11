// SPDX-License-Identifier: MIT
#include "settings.h"
#include <esp_log.h>
#include <nvs_flash.h>
#include <nvs.h>

static const char *TAG = "Settings";
static const char *NVS_NAMESPACE = "zenclock";
static const char *KEY_THEME_LIGHT = "theme_light";

void settings_init(void)
{
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_LOGW(TAG, "NVS needs to be erased. Erasing...");
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  ESP_LOGI(TAG, "NVS initialized.");
}

bool settings_get_theme_light(void)
{
  nvs_handle_t my_handle;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &my_handle);
  if (err != ESP_OK)
  {
    // Namespace doesn't exist yet, return default (dark theme)
    return false;
  }

  uint8_t is_light = 0; // 0 = dark, 1 = light
  err = nvs_get_u8(my_handle, KEY_THEME_LIGHT, &is_light);
  switch (err)
  {
  case ESP_OK:
    ESP_LOGI(TAG, "Loaded theme config: %s", is_light ? "Light" : "Dark");
    break;
  case ESP_ERR_NVS_NOT_FOUND:
    ESP_LOGD(TAG, "Theme config not found, using default (Dark)");
    break;
  default:
    ESP_LOGE(TAG, "Error reading theme config (%s)", esp_err_to_name(err));
    break;
  }

  nvs_close(my_handle);
  return is_light != 0;
}

void settings_set_theme_light(bool is_light)
{
  nvs_handle_t my_handle;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Error opening NVS handle (%s)", esp_err_to_name(err));
    return;
  }

  err = nvs_set_u8(my_handle, KEY_THEME_LIGHT, is_light ? 1 : 0);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Error saving theme config (%s)", esp_err_to_name(err));
  }
  else
  {
    err = nvs_commit(my_handle);
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Error committing NVS (%s)", esp_err_to_name(err));
    }
    else
    {
      ESP_LOGI(TAG, "Saved theme config: %s", is_light ? "Light" : "Dark");
    }
  }
  nvs_close(my_handle);
}
