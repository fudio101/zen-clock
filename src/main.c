#include <stdio.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "t_display_s3.h"
#include "lvgl.h"
#include "ui.h"

#define TAG "ZenClock"

void app_main(void)
{
  // Initialize LCD + LVGL port (backlight off initially)
  static lv_display_t *disp_handle;
  lcd_init(&disp_handle, false);

  // Initialize SquareLine Studio UI
  lvgl_port_lock(0);
  ui_init();
  lvgl_port_unlock();

  // Fade in backlight smoothly over 2 seconds
  lcd_set_brightness(100, 2000);

  ESP_LOGI(TAG, "ZenClock started — LVGL display active");

  // Main loop
  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}