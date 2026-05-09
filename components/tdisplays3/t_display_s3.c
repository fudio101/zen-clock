// SPDX-License-Identifier: MIT
// Based on: https://github.com/hiruna/esp-idf-t-display-s3
// Adapted for ZenClock project — uses lcd_backlight component instead of aw9364

#include "t_display_s3.h"
#include <stdio.h>
#include <math.h>
#include <esp_log.h>
#include "esp_adc/adc_oneshot.h"
#include <soc/adc_channel.h>
#include <esp_lcd_panel_st7789.h>
#include "driver/gpio.h"
#include "lcd_backlight.h"

static const char *TAG = "tdisplays3";

// ============================================================
// Static handles
// ============================================================
static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t adc_cali_handle;
static lcd_backlight_handle_t bl_handle;

// ============================================================
// LCD I80 Bus Init
// ============================================================
static void init_lcd_i80_bus(esp_lcd_panel_io_handle_t *io_handle)
{
  ESP_LOGI(TAG, "Initializing Intel 8080 bus...");
  esp_lcd_i80_bus_handle_t i80_bus = NULL;
  esp_lcd_i80_bus_config_t bus_config = {
      .clk_src = LCD_CLK_SRC_DEFAULT,
      .dc_gpio_num = LCD_PIN_NUM_DC,
      .wr_gpio_num = LCD_PIN_NUM_PCLK,
      .data_gpio_nums = {
          LCD_PIN_NUM_DATA0,
          LCD_PIN_NUM_DATA1,
          LCD_PIN_NUM_DATA2,
          LCD_PIN_NUM_DATA3,
          LCD_PIN_NUM_DATA4,
          LCD_PIN_NUM_DATA5,
          LCD_PIN_NUM_DATA6,
          LCD_PIN_NUM_DATA7},
      .bus_width = LCD_I80_BUS_WIDTH,
      .max_transfer_bytes = LCD_H_RES * 100 * sizeof(uint16_t),
  };
  ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &i80_bus));

  esp_lcd_panel_io_i80_config_t io_config = {
      .cs_gpio_num = LCD_PIN_NUM_CS,
      .pclk_hz = LCD_PIXEL_CLOCK_HZ,
      .trans_queue_depth = LCD_I80_TRANS_QUEUE_SIZE,
      .dc_levels = {
          .dc_idle_level = LCD_I80_DC_CMD_LEVEL,
          .dc_cmd_level = LCD_I80_DC_CMD_LEVEL,
          .dc_dummy_level = LCD_I80_DC_DUMMY_LEVEL,
          .dc_data_level = LCD_I80_DC_DATA_LEVEL,
      },
      .lcd_cmd_bits = LCD_CMD_BITS,
      .lcd_param_bits = LCD_PARAM_BITS,
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_config, io_handle));
}

// ============================================================
// LCD Panel Init (ST7789)
// ============================================================
static void init_lcd_panel(esp_lcd_panel_io_handle_t io_handle, esp_lcd_panel_handle_t *panel)
{
  esp_lcd_panel_handle_t panel_handle = NULL;

  ESP_LOGI(TAG, "Initializing ST7789 LCD Driver...");
  esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num = LCD_PIN_NUM_RST,
      .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
      .bits_per_pixel = 16,
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

  esp_lcd_panel_reset(panel_handle);
  esp_lcd_panel_init(panel_handle);

  esp_lcd_panel_invert_color(panel_handle, true);

  // Landscape orientation: buttons on left, screen on right
  esp_lcd_panel_swap_xy(panel_handle, true);
  esp_lcd_panel_mirror(panel_handle, false, true);
  esp_lcd_panel_set_gap(panel_handle, 0, 35);

  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

  *panel = panel_handle;
}

// ============================================================
// Battery ADC
// ============================================================
static void init_battery_adc(void)
{
  if (adc_handle == NULL)
  {
    const adc_oneshot_unit_init_cfg_t adc_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_cfg, &adc_handle));
  }
}

static void init_battery_monitor(void)
{
  ESP_LOGI(TAG, "Configuring battery monitor...");
  init_battery_adc();

  const adc_oneshot_chan_cfg_t adc_chan_cfg = {
      .bitwidth = ADC_BITWIDTH_DEFAULT,
      .atten = ADC_ATTEN_DB_12,
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_3, &adc_chan_cfg));

  const adc_cali_curve_fitting_config_t cali_config = {
      .unit_id = ADC_UNIT_1,
      .atten = ADC_ATTEN_DB_12,
      .bitwidth = ADC_BITWIDTH_DEFAULT,
  };
  ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali_handle));
}

int get_battery_voltage(void)
{
  int voltage, adc_raw;
  assert(adc_handle);
  ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL_3, &adc_raw));
  ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc_cali_handle, adc_raw, &voltage));
  return voltage * 2;
}

double volts_to_percentage(double volts)
{
  return 123 - ((double)123 / pow((1 + pow(((double)volts / 3.7), 80)), 0.165));
}

int get_battery_percentage(void)
{
  return (int)ceil(volts_to_percentage((double)get_battery_voltage() / 1000));
}

static bool usb_power_voltage(int milliVolts)
{
  return ceilf((float)(milliVolts - 100) / 1000) == 5.0;
}

bool usb_power_connected(void)
{
  return usb_power_voltage(get_battery_voltage());
}

// ============================================================
// LCD Power & Backlight
// ============================================================
static void lcd_power_init(void)
{
  ESP_LOGI(TAG, "Configuring LCD PWR GPIO...");
  gpio_set_direction(LCD_PIN_NUM_PWR, GPIO_MODE_OUTPUT);
  gpio_set_level(LCD_PIN_NUM_PWR, LCD_PWR_ON_LEVEL);

  ESP_LOGI(TAG, "Configuring LCD RD GPIO...");
  gpio_set_direction(LCD_PIN_NUM_RD, GPIO_MODE_INPUT);
  gpio_set_pull_mode(LCD_PIN_NUM_RD, GPIO_PULLUP_ONLY);
}

static void lcd_backlight_setup(void)
{
  ESP_LOGI(TAG, "Configuring LCD Backlight via lcd_backlight component...");
  const lcd_backlight_config_t bl_config = {
      .gpio_num = LCD_PIN_NUM_BK_LIGHT,
      .pwm_freq_hz = 5000,
      .timer_num = LEDC_TIMER_1,
      .channel_num = LCD_BK_LIGHT_LEDC_CH,
  };
  ESP_ERROR_CHECK(lcd_backlight_init(&bl_config, &bl_handle));
}

void lcd_set_brightness(uint8_t percent, uint32_t fade_time_ms)
{
  if (bl_handle)
  {
    ESP_ERROR_CHECK(lcd_backlight_set_brightness(bl_handle, percent, fade_time_ms));
  }
}

uint8_t lcd_get_brightness(void)
{
  if (bl_handle)
  {
    return lcd_backlight_get_brightness(bl_handle);
  }
  return 0;
}

// ============================================================
// LVGL Display Registration
// ============================================================
static lv_disp_t *lcd_lvgl_add_disp(esp_lcd_panel_io_handle_t io_handle, esp_lcd_panel_handle_t panel_handle)
{
  ESP_LOGI(TAG, "Adding display driver to LVGL port...");
  const lvgl_port_display_cfg_t disp_cfg = {
      .io_handle = io_handle,
      .panel_handle = panel_handle,
      .buffer_size = LVGL_BUFFER_SIZE,
      .double_buffer = true,
      .hres = LCD_H_RES,
      .vres = LCD_V_RES,
      .monochrome = false,
      .color_format = LV_COLOR_FORMAT_RGB565,
      .rotation = {
          .swap_xy = true,
          .mirror_x = false,
          .mirror_y = true,
      },
      .flags = {
          .buff_spiram = true,
          .swap_bytes = true,
      }};
  return lvgl_port_add_disp(&disp_cfg);
}

// ============================================================
// Main LCD Init — call from app_main
// ============================================================
void lcd_init(lv_disp_t **disp_handle, bool backlight_on)
{
  // LVGL port initialization
  const lvgl_port_cfg_t lvgl_cfg = {
      .task_priority = LVGL_TASK_PRIORITY,
      .task_stack = LVGL_TASK_STACK_SIZE,
      .task_affinity = 1,
      .task_max_sleep_ms = LVGL_MAX_SLEEP_MS,
      .timer_period_ms = LVGL_TICK_PERIOD_MS};
  esp_err_t err = lvgl_port_init(&lvgl_cfg);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Error initializing LVGL port!");
  }

  lcd_power_init();
  lcd_backlight_setup();
  init_battery_monitor();

  // LCD IO
  esp_lcd_panel_io_handle_t io_handle = NULL;
  init_lcd_i80_bus(&io_handle);

  // LCD driver
  esp_lcd_panel_handle_t panel_handle = NULL;
  init_lcd_panel(io_handle, &panel_handle);

  // Register with LVGL
  lv_disp_t *disp_hdl = lcd_lvgl_add_disp(io_handle, panel_handle);
  *disp_handle = disp_hdl;

  if (backlight_on)
  {
    lcd_set_brightness(100, 0);
  }

  ESP_LOGI(TAG, "LCD initialized: %dx%d, ST7789 via I80, LVGL port ready", LCD_H_RES, LCD_V_RES);
}
