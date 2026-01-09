#include "settings_manager.h"
#include "esp_log.h"
#include "sd_card_manager.h"
#include <stdio.h>
#include <string.h>


static const char *TAG = "SETTINGS";
#define SETTINGS_FILE SD_MOUNT_POINT "/settings.bin"

static app_settings_t s_settings = {.screen_brightness = 70,
                                    .can_logging_enabled = true};

esp_err_t app_settings_init(void) {
  if (!sd_card_is_mounted()) {
    ESP_LOGW(TAG, "SD card not mounted, using default settings");
    return ESP_OK;
  }

  FILE *f = fopen(SETTINGS_FILE, "rb");
  if (f == NULL) {
    ESP_LOGI(TAG, "Settings file not found, creating with defaults");
    return app_settings_save();
  }

  fread(&s_settings, sizeof(app_settings_t), 1, f);
  fclose(f);

  ESP_LOGI(TAG, "Settings loaded: Brightness=%lu, Logging=%s",
           (unsigned long)s_settings.screen_brightness,
           s_settings.can_logging_enabled ? "ON" : "OFF");

  return ESP_OK;
}

esp_err_t app_settings_save(void) {
  if (!sd_card_is_mounted()) {
    return ESP_ERR_INVALID_STATE;
  }

  FILE *f = fopen(SETTINGS_FILE, "wb");
  if (f == NULL) {
    ESP_LOGE(TAG, "Failed to open settings file for writing");
    return ESP_FAIL;
  }

  fwrite(&s_settings, sizeof(app_settings_t), 1, f);
  fclose(f);
  ESP_LOGI(TAG, "Settings saved to SD card");
  return ESP_OK;
}

app_settings_t *app_settings_get(void) { return &s_settings; }
