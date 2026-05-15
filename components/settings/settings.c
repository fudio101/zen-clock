// SPDX-License-Identifier: MIT
#include "settings.h"
#include <esp_log.h>
#include <nvs_flash.h>
#include <nvs.h>

static const char *const tag = "Settings";
static const char *nvs_namespace = "zenclock";
static const char *key_theme_light = "theme_light";
static const char *key_brightness = "brightness";
static const char *key_sleep_h = "sleep_h";
static const char *key_sleep_m = "sleep_m";
static const char *key_sleep_s = "sleep_s";

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void settings_init(void)
{
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_LOGW(tag, "NVS needs to be erased. Erasing...");
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  ESP_LOGI(tag, "NVS initialized.");
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
bool settings_get_theme_light(void)
{
  nvs_handle_t my_handle;
  esp_err_t err = nvs_open(nvs_namespace, NVS_READONLY, &my_handle);
  if (err != ESP_OK)
  {
    // Namespace doesn't exist yet, return default (dark theme)
    return false;
  }

  uint8_t is_light = 0; // 0 = dark, 1 = light
  err = nvs_get_u8(my_handle, key_theme_light, &is_light);
  switch (err)
  {
  case ESP_OK:
    ESP_LOGI(tag, "Loaded theme config: %s", is_light ? "Light" : "Dark");
    break;
  case ESP_ERR_NVS_NOT_FOUND:
    ESP_LOGD(tag, "Theme config not found, using default (Dark)");
    break;
  default:
    ESP_LOGE(tag, "Error reading theme config (%s)", esp_err_to_name(err));
    break;
  }

  nvs_close(my_handle);
  return is_light != 0;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void settings_set_theme_light(bool is_light)
{
  nvs_handle_t my_handle;
  esp_err_t err = nvs_open(nvs_namespace, NVS_READWRITE, &my_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(tag, "Error opening NVS handle (%s)", esp_err_to_name(err));
    return;
  }

  err = nvs_set_u8(my_handle, key_theme_light, is_light ? 1 : 0);
  if (err != ESP_OK)
  {
    ESP_LOGE(tag, "Error saving theme config (%s)", esp_err_to_name(err));
  }
  else
  {
    err = nvs_commit(my_handle);
    if (err != ESP_OK)
    {
      ESP_LOGE(tag, "Error committing NVS (%s)", esp_err_to_name(err));
    }
    else
    {
      ESP_LOGI(tag, "Saved theme config: %s", is_light ? "Light" : "Dark");
    }
  }
  nvs_close(my_handle);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
uint8_t settings_get_brightness(void)
{
  nvs_handle_t my_handle;
  esp_err_t err = nvs_open(nvs_namespace, NVS_READONLY, &my_handle);
  if (err != ESP_OK)
  {
    return 100; // default: full brightness
  }

  uint8_t val = 100;
  err = nvs_get_u8(my_handle, key_brightness, &val);
  switch (err)
  {
  case ESP_OK:
    ESP_LOGI(tag, "Loaded brightness: %d%%", val);
    break;
  case ESP_ERR_NVS_NOT_FOUND:
    ESP_LOGD(tag, "Brightness not found, using default (100%%)");
    break;
  default:
    ESP_LOGE(tag, "Error reading brightness (%s)", esp_err_to_name(err));
    break;
  }

  nvs_close(my_handle);
  return val;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void settings_set_brightness(uint8_t percent)
{
  if (percent > 100)
  {
    percent = 100;
  }

  nvs_handle_t my_handle;
  esp_err_t err = nvs_open(nvs_namespace, NVS_READWRITE, &my_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(tag, "Error opening NVS handle (%s)", esp_err_to_name(err));
    return;
  }

  err = nvs_set_u8(my_handle, key_brightness, percent);
  if (err != ESP_OK)
  {
    ESP_LOGE(tag, "Error saving brightness (%s)", esp_err_to_name(err));
  }
  else
  {
    err = nvs_commit(my_handle);
    if (err != ESP_OK)
    {
      ESP_LOGE(tag, "Error committing NVS (%s)", esp_err_to_name(err));
    }
    else
    {
      ESP_LOGI(tag, "Saved brightness: %d%%", percent);
    }
  }
  nvs_close(my_handle);
}

// --- Sleep timeout helpers (H/M/S) ---
// Pattern identical to brightness: open READONLY/READWRITE, get/set u8, close.

static uint8_t get_sleep_component(const char *key, uint8_t default_val) // NOLINT
{
  nvs_handle_t h;
  if (nvs_open(nvs_namespace, NVS_READONLY, &h) != ESP_OK)
  {
    return default_val;
  }
  uint8_t val = default_val;
  nvs_get_u8(h, key, &val);
  nvs_close(h);
  return val;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static void set_sleep_component(const char *key, uint8_t val, uint8_t max_val)
{
  if (val > max_val)
  {
    val = max_val;
  }
  nvs_handle_t h;
  esp_err_t err = nvs_open(nvs_namespace, NVS_READWRITE, &h);
  if (err != ESP_OK)
  {
    ESP_LOGE(tag, "Error opening NVS (%s)", esp_err_to_name(err));
    return;
  }
  err = nvs_set_u8(h, key, val);
  if (err == ESP_OK)
  {
    nvs_commit(h);
  }
  else
  {
    ESP_LOGE(tag, "Error saving %s (%s)", key, esp_err_to_name(err));
  }
  nvs_close(h);
}

uint8_t settings_get_sleep_h(void)
{
  return get_sleep_component(key_sleep_h, 0);
}
uint8_t settings_get_sleep_m(void)
{
  return get_sleep_component(key_sleep_m, 0);
}
uint8_t settings_get_sleep_s(void)
{
  return get_sleep_component(key_sleep_s, 0);
}

void settings_set_sleep_h(uint8_t h)
{
  set_sleep_component(key_sleep_h, h, 23);
}
void settings_set_sleep_m(uint8_t m)
{
  set_sleep_component(key_sleep_m, m, 59);
}
void settings_set_sleep_s(uint8_t s)
{
  set_sleep_component(key_sleep_s, s, 59);
}
