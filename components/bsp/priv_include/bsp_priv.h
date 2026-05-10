// SPDX-License-Identifier: MIT
// BSP internal function declarations shared between sub-modules.
// NOT part of the public API — do not include from outside bsp component.

#pragma once

#include <stdint.h>

// --- Backlight (bsp_backlight.c) ---
void bsp_backlight_setup(void);
void bsp_backlight_set(uint8_t percent, uint32_t fade_time_ms);
uint8_t bsp_backlight_get(void);

// --- Battery (bsp_battery.c) ---
void bsp_battery_setup(void);
