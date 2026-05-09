#include "display/lcd_driver.h"
#include "board_config.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_lcd_io_i80.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "lcd_driver";

/// Static IO handle for access via getter
static esp_lcd_panel_io_handle_t s_io_handle = NULL;

// -------------------------------------------------------
// LCD_MODULE_CMD_1: Init sequence for newer T-Display-S3
// panels (from LilyGo factory example)
// -------------------------------------------------------
typedef struct
{
  uint8_t cmd;
  uint8_t data[14];
  uint8_t len; // bit7 = delay flag
} lcd_cmd_t;

static const lcd_cmd_t lcd_st7789v[] = {
    {0x11, {0}, 0 | 0x80},                     // Sleep out + delay
    {0x3A, {0x05}, 1},                         // Pixel format: 16bit/pixel
    {0xB2, {0x0B, 0x0B, 0x00, 0x33, 0x33}, 5}, // Porch setting
    {0xB7, {0x75}, 1},                         // Gate control
    {0xBB, {0x28}, 1},                         // VCOM setting
    {0xC0, {0x2C}, 1},                         // LCM control
    {0xC2, {0x01}, 1},                         // VDV/VRH enable
    {0xC3, {0x1F}, 1},                         // VRH set
    {0xC6, {0x13}, 1},                         // Frame rate control
    {0xD0, {0xA7}, 1},                         // Power control 1
    {0xD0, {0xA4, 0xA1}, 2},                   // Power control 2
    {0xD6, {0xA1}, 1},                         // Unknown
    {0xE0,
     {0xF0, 0x05, 0x0A, 0x06, 0x06, 0x03, 0x2B, 0x32, 0x43, 0x36, 0x11, 0x10, 0x2B, 0x32},
     14}, // Positive gamma
    {0xE1,
     {0xF0, 0x08, 0x0C, 0x0B, 0x09, 0x24, 0x2B, 0x22, 0x43, 0x38, 0x15, 0x16, 0x2F, 0x37},
     14}, // Negative gamma
};

// -------------------------------------------------------
// Backlight PWM setup (LEDC)
// -------------------------------------------------------
static void backlight_pwm_init(void)
{
  ledc_timer_config_t timer_conf = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .duty_resolution = LCD_BL_PWM_RESOLUTION,
      .timer_num = LCD_BL_PWM_TIMER,
      .freq_hz = LCD_BL_PWM_FREQ_HZ,
      .clk_cfg = LEDC_AUTO_CLK,
  };
  ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

  ledc_channel_config_t channel_conf = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = LCD_BL_PWM_CHANNEL,
      .timer_sel = LCD_BL_PWM_TIMER,
      .intr_type = LEDC_INTR_DISABLE,
      .gpio_num = LCD_BL,
      .duty = 0, // Start off, will be set after init
      .hpoint = 0,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));
}

void lcd_set_backlight(uint8_t brightness)
{
  ledc_set_duty(LEDC_LOW_SPEED_MODE, LCD_BL_PWM_CHANNEL, brightness);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, LCD_BL_PWM_CHANNEL);
  ESP_LOGI(TAG, "Backlight set to %d/255", brightness);
}

// -------------------------------------------------------
// LCD Driver Init
// -------------------------------------------------------
esp_lcd_panel_handle_t lcd_driver_init(void)
{
  ESP_LOGI(TAG, "Initializing LCD display...");

  // 1. Power on the LCD (LDO enable)
  gpio_set_direction(LCD_POWER, GPIO_MODE_OUTPUT);
  gpio_set_level(LCD_POWER, 1);

  // 2. Setup backlight via PWM
  backlight_pwm_init();

  // 3. Critical workaround: Pull RD pin HIGH
  //    Without this, I80 bus transfer is corrupted on ESP-IDF v5.0+
  gpio_set_direction(LCD_RD, GPIO_MODE_OUTPUT);
  gpio_set_level(LCD_RD, 1);

  // 4. Initialize Intel 8080 (I80) parallel bus
  esp_lcd_i80_bus_handle_t i80_bus = NULL;
  esp_lcd_i80_bus_config_t bus_config = {
      .dc_gpio_num = LCD_DC,
      .wr_gpio_num = LCD_WR,
      .data_gpio_nums = {LCD_D0, LCD_D1, LCD_D2, LCD_D3, LCD_D4, LCD_D5, LCD_D6, LCD_D7},
      .bus_width = 8,
      .max_transfer_bytes = LCD_H_RES * 40 * sizeof(uint16_t),
  };
  ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &i80_bus));

  // 5. Initialize LCD IO panel on the I80 bus
  esp_lcd_panel_io_i80_config_t io_config = {
      .cs_gpio_num = LCD_CS,
      .pclk_hz = 20 * 1000 * 1000, // 20 MHz pixel clock
      .trans_queue_depth = 10,
      .lcd_cmd_bits = 8,
      .lcd_param_bits = 8,
      .dc_levels =
          {
              .dc_idle_level = 0,
              .dc_cmd_level = 0,
              .dc_dummy_level = 0,
              .dc_data_level = 1,
          },
      .flags =
          {
              .swap_color_bytes = 1, // Fix byte order for RGB565
          },
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_config, &s_io_handle));

  // 6. Install ST7789 panel controller driver
  esp_lcd_panel_handle_t panel_handle = NULL;
  esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num = LCD_RST,
      .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
      .bits_per_pixel = 16, // RGB565
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(s_io_handle, &panel_config, &panel_handle));

  // 7. Reset, init, and configure the panel
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

  // T-Display-S3 requires color inversion for correct display
  ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));

  // Rotate to landscape: swap X and Y axes
  ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));

  // Mirror for correct orientation (flip horizontal only)
  ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, false, true));

  // Compensate for 170px display offset on 240px panel RAM
  ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 0, 35));

  // 8. Send LCD_MODULE_CMD_1 init sequence
  for (uint8_t i = 0; i < (sizeof(lcd_st7789v) / sizeof(lcd_cmd_t)); i++)
  {
    esp_lcd_panel_io_tx_param(s_io_handle, lcd_st7789v[i].cmd, lcd_st7789v[i].data,
                              lcd_st7789v[i].len & 0x7F);
    if (lcd_st7789v[i].len & 0x80)
    {
      vTaskDelay(pdMS_TO_TICKS(120));
    }
  }

  // Turn on the display
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

  // Set default backlight brightness
  lcd_set_backlight(LCD_BL_DEFAULT_BRIGHTNESS);

  ESP_LOGI(TAG, "LCD initialized: %dx%d, ST7789 via I80, backlight PWM enabled", LCD_H_RES,
           LCD_V_RES);
  return panel_handle;
}

esp_lcd_panel_io_handle_t lcd_driver_get_io_handle(void)
{
  return s_io_handle;
}
