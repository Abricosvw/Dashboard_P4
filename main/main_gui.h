#pragma once

#include "esp_err.h"
#include "esp_lcd_panel_ops.h"

/**
 * @brief Initialize LVGL and the UI
 *
 * @param panel_handle The hande to the initialized LCD panel
 * @return esp_err_t ESP_OK on success
 */
esp_err_t main_gui_init(esp_lcd_panel_handle_t panel_handle);

// LVGL Locking mechanism for FreeRTOS tasks
bool example_lvgl_lock(int timeout_ms);
void example_lvgl_unlock(void);
