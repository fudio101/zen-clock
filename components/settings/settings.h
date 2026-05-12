// SPDX-License-Identifier: MIT
#pragma once

#include <stdbool.h>

#ifdef __cplusplus

#endif

/**
 * @brief Initialize the NVS flash.
 * Must be called early in app_main before reading/writing settings.
 */
void settings_init(void);

/**
 * @brief Get the stored theme configuration.
 * @return true if light theme, false if dark theme (default).
 */
bool settings_get_theme_light(void);

/**
 * @brief Store the theme configuration to NVS.
 * @param is_light true for light theme, false for dark theme.
 */
void settings_set_theme_light(bool is_light);

#ifdef __cplusplus

#endif
