#pragma once

#include "esp_err.h"
#include <stdbool.h>

typedef struct {
  uint32_t screen_brightness;
  bool can_logging_enabled;
  // Add more settings here as needed
} app_settings_t;

/**
 * @brief Initialize settings (load from SD if possible, otherwise use defaults)
 *
 * @return esp_err_t
 */
esp_err_t app_settings_init(void);

/**
 * @brief Save current settings to SD card
 *
 * @return esp_err_t
 */
esp_err_t app_settings_save(void);

/**
 * @brief Get current settings
 *
 * @return app_settings_t*
 */
app_settings_t *app_settings_get(void);
