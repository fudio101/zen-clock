// SPDX-License-Identifier: MIT
// ZenClock UI — screen + theme + module composition

#include "ui.h"
#include "nav.h"
#include "lvgl.h"

void ui_init(bool is_light)
{
  // --- Theme init ---
  lv_disp_t *dispp = lv_display_get_default();
  lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                                            !is_light, LV_FONT_DEFAULT);
  lv_disp_set_theme(dispp, theme);

  // --- Navigation (creates and loads initial clock screen) ---
  nav_init();
}

void ui_set_theme(bool is_light)
{
  lv_disp_t *dispp = lv_display_get_default();
  lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                                            !is_light, LV_FONT_DEFAULT);
  lv_disp_set_theme(dispp, theme);
}
