#pragma once

#include "esp_err.h"

/**
 * @brief Initialize WiFi in Access Point (AP) mode.
 *
 * @param ssid The SSID for the AP
 * @param pass The password for the AP
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wifi_init_ap(const char *ssid, const char *pass);

/**
 * @brief Initialize WiFi in Station (STA) mode.
 *
 * @param ssid The SSID to connect to
 * @param pass The password for the network
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wifi_init_sta(const char *ssid, const char *pass);
