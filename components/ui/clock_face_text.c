// SPDX-License-Identifier: MIT
// ZenClock — Text-based clock face (Montserrat 48 time, Montserrat 14 date)

#include "clock_face.h"

#include <time.h>
#include <sys/time.h>

// ============================================================
// Private state
// ============================================================
static lv_obj_t *s_time_label = NULL;
static lv_obj_t *s_date_label = NULL;

// ============================================================
// Timer callback — runs inside lv_timer_handler() on LVGL task
// ============================================================
static void clock_timer_cb(lv_timer_t *timer)
{
  (void)timer;
  char time_buf[16];
  char date_buf[16];

  time_t now = time(NULL);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);

  strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &timeinfo);
  strftime(date_buf, sizeof(date_buf), "%d/%m/%Y", &timeinfo);

  lv_label_set_text(s_time_label, time_buf);
  lv_label_set_text(s_date_label, date_buf);
}

// ============================================================
// Public API
// ============================================================
void clock_face_create(lv_obj_t *parent)
{
  // --- Time label (center, large font, fixed width to prevent artifacts) ---
  s_time_label = lv_label_create(parent);
  lv_obj_set_width(s_time_label, 260);
  lv_obj_set_height(s_time_label, LV_SIZE_CONTENT);
  lv_obj_set_style_text_font(s_time_label, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_align(s_time_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(s_time_label, LV_ALIGN_CENTER, 0, -12);
  lv_label_set_text(s_time_label, "00:00:00");

  // --- Date label (below time, smaller muted text, fixed width) ---
  s_date_label = lv_label_create(parent);
  lv_obj_set_width(s_date_label, 260);
  lv_obj_set_height(s_date_label, LV_SIZE_CONTENT);
  lv_obj_set_style_text_align(s_date_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(s_date_label, lv_palette_main(LV_PALETTE_GREY), 0);
  lv_obj_align_to(s_date_label, s_time_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);
  lv_label_set_text(s_date_label, "--/--/----");

  // --- LVGL timer: update every 1 second ---
  lv_timer_t *timer = lv_timer_create(clock_timer_cb, 1000, NULL);
  lv_timer_ready(timer); // Fire immediately on first tick
}
