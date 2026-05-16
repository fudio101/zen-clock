// SPDX-License-Identifier: MIT
// ZenClock — Text-based clock face (DS-Digital 48, fixed-width digit containers)
//
// Colors follow the global theme (ui_is_light_theme):
//   light: digit #333333, date #666666, background handled by screen (nav.c)
//   dark:  digit #01ddff, date #007a99, background handled by screen (nav.c)
//
// Fixed-width layout: HH(50) + :(12) + MM(50) + :(12) + SS(50) = 174px total
// DS-Digital adv_w: digits 391/16 = 24.4px → pair 48.9px → DIGIT_W=50 (1.1px margin)
//                   colon  170/16 = 10.6px             → COLON_W=12 (1.4px margin)

#include "clock_face.h"
#include "ui.h"
#include "fonts/lv_font_ds_digital_48.h"

#include <time.h>
#include <sys/time.h>

#define DIGIT_W 50
#define COLON_W 12

static lv_obj_t *s_hh_label = NULL;
static lv_obj_t *s_mm_label = NULL;
static lv_obj_t *s_ss_label = NULL;
static lv_obj_t *s_date_label = NULL;
static lv_timer_t *s_clock_timer = NULL;

// ============================================================
// Timer callback — runs inside lv_timer_handler() on LVGL task
// ============================================================
static void clock_timer_cb(lv_timer_t *timer) // NOLINT(readability-non-const-parameter)
{
  (void) timer;

  time_t now = time(NULL);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);

  char buf[12];

  (void) strftime(buf, sizeof(buf), "%H", &timeinfo);
  lv_label_set_text(s_hh_label, buf);

  (void) strftime(buf, sizeof(buf), "%M", &timeinfo);
  lv_label_set_text(s_mm_label, buf);

  (void) strftime(buf, sizeof(buf), "%S", &timeinfo);
  lv_label_set_text(s_ss_label, buf);

  (void) strftime(buf, sizeof(buf), "%d/%m/%Y", &timeinfo);
  lv_label_set_text(s_date_label, buf);
}

// ============================================================
// Helper — create one fixed-width digit or colon label
// ============================================================
static lv_obj_t *make_segment(lv_obj_t *parent, int32_t w, const char *init, lv_color_t color)
{
  lv_obj_t *label = lv_label_create(parent);
  lv_obj_set_width(label, w);
  lv_obj_set_height(label, LV_SIZE_CONTENT);
  lv_obj_set_style_text_font(label, &lv_font_ds_digital_48, 0);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(label, color, 0);
  lv_label_set_text(label, init);
  return label;
}

// ============================================================
// Public API
// ============================================================
void clock_face_create(lv_obj_t *parent)
{
  bool light = ui_is_light_theme();
  lv_color_t digit_color = light ? lv_color_hex(0x333333) : lv_color_hex(0x01ddff);
  lv_color_t date_color = light ? lv_color_hex(0x666666) : lv_color_hex(0x007a99);

  // Row container: fixed total width = DIGIT_W*3 + COLON_W*2
  lv_obj_t *time_row = lv_obj_create(parent);
  lv_obj_remove_style_all(time_row);
  lv_obj_set_size(time_row, DIGIT_W * 3 + COLON_W * 2, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(time_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(time_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(time_row, 0, 0);
  lv_obj_set_style_pad_column(time_row, 0, 0);
  lv_obj_align(time_row, LV_ALIGN_CENTER, 0, -12);

  s_hh_label = make_segment(time_row, DIGIT_W, "00", digit_color);
  make_segment(time_row, COLON_W, ":", digit_color);
  s_mm_label = make_segment(time_row, DIGIT_W, "00", digit_color);
  make_segment(time_row, COLON_W, ":", digit_color);
  s_ss_label = make_segment(time_row, DIGIT_W, "00", digit_color);

  // Date label below the time row
  s_date_label = lv_label_create(parent);
  lv_obj_set_width(s_date_label, LV_PCT(100));
  lv_obj_set_height(s_date_label, LV_SIZE_CONTENT);
  lv_obj_set_style_text_align(s_date_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(s_date_label, date_color, 0);
  lv_obj_align_to(s_date_label, time_row, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);
  lv_label_set_text(s_date_label, "--/--/----");

  s_clock_timer = lv_timer_create(clock_timer_cb, 1000, NULL);
  lv_timer_ready(s_clock_timer);
}

void clock_face_destroy(void)
{
  if (s_clock_timer)
  {
    lv_timer_delete(s_clock_timer);
    s_clock_timer = NULL;
  }
  s_hh_label = NULL;
  s_mm_label = NULL;
  s_ss_label = NULL;
  s_date_label = NULL;
}
