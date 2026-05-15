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
static lv_timer_t *s_clock_timer = NULL;
static lv_timer_t *s_orbit_timer = NULL;
static uint8_t s_orbit_step = 0;

// Pixel orbital shift: 2px range, 4 positions, 7-min cycle
// Prevents LCD image retention on static background regions at screen edges
static constexpr int8_t orbit_x[4] = {0, 2, 2, 0};
static constexpr int8_t orbit_y[4] = {0, 0, 2, 2};

// ============================================================
// Orbital shift callback — fires every 7 min, cycles 4 offset positions
// ============================================================
static void orbit_timer_cb(lv_timer_t *timer) // NOLINT(readability-non-const-parameter)
{
  (void) timer;
  s_orbit_step = (s_orbit_step + 1) % 4;
  lv_obj_align(s_time_label, LV_ALIGN_CENTER, orbit_x[s_orbit_step], -12 + orbit_y[s_orbit_step]);
  lv_obj_align_to(s_date_label, s_time_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);
}

// ============================================================
// Timer callback — runs inside lv_timer_handler() on LVGL task
// ============================================================
static void clock_timer_cb(lv_timer_t *timer) // NOLINT(readability-non-const-parameter)
{
  (void) timer;
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
  s_clock_timer = lv_timer_create(clock_timer_cb, 1000, NULL);
  lv_timer_ready(s_clock_timer); // Fire immediately on first tick

  // --- Orbital shift: 7-min period, prevents LCD image retention at screen edges ---
  s_orbit_step = 0;
  s_orbit_timer = lv_timer_create(orbit_timer_cb, 7 * 60 * 1000, NULL);
}

void clock_face_destroy(void)
{
  if (s_clock_timer)
  {
    lv_timer_delete(s_clock_timer);
    s_clock_timer = NULL;
  }
  if (s_orbit_timer)
  {
    lv_timer_delete(s_orbit_timer);
    s_orbit_timer = NULL;
  }
  s_time_label = NULL;
  s_date_label = NULL;
}
