#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "display/lcd_driver.h"
#include "ui/ui_manager.h"
#include "esp_log.h"

void app_main(void)
{
  // Initialize LCD display (I80 bus + ST7789 + backlight PWM)
  esp_lcd_panel_handle_t panel = lcd_driver_init();

  // Initialize UI — draw header, RAM info
  ui_init(panel);

  // Turn on backlight smoothly to 75% over 1 second after UI is drawn
  lcd_driver_set_backlight(75, 1000);

  // Main loop — update counter every second
  int counter = 0;
  while (1)
  {
    ui_update_counter(counter++);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}