#include "ui/ui_manager.h"
#include "display/lcd_driver.h"
#include "display/lcd_graphics.h"
#include "display/lcd_text.h"
#include "display/lcd_colors.h"
#include "board_config.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "ui_manager";

// ============================================================
// Static state — panel handle and layout positions
// ============================================================
static esp_lcd_panel_handle_t s_panel = NULL;
static int s_counter_y = 0; // Y position of the counter line
static int s_status_y = 0;  // Y position of the status line

// Layout constants
#define UI_MARGIN_X 0
#define UI_MARGIN_TOP 8
#define UI_LINE_SPACING 0 // Extra spacing between sections
#define UI_MAX_LINE_CHARS (LCD_H_RES / FONT_DEFAULT.render_width)

void ui_init(esp_lcd_panel_handle_t panel)
{
  s_panel = panel;

  // Clear screen
  lcd_fill_color(panel, COLOR_BLACK);

  int y = UI_MARGIN_TOP;

  // -------------------------------------------------------
  // Header banner
  // -------------------------------------------------------
  lcd_draw_string(panel, UI_MARGIN_X, y, "========================================",
                  &FONT_DEFAULT, COLOR_CYAN, COLOR_BLACK);
  y += FONT_DEFAULT.render_height;

  lcd_draw_string(panel, UI_MARGIN_X, y, "LilyGo S3 da khoi dong.",
                  &FONT_DEFAULT, COLOR_GREEN, COLOR_BLACK);
  y += FONT_DEFAULT.render_height;

  lcd_draw_string(panel, UI_MARGIN_X, y, "========================================",
                  &FONT_DEFAULT, COLOR_CYAN, COLOR_BLACK);
  y += FONT_DEFAULT.render_height + 8; // Extra spacing

  // -------------------------------------------------------
  // RAM info
  // -------------------------------------------------------
  size_t internal_ram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  size_t external_ram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

  char buf[50];

  snprintf(buf, sizeof(buf), "SRAM : %d bytes", internal_ram);
  lcd_draw_string(panel, UI_MARGIN_X, y, buf, &FONT_DEFAULT, COLOR_WHITE, COLOR_BLACK);
  y += FONT_DEFAULT.render_height;

  snprintf(buf, sizeof(buf), "PSRAM: %d bytes (~%d MB)", external_ram,
           external_ram / (1024 * 1024));
  lcd_draw_string(panel, UI_MARGIN_X, y, buf, &FONT_DEFAULT, COLOR_WHITE, COLOR_BLACK);
  y += FONT_DEFAULT.render_height + 16; // Extra spacing before counter

  // Console output (keep for debug)
  printf("========================================\n");
  printf("LilyGo S3 da khoi dong.\n");
  printf("SRAM noi bo: %d bytes\n", internal_ram);
  printf("PSRAM ngoai: %d bytes (~%d MB)\n", external_ram, external_ram / (1024 * 1024));
  printf("========================================\n");

  // Save layout positions for dynamic updates
  s_counter_y = y;
  s_status_y = y + FONT_DEFAULT.render_height + 4;

  ESP_LOGI(TAG, "UI initialized, counter_y=%d", s_counter_y);
}

void ui_update_counter(int counter)
{
  if (!s_panel)
    return;

  char buf[50];
  snprintf(buf, sizeof(buf), "Dang chay... [%d]", counter);

  lcd_draw_string_fast(s_panel, UI_MARGIN_X, s_counter_y, buf,
                       UI_MAX_LINE_CHARS, &FONT_DEFAULT, COLOR_YELLOW, COLOR_BLACK);

  printf("He thong dang chay... [%d]\n", counter);
}

void ui_update_status(const char *msg)
{
  if (!s_panel || !msg)
    return;

  lcd_draw_string_fast(s_panel, UI_MARGIN_X, s_status_y, msg,
                       UI_MAX_LINE_CHARS, &FONT_DEFAULT, COLOR_WHITE, COLOR_BLACK);
}
