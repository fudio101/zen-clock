// SPDX-License-Identifier: MIT
// ZenClock — Settings screen (vertical list with inline edit mode)
//
// Layout (320×170):
//   Status bar (top, managed externally)
//   Title "Settings" (y=24)
//   Item list: label left, value right, starting at y=48
//
// Item types:
//   TOGGLE — cycles between string options (Theme: Dark/Light)
//   RANGE  — increments/decrements numeric value (Brightness: 0-100)
//   ACTION — executes on select, no edit mode (Reset WiFi)

#include "settings_screen.h"
#include "settings.h"
#include "bsp.h"
#include "ui.h"

#include <esp_log.h>
#include <stdio.h>

static const char *TAG = "settings_scr";

// ============================================================
// Item definitions
// ============================================================
typedef enum
{
  STYPE_TOGGLE,
  STYPE_RANGE,
  STYPE_ACTION,
} setting_type_t;

typedef struct
{
  const char *label;
  setting_type_t type;
  // Toggle fields
  const char **options;
  int option_count;
  // Range fields
  int min;
  int max;
  int step;
  // Current value (working copy)
  int value;
} setting_item_t;

static const char *s_theme_options[] = {"Dark", "Light"};

#define SETTINGS_ITEM_COUNT 3

static setting_item_t s_items[SETTINGS_ITEM_COUNT] = {
    {
        .label = "Theme",
        .type = STYPE_TOGGLE,
        .options = s_theme_options,
        .option_count = 2,
    },
    {
        .label = "Brightness",
        .type = STYPE_RANGE,
        .min = 0,
        .max = 100,
        .step = 10,
    },
    {
        .label = "Reset WiFi",
        .type = STYPE_ACTION,
    },
};

// ============================================================
// Layout constants
// ============================================================
#define TITLE_Y 24
#define LIST_Y_START 50
#define LIST_ITEM_H 24
#define LIST_X_PAD 16
#define VALUE_X_RIGHT (-12) // right-aligned offset from screen edge

// ============================================================
// Private state
// ============================================================
static int s_focus = 0;
static bool s_editing = false;

static lv_obj_t *s_name_labels[SETTINGS_ITEM_COUNT] = {NULL};
static lv_obj_t *s_value_labels[SETTINGS_ITEM_COUNT] = {NULL};
static lv_obj_t *s_focus_marker = NULL;
static lv_obj_t *s_edit_box = NULL; // border around edited value

// ============================================================
// Value display helpers
// ============================================================
static void update_value_text(int index)
{
  if (!s_value_labels[index])
  {
    return;
  }

  setting_item_t *item = &s_items[index];
  char buf[16];

  switch (item->type)
  {
  case STYPE_TOGGLE:
    if (item->value >= 0 && item->value < item->option_count)
    {
      lv_label_set_text(s_value_labels[index], item->options[item->value]);
    }
    break;
  case STYPE_RANGE:
    snprintf(buf, sizeof(buf), "%d%%", item->value);
    lv_label_set_text(s_value_labels[index], buf);
    break;
  case STYPE_ACTION:
    // No value display for actions
    break;
  }
}

// ============================================================
// Focus visual
// ============================================================
static void update_focus_visual(void)
{
  if (s_focus_marker)
  {
    if (s_editing)
    {
      // Hide marker during edit
      lv_obj_add_flag(s_focus_marker, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
      lv_obj_remove_flag(s_focus_marker, LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_y(s_focus_marker, LIST_Y_START + s_focus * LIST_ITEM_H);
    }
  }

  for (int i = 0; i < SETTINGS_ITEM_COUNT; i++)
  {
    if (!s_name_labels[i])
    {
      continue;
    }

    if (i == s_focus)
    {
      lv_obj_set_style_text_opa(s_name_labels[i], LV_OPA_COVER, 0);
      lv_obj_set_style_text_color(s_name_labels[i], lv_palette_main(LV_PALETTE_LIGHT_BLUE), 0);
    }
    else
    {
      lv_obj_set_style_text_opa(s_name_labels[i], LV_OPA_70, 0);
      lv_obj_remove_local_style_prop(s_name_labels[i], LV_STYLE_TEXT_COLOR, 0);
    }

    // Value label follows name styling
    if (s_value_labels[i])
    {
      if (i == s_focus)
      {
        lv_obj_set_style_text_opa(s_value_labels[i], LV_OPA_COVER, 0);
      }
      else
      {
        lv_obj_set_style_text_opa(s_value_labels[i], LV_OPA_70, 0);
      }
    }
  }
}

// ============================================================
// Edit mode visual (box around value)
// ============================================================
static void show_edit_box(int index)
{
  if (!s_value_labels[index])
  {
    return;
  }

  // Create a small border container around the value label
  if (s_edit_box)
  {
    lv_obj_delete(s_edit_box);
    s_edit_box = NULL;
  }

  lv_obj_t *parent = lv_obj_get_parent(s_value_labels[index]);
  s_edit_box = lv_obj_create(parent);
  lv_obj_remove_flag(s_edit_box, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(s_edit_box, 80, LIST_ITEM_H);
  lv_obj_set_style_bg_opa(s_edit_box, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(s_edit_box, 1, 0);
  lv_obj_set_style_border_color(s_edit_box, lv_palette_main(LV_PALETTE_LIGHT_BLUE), 0);
  lv_obj_set_style_radius(s_edit_box, 4, 0);
  lv_obj_set_style_pad_all(s_edit_box, 0, 0);
  lv_obj_align(s_edit_box, LV_ALIGN_TOP_RIGHT, VALUE_X_RIGHT + 4, LIST_Y_START + index * LIST_ITEM_H - 2);

  // Re-parent value label text color for edit highlight
  lv_obj_set_style_text_color(s_value_labels[index], lv_palette_main(LV_PALETTE_LIGHT_BLUE), 0);
}

static void hide_edit_box(void)
{
  if (s_edit_box)
  {
    lv_obj_delete(s_edit_box);
    s_edit_box = NULL;
  }
}

// ============================================================
// Apply value changes (auto-save + live preview)
// ============================================================
static void apply_change(int index)
{
  setting_item_t *item = &s_items[index];

  switch (index)
  {
  case 0: // Theme
  {
    bool is_light = (item->value == 1);
    settings_set_theme_light(is_light);
    ui_set_theme(is_light);
    ESP_LOGI(TAG, "Theme → %s", is_light ? "Light" : "Dark");
    break;
  }
  case 1: // Brightness
  {
    settings_set_brightness((uint8_t)item->value);
    bsp_display_set_brightness((uint8_t)item->value, 0);
    ESP_LOGI(TAG, "Brightness → %d%%", item->value);
    break;
  }
  default:
    break;
  }
}

// ============================================================
// Public API
// ============================================================
void settings_screen_create(lv_obj_t *parent)
{
  s_focus = 0;
  s_editing = false;
  s_edit_box = NULL;

  // Load current values from NVS
  s_items[0].value = settings_get_theme_light() ? 1 : 0;
  s_items[1].value = (int)settings_get_brightness();

  // Title
  lv_obj_t *title = lv_label_create(parent);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
  lv_label_set_text(title, "Settings");
  lv_obj_set_pos(title, LIST_X_PAD, TITLE_Y);

  // Focus marker ▸
  s_focus_marker = lv_label_create(parent);
  lv_obj_set_style_text_font(s_focus_marker, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(s_focus_marker, lv_palette_main(LV_PALETTE_LIGHT_BLUE), 0);
  lv_label_set_text(s_focus_marker, LV_SYMBOL_RIGHT);
  lv_obj_set_pos(s_focus_marker, LIST_X_PAD, LIST_Y_START);

  // Item rows
  for (int i = 0; i < SETTINGS_ITEM_COUNT; i++)
  {
    // Name label (left)
    s_name_labels[i] = lv_label_create(parent);
    lv_obj_set_style_text_font(s_name_labels[i], &lv_font_montserrat_14, 0);
    lv_label_set_text(s_name_labels[i], s_items[i].label);
    lv_obj_set_pos(s_name_labels[i], LIST_X_PAD + 18, LIST_Y_START + i * LIST_ITEM_H);

    // Value label (right-aligned) — only for toggle/range
    if (s_items[i].type != STYPE_ACTION)
    {
      s_value_labels[i] = lv_label_create(parent);
      lv_obj_set_style_text_font(s_value_labels[i], &lv_font_montserrat_14, 0);
      lv_obj_align(s_value_labels[i], LV_ALIGN_TOP_RIGHT, VALUE_X_RIGHT, LIST_Y_START + i * LIST_ITEM_H);
      update_value_text(i);
    }
    else
    {
      s_value_labels[i] = NULL;
    }
  }

  update_focus_visual();
}

void settings_screen_focus_prev(void)
{
  s_focus = (s_focus - 1 + SETTINGS_ITEM_COUNT) % SETTINGS_ITEM_COUNT;
  update_focus_visual();
}

void settings_screen_focus_next(void)
{
  s_focus = (s_focus + 1) % SETTINGS_ITEM_COUNT;
  update_focus_visual();
}

int settings_screen_get_focus(void)
{
  return s_focus;
}

void settings_screen_set_focus(int index)
{
  if (index >= 0 && index < SETTINGS_ITEM_COUNT)
  {
    s_focus = index;
    update_focus_visual();
  }
}

bool settings_screen_is_action_item(int index)
{
  if (index < 0 || index >= SETTINGS_ITEM_COUNT)
  {
    return false;
  }
  return s_items[index].type == STYPE_ACTION;
}

void settings_screen_execute_action(int index, nav_action_cb_t cb)
{
  if (index < 0 || index >= SETTINGS_ITEM_COUNT)
  {
    return;
  }
  if (s_items[index].type != STYPE_ACTION)
  {
    return;
  }

  ESP_LOGW(TAG, "Executing action: %s", s_items[index].label);
  if (cb)
  {
    cb();
  }
}

void settings_screen_enter_edit(int index)
{
  if (index < 0 || index >= SETTINGS_ITEM_COUNT)
  {
    return;
  }
  if (s_items[index].type == STYPE_ACTION)
  {
    return;
  }

  s_editing = true;
  update_focus_visual();
  show_edit_box(index);
  ESP_LOGI(TAG, "Edit: %s", s_items[index].label);
}

void settings_screen_exit_edit(void)
{
  s_editing = false;
  hide_edit_box();
  update_focus_visual();

  // Remove edit highlight from value label
  if (s_value_labels[s_focus])
  {
    lv_obj_remove_local_style_prop(s_value_labels[s_focus], LV_STYLE_TEXT_COLOR, 0);
  }

  ESP_LOGI(TAG, "Edit done");
}

void settings_screen_edit_increase(void)
{
  setting_item_t *item = &s_items[s_focus];

  switch (item->type)
  {
  case STYPE_TOGGLE:
    item->value = (item->value + 1) % item->option_count;
    break;
  case STYPE_RANGE:
    if (item->value + item->step <= item->max)
    {
      item->value += item->step;
    }
    else
    {
      item->value = item->max;
    }
    break;
  default:
    return;
  }

  update_value_text(s_focus);
  apply_change(s_focus);
}

void settings_screen_edit_decrease(void)
{
  setting_item_t *item = &s_items[s_focus];

  switch (item->type)
  {
  case STYPE_TOGGLE:
    item->value = (item->value - 1 + item->option_count) % item->option_count;
    break;
  case STYPE_RANGE:
    if (item->value - item->step >= item->min)
    {
      item->value -= item->step;
    }
    else
    {
      item->value = item->min;
    }
    break;
  default:
    return;
  }

  update_value_text(s_focus);
  apply_change(s_focus);
}
