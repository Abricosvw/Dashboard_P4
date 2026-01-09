#pragma once

#include "esp_err.h"
#include <stdbool.h>

// =============================================================================
// SDMMC SLOT CONFIGURATION
// VARIANT 1: ESP32-P4-WIFI6-DEV-KIT (Current Focus)
// VARIANT 2: ESP32-P4-Pico (Available for future use)
// =============================================================================

// SDMMC Slot 0 Pins for VARIANT 1 (Waveshare ESP32-P4-Module-DEV-KIT)
#define SD_SLOT_NUM 0
#define SD_PIN_CMD 39
#define SD_PIN_CLK 40
#define SD_PIN_D0 41
#define SD_PIN_D1 42
#define SD_PIN_D2 43
#define SD_PIN_D3 44
#define SD_FREQ_KHZ SDMMC_FREQ_DEFAULT // 20MHz for stability

#define SD_MOUNT_POINT "/sdcard"

/**
 * @brief Initialize and mount SD card
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sd_card_init(void);

/**
 * @brief Deinitialize and unmount SD card
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sd_card_deinit(void);

/**
 * @brief Check if SD card is mounted
 *
 * @return true if mounted
 */
bool sd_card_is_mounted(void);
