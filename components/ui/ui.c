// SPDX-License-Identifier: MIT
// ZenClock UI — screen + theme + module composition

#include "ui.h"
#include "clock_face.h"
#include "status_bar.h"

void ui_init(void)
{
  // --- Theme init ---
  lv_disp_t *dispp = lv_display_get_default();
  lv_theme_t *theme = lv_theme_default_init(
      dispp,
      lv_palette_main(LV_PALETTE_BLUE),
      lv_palette_main(LV_PALETTE_RED),
      false, LV_FONT_DEFAULT);
  lv_disp_set_theme(dispp, theme);

  // --- Main screen ---
  lv_obj_t *screen = lv_obj_create(NULL);
  lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

  // --- Widget modules ---
  clock_face_create(screen);
  status_bar_create(screen);

  // --- Load screen ---
  lv_disp_load_scr(screen);
}
