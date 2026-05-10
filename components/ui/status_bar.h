// SPDX-License-Identifier: MIT
// ZenClock — Status bar widget interface

#pragma once

#include "lvgl.h"

/**
 * @brief Create the status bar on the given parent.
 *
 * Creates battery icon + percentage label in the top-right corner
 * with a 30-second LVGL timer for automatic updates.
 * Timer fires immediately on first tick.
 *
 * Must be called inside lvgl_port_lock()/unlock().
 */
void status_bar_create(lv_obj_t *parent);
