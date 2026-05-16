// SPDX-License-Identifier: MIT
// ZenClock UI — screen + theme + module composition

#include "ui.h"
#include "nav.h"
#include "lvgl.h"

static bool s_is_light = true;
static lv_style_t s_screen_bg_style;
static bool s_style_inited = false;

void ui_init(const bool is_light)
{
  s_is_light = is_light;

  lv_style_init(&s_screen_bg_style);
  lv_style_set_bg_color(&s_screen_bg_style, is_light ? lv_color_hex(0xe6e6e6) : lv_color_hex(0x0d0d0d));
  lv_style_set_bg_opa(&s_screen_bg_style, LV_OPA_COVER);
  s_style_inited = true;

  lv_disp_t *dispp = lv_display_get_default();
  lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                                            !is_light, LV_FONT_DEFAULT);
  lv_disp_set_theme(dispp, theme);

  nav_init();
}

void ui_set_theme(const bool is_light)
{
  s_is_light = is_light;

  lv_style_set_bg_color(&s_screen_bg_style, is_light ? lv_color_hex(0xe6e6e6) : lv_color_hex(0x0d0d0d));
  lv_obj_report_style_change(&s_screen_bg_style);

  lv_disp_t *dispp = lv_display_get_default();
  lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                                            !is_light, LV_FONT_DEFAULT);
  lv_disp_set_theme(dispp, theme);
}

bool ui_is_light_theme(void)
{
  return s_is_light;
}

void ui_apply_screen_bg(lv_obj_t *scr)
{
  if (s_style_inited)
  {
    lv_obj_add_style(scr, &s_screen_bg_style, 0);
  }
}
