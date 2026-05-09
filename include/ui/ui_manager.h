#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "esp_lcd_panel_ops.h"

/**
 * @brief Initialize the UI — draw header, RAM info, and prepare layout.
 *
 * Call this once after lcd_driver_init(). It clears the screen and draws
 * all static UI elements (header banner, memory stats).
 *
 * @param panel  Panel handle from lcd_driver_init()
 */
void ui_init(esp_lcd_panel_handle_t panel);

/**
 * @brief Update the counter display in-place.
 *
 * Draws the counter value at the pre-calculated position using
 * batched rendering to avoid flicker.
 *
 * @param counter  Current counter value
 */
void ui_update_counter(int counter);

/**
 * @brief Update a general status message in-place.
 *
 * @param msg  Null-terminated status message
 */
void ui_update_status(const char *msg);

#endif // UI_MANAGER_H
