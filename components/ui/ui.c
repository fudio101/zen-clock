// SPDX-License-Identifier: MIT
// ZenClock UI — Hand-written LVGL interface

#include "ui.h"

// ============================================================
// Widget handles
// ============================================================
lv_obj_t *ui_screen_main = NULL;
lv_obj_t *ui_bat_icon_label = NULL;
lv_obj_t *ui_bat_pct_label = NULL;
lv_obj_t *ui_clock_label = NULL;

// ============================================================
// Public API
// ============================================================
void ui_init(void)
{
  // --- Theme init (exact same pattern as old SquareLine code) ---
  lv_disp_t *dispp = lv_display_get_default();
  lv_theme_t *theme = lv_theme_default_init(
      dispp,
      lv_palette_main(LV_PALETTE_BLUE),
      lv_palette_main(LV_PALETTE_RED),
      false, LV_FONT_DEFAULT);
  lv_disp_set_theme(dispp, theme);

  // --- Main screen (exact same as old SquareLine Screen1) ---
  ui_screen_main = lv_obj_create(NULL);
  lv_obj_remove_flag(ui_screen_main, LV_OBJ_FLAG_SCROLLABLE);

  // --- Clock label (center, like old Label1) ---
  ui_clock_label = lv_label_create(ui_screen_main);
  lv_obj_set_width(ui_clock_label, LV_SIZE_CONTENT);
  lv_obj_set_height(ui_clock_label, LV_SIZE_CONTENT);
  lv_obj_set_align(ui_clock_label, LV_ALIGN_CENTER);
  lv_label_set_text(ui_clock_label, "ZenClock Ready");

  // --- Battery percentage (top-right corner) ---
  ui_bat_pct_label = lv_label_create(ui_screen_main);
  lv_obj_set_width(ui_bat_pct_label, LV_SIZE_CONTENT);
  lv_obj_set_height(ui_bat_pct_label, LV_SIZE_CONTENT);
  lv_obj_align(ui_bat_pct_label, LV_ALIGN_TOP_RIGHT, -8, 4);
  lv_label_set_text(ui_bat_pct_label, "--%");

  // --- Battery icon (left of percentage, with gap) ---
  ui_bat_icon_label = lv_label_create(ui_screen_main);
  lv_obj_set_width(ui_bat_icon_label, LV_SIZE_CONTENT);
  lv_obj_set_height(ui_bat_icon_label, LV_SIZE_CONTENT);
  lv_obj_align_to(ui_bat_icon_label, ui_bat_pct_label, LV_ALIGN_OUT_LEFT_MID, -4, 0);
  lv_label_set_text(ui_bat_icon_label, LV_SYMBOL_BATTERY_FULL);

  // --- Load screen ---
  lv_disp_load_scr(ui_screen_main);
}
