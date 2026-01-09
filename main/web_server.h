#pragma once

#include "esp_err.h"

/**
 * @brief Start the web file manager server
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t web_server_start(void);

/**
 * @brief Stop the web file manager server
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t web_server_stop(void);
