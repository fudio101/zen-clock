// SPDX-License-Identifier: MIT
// BSP Display — I80 bus, ST7789 panel, LVGL port, power GPIO

#include "bsp.h"
#include "bsp_priv.h"
#include "board_config.h"

#include <esp_log.h>
#include <esp_lcd_panel_st7789.h>
#include "driver/gpio.h"

static const char *TAG = "bsp_display";

// ============================================================
// I80 Bus & Panel Constants (private to this file)
// ============================================================
#define PIXEL_CLOCK_HZ (17 * 1000 * 1000)
#define I80_BUS_WIDTH 8
#define I80_TRANS_QUEUE_DEPTH 20
#define I80_DC_CMD_LEVEL 0
#define I80_DC_DUMMY_LEVEL 0
#define I80_DC_DATA_LEVEL 1
#define CMD_BITS 8
#define PARAM_BITS 8
#define PWR_ON_LEVEL 1

// ============================================================
// LVGL Port Constants
// ============================================================
#define LVGL_BUF_SIZE (LCD_H_RES * LCD_V_RES)
#define LVGL_TICK_PERIOD_MS 5
#define LVGL_MAX_SLEEP_MS 10
#define LVGL_TASK_STACK_SIZE (6 * 1024)
#define LVGL_TASK_PRIORITY 2

// ============================================================
// I80 Bus Init
// ============================================================
static void init_i80_bus(esp_lcd_panel_io_handle_t *io_handle)
{
  ESP_LOGI(TAG, "Initializing Intel 8080 bus...");

  esp_lcd_i80_bus_handle_t i80_bus = NULL;
  esp_lcd_i80_bus_config_t bus_config = {
      .clk_src = LCD_CLK_SRC_DEFAULT,
      .dc_gpio_num = PIN_LCD_DC,
      .wr_gpio_num = PIN_LCD_WR,
      .data_gpio_nums = {PIN_LCD_D0, PIN_LCD_D1, PIN_LCD_D2, PIN_LCD_D3, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6,
                         PIN_LCD_D7},
      .bus_width = I80_BUS_WIDTH,
      .max_transfer_bytes = LCD_H_RES * LCD_V_RES * sizeof(uint16_t),
  };
  ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &i80_bus));

  esp_lcd_panel_io_i80_config_t io_config = {
      .cs_gpio_num = PIN_LCD_CS,
      .pclk_hz = PIXEL_CLOCK_HZ,
      .trans_queue_depth = I80_TRANS_QUEUE_DEPTH,
      .dc_levels =
          {
              .dc_idle_level = I80_DC_CMD_LEVEL,
              .dc_cmd_level = I80_DC_CMD_LEVEL,
              .dc_dummy_level = I80_DC_DUMMY_LEVEL,
              .dc_data_level = I80_DC_DATA_LEVEL,
          },
      .lcd_cmd_bits = CMD_BITS,
      .lcd_param_bits = PARAM_BITS,
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_config, io_handle));
}

// ============================================================
// ST7789 Panel Init
// ============================================================
static void init_panel(esp_lcd_panel_io_handle_t io_handle, esp_lcd_panel_handle_t *panel)
{
  ESP_LOGI(TAG, "Initializing ST7789 LCD driver...");

  esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num = PIN_LCD_RST,
      .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
      .bits_per_pixel = 16,
  };
  esp_lcd_panel_handle_t panel_handle = NULL;
  ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

  esp_lcd_panel_reset(panel_handle);
  esp_lcd_panel_init(panel_handle);
  esp_lcd_panel_invert_color(panel_handle, true);

  // Landscape orientation: buttons on left, USB on right
  esp_lcd_panel_swap_xy(panel_handle, true);
  esp_lcd_panel_mirror(panel_handle, false, true);
  esp_lcd_panel_set_gap(panel_handle, 0, 35);

  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
  *panel = panel_handle;
}

// ============================================================
// Power GPIO
// ============================================================
static void init_power(void)
{
  ESP_LOGI(TAG, "Configuring LCD power GPIO...");
  gpio_set_direction(PIN_LCD_PWR, GPIO_MODE_OUTPUT);
  gpio_set_level(PIN_LCD_PWR, PWR_ON_LEVEL);

  ESP_LOGI(TAG, "Configuring LCD RD GPIO...");
  gpio_set_direction(PIN_LCD_RD, GPIO_MODE_INPUT);
  gpio_set_pull_mode(PIN_LCD_RD, GPIO_PULLUP_ONLY);
}

// ============================================================
// LVGL Display Registration
// ============================================================
static lv_disp_t *register_lvgl_display(esp_lcd_panel_io_handle_t io_handle, esp_lcd_panel_handle_t panel)
{
  ESP_LOGI(TAG, "Registering display with LVGL port...");

  const lvgl_port_display_cfg_t disp_cfg = {.io_handle = io_handle,
                                            .panel_handle = panel,
                                            .buffer_size = LVGL_BUF_SIZE,
                                            .double_buffer = true,
                                            .hres = LCD_H_RES,
                                            .vres = LCD_V_RES,
                                            .monochrome = false,
                                            .color_format = LV_COLOR_FORMAT_RGB565,
                                            .rotation =
                                                {
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
// Public API
// ============================================================
void bsp_display_init(lv_disp_t **disp_handle, bool backlight_on)
{
  // LVGL port initialization
  const lvgl_port_cfg_t lvgl_cfg = {.task_priority = LVGL_TASK_PRIORITY,
                                    .task_stack = LVGL_TASK_STACK_SIZE,
                                    .task_affinity = 1,
                                    .task_max_sleep_ms = LVGL_MAX_SLEEP_MS,
                                    .timer_period_ms = LVGL_TICK_PERIOD_MS};
  ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

  init_power();
  bsp_backlight_setup();
  bsp_battery_setup();

  // LCD I/O
  esp_lcd_panel_io_handle_t io_handle = NULL;
  init_i80_bus(&io_handle);

  // LCD panel
  esp_lcd_panel_handle_t panel_handle = NULL;
  init_panel(io_handle, &panel_handle);

  // Register with LVGL
  *disp_handle = register_lvgl_display(io_handle, panel_handle);

  if (backlight_on)
  {
    bsp_backlight_set(100, 0);
  }

  ESP_LOGI(TAG, "BSP initialized: %dx%d ST7789 via I80, LVGL ready", LCD_H_RES, LCD_V_RES);
}

void bsp_display_set_brightness(uint8_t percent, uint32_t fade_time_ms)
{
  bsp_backlight_set(percent, fade_time_ms);
}

uint8_t bsp_display_get_brightness(void)
{
  return bsp_backlight_get();
}
