// Settings Configuration Implementation
#include "settings_config.h"
#include "can_parser.h" // For can_parser_set_platform
#include "ecu_data.h"
#include "sd_card_manager.h" // Use P4 SD manager
#include <esp_log.h>
#include <nvs_flash.h>
#include <stdio.h>

#include "../background_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <nvs.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "SETTINGS_CONFIG";
#define NVS_NAMESPACE "settings"
static touch_settings_t current_settings;

// SD card access protection (Thread safety for file I/O)
static SemaphoreHandle_t sd_card_mutex = NULL;

// Helper to serialize settings to a JSON string
static void settings_to_json(const touch_settings_t *settings, char *buffer,
                             size_t buffer_size) {
  system_settings_t *sys_settings = system_settings_get();

  // A more robust implementation would use a proper JSON library
  snprintf(buffer, buffer_size,
           "{\"sensitivity\":%d,\"demo_mode\":%s,\"screen3_enabled\":%s,\"nav_"
           "buttons_enabled\":%s,"
           "\"show_map\":%s,\"show_wastegate\":%s,\"show_tps\":%s,\"show_rpm\":"
           "%s,\"show_boost\":%s,\"show_tcu\":%s,"
           "\"show_oil_press\":%s,\"show_oil_temp\":%s,\"show_water_temp\":%s,"
           "\"show_fuel_press\":%s,\"show_battery\":%s,"
           "\"show_pedal\":%s,\"show_wg_pos\":%s,\"show_bov\":%s,\"show_tcu_"
           "req\":%s,\"show_tcu_act\":%s,\"show_eng_req\":%s,"
           "\"show_eng_act\":%s,\"show_limit_tq\":%s,\"can_platform\":%d}",
           settings->touch_sensitivity_level,
           settings->demo_mode_enabled ? "true" : "false",
           settings->screen3_enabled ? "true" : "false",
           settings->nav_buttons_enabled ? "true" : "false",
           sys_settings->show_map ? "true" : "false",
           sys_settings->show_wastegate ? "true" : "false",
           sys_settings->show_tps ? "true" : "false",
           sys_settings->show_rpm ? "true" : "false",
           sys_settings->show_boost ? "true" : "false",
           sys_settings->show_tcu ? "true" : "false",
           sys_settings->show_oil_press ? "true" : "false",
           sys_settings->show_oil_temp ? "true" : "false",
           sys_settings->show_water_temp ? "true" : "false",
           sys_settings->show_fuel_press ? "true" : "false",
           sys_settings->show_battery ? "true" : "false",
           sys_settings->show_pedal ? "true" : "false",
           sys_settings->show_wg_pos ? "true" : "false",
           sys_settings->show_bov ? "true" : "false",
           sys_settings->show_tcu_req ? "true" : "false",
           sys_settings->show_tcu_act ? "true" : "false",
           sys_settings->show_eng_req ? "true" : "false",
           sys_settings->show_eng_act ? "true" : "false",
           sys_settings->show_limit_tq ? "true" : "false",
           settings->can_platform);
}

// Helper to deserialize settings from a JSON string
static bool settings_from_json(const char *json_str,
                               touch_settings_t *settings) {
  system_settings_t *sys_settings = system_settings_get();

  // This is a very basic parser. A real implementation should use a JSON
  // library like cJSON.
  const char *sens_key = "\"sensitivity\":";
  const char *demo_key = "\"demo_mode\":";
  const char *s3_key = "\"screen3_enabled\":";
  const char *nav_key = "\"nav_buttons_enabled\":";
  const char *platform_key = "\"can_platform\":";

  char *sens_ptr = strstr(json_str, sens_key);
  char *demo_ptr = strstr(json_str, demo_key);
  char *s3_ptr = strstr(json_str, s3_key);
  char *nav_ptr = strstr(json_str, nav_key);
  char *platform_ptr = strstr(json_str, platform_key);

  if (sens_ptr && demo_ptr && s3_ptr) {
    // Parse sensitivity
    settings->touch_sensitivity_level = atoi(sens_ptr + strlen(sens_key));

    // Parse demo_mode
    char *demo_val_ptr = demo_ptr + strlen(demo_key);
    settings->demo_mode_enabled = (strncmp(demo_val_ptr, "true", 4) == 0);

    // Parse screen3_enabled
    char *s3_val_ptr = s3_ptr + strlen(s3_key);
    settings->screen3_enabled = (strncmp(s3_val_ptr, "true", 4) == 0);

    // Parse nav_buttons_enabled
    if (nav_ptr) {
      char *nav_val_ptr = nav_ptr + strlen(nav_key);
      settings->nav_buttons_enabled = (strncmp(nav_val_ptr, "true", 4) == 0);
    } else {
      settings->nav_buttons_enabled = DEFAULT_NAV_BUTTONS_ENABLED;
    }

    // Parse can_platform
    if (platform_ptr) {
      settings->can_platform = atoi(platform_ptr + strlen(platform_key));
    } else {
      settings->can_platform = DEFAULT_CAN_PLATFORM;
    }

// Parse Gauge Visibility Settings
// Helper macro for boolean parsing
#define PARSE_BOOL(key, target)                                                \
  do {                                                                         \
    const char *k = "\"" key "\":";                                            \
    char *p = strstr(json_str, k);                                             \
    if (p) {                                                                   \
      char *v = p + strlen(k);                                                 \
      target = (strncmp(v, "true", 4) == 0);                                   \
    }                                                                          \
  } while (0)

    PARSE_BOOL("show_map", sys_settings->show_map);
    PARSE_BOOL("show_wastegate", sys_settings->show_wastegate);
    PARSE_BOOL("show_tps", sys_settings->show_tps);
    PARSE_BOOL("show_rpm", sys_settings->show_rpm);
    PARSE_BOOL("show_boost", sys_settings->show_boost);
    PARSE_BOOL("show_tcu", sys_settings->show_tcu);

    PARSE_BOOL("show_oil_press", sys_settings->show_oil_press);
    PARSE_BOOL("show_oil_temp", sys_settings->show_oil_temp);
    PARSE_BOOL("show_water_temp", sys_settings->show_water_temp);
    PARSE_BOOL("show_fuel_press", sys_settings->show_fuel_press);
    PARSE_BOOL("show_battery", sys_settings->show_battery);

    PARSE_BOOL("show_pedal", sys_settings->show_pedal);
    PARSE_BOOL("show_wg_pos", sys_settings->show_wg_pos);
    PARSE_BOOL("show_bov", sys_settings->show_bov);
    PARSE_BOOL("show_tcu_req", sys_settings->show_tcu_req);
    PARSE_BOOL("show_tcu_act", sys_settings->show_tcu_act);
    PARSE_BOOL("show_eng_req", sys_settings->show_eng_req);

    PARSE_BOOL("show_eng_act", sys_settings->show_eng_act);
    PARSE_BOOL("show_limit_tq", sys_settings->show_limit_tq);

    return true;
  }
  return false;
}

void settings_init_defaults(touch_settings_t *settings) {
  if (settings == NULL)
    return;
  settings->touch_sensitivity_level = DEFAULT_TOUCH_SENSITIVITY;
  settings->demo_mode_enabled = DEFAULT_DEMO_MODE_ENABLED;
  settings->demo_mode_enabled = DEFAULT_DEMO_MODE_ENABLED;
  settings->screen3_enabled = DEFAULT_SCREEN3_ENABLED;
  settings->nav_buttons_enabled = DEFAULT_NAV_BUTTONS_ENABLED;
  settings->can_platform = DEFAULT_CAN_PLATFORM;
  for (int i = 0; i < SCREEN1_ARCS_COUNT; i++)
    settings->screen1_arcs_enabled[i] = true;
  for (int i = 0; i < SCREEN2_ARCS_COUNT; i++)
    settings->screen2_arcs_enabled[i] = true;

  ESP_LOGI(TAG,
           "Initialized default settings: Demo=%s, Screen3=%s, NavButtons=%s, "
           "Sensitivity=%d",
           settings->demo_mode_enabled ? "ON" : "OFF",
           settings->screen3_enabled ? "ON" : "OFF",
           settings->nav_buttons_enabled ? "ON" : "OFF",
           settings->touch_sensitivity_level);
}

/**
 * @brief Saves the provided settings struct to the SD card.
 * This is a slow, blocking function and should only be called from a background
 * task or during initial setup.
 * @param settings_to_save A pointer to the settings struct to save.
 */
void settings_save(const touch_settings_t *settings_to_save) {
  if (settings_to_save == NULL) {
    ESP_LOGE(TAG, "settings_save called with NULL data!");
    return;
  }

  // Check if SD card is mounted
  if (!sd_card_is_mounted()) {
    ESP_LOGW(TAG, "SD card not mounted, skipping save.");
    return;
  }

  // Save to SD Card as JSON
  char json_buffer[1024];
  settings_to_json(settings_to_save, json_buffer, sizeof(json_buffer));

  ESP_LOGI(TAG, "Attempting to save settings to SD card...");

  // Take mutex before SD card access
  if (sd_card_mutex == NULL ||
      xSemaphoreTake(sd_card_mutex, pdMS_TO_TICKS(2000)) != pdTRUE) {
    ESP_LOGE(TAG,
             "Failed to take SD card mutex for writing, operation aborted");
    return;
  }

  // Save settings as .txt file with 8.3 filename format
  // Use standard C file I/O operations
  FILE *f = fopen(SD_MOUNT_POINT "/settings.cfg", "w");
  esp_err_t result = ESP_FAIL;

  if (f != NULL) {
    size_t written = fwrite(json_buffer, 1, strlen(json_buffer), f);
    fclose(f);
    if (written == strlen(json_buffer)) {
      result = ESP_OK;
    } else {
      ESP_LOGE(TAG, "Incomplete write");
    }
  } else {
    ESP_LOGE(TAG, "Failed to open settings file for writing");
  }

  // Release mutex after file operations
  xSemaphoreGive(sd_card_mutex);

  if (result == ESP_OK) {
    ESP_LOGI(TAG, "Settings saved to SD card successfully.");
  } else {
    ESP_LOGE(TAG, "Failed to save settings to SD card.");
  }
}

/**
 * @brief Queues a request to save the current settings in a background task.
 * This function makes a copy of the current settings to pass to the background
 * task.
 */
void trigger_settings_save(void) {
  // Allocate memory for a copy of the settings to ensure thread safety.
  touch_settings_t *settings_copy = malloc(sizeof(touch_settings_t));
  if (settings_copy == NULL) {
    ESP_LOGE(TAG, "Failed to allocate memory for settings copy!");
    return;
  }

  // Copy the current settings to the new memory block
  memcpy(settings_copy, &current_settings, sizeof(touch_settings_t));

  background_task_t task = {.type = BG_TASK_SETTINGS_SAVE,
                            .data =
                                settings_copy, // Pass the pointer to the copy
                            .data_size = sizeof(touch_settings_t),
                            .callback = NULL};

  if (background_task_add(&task) != ESP_OK) {
    ESP_LOGE(TAG, "Failed to queue settings save task. Queue might be full.");
    free(settings_copy); // Free memory if task queuing fails
  } else {
    ESP_LOGI(TAG, "Settings save queued for background processing.");
  }
}

/**
 * @brief Loads settings from the SD card. If it fails, loads defaults.
 */
esp_err_t settings_load(void) {
  // Initialize SD card mutex if not already done
  if (sd_card_mutex == NULL) {
    sd_card_mutex = xSemaphoreCreateMutex();
    if (sd_card_mutex == NULL) {
      ESP_LOGE(TAG, "Failed to create SD card mutex!");
      settings_init_defaults(&current_settings);
      return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "SD card access mutex created successfully");
  }

  // Check if SD card is mounted
  if (!sd_card_is_mounted()) {
    ESP_LOGW(TAG, "SD card not mounted. Using default settings.");
    settings_init_defaults(&current_settings);
    return ESP_ERR_INVALID_STATE;
  }

  // Try to load from SD card with mutex protection
  ESP_LOGI(TAG, "Attempting to load settings from SD card...");

  // Take mutex before SD card access
  if (xSemaphoreTake(sd_card_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
    ESP_LOGW(TAG, "Failed to take SD card mutex for reading, using defaults");
    settings_init_defaults(&current_settings);
    return ESP_ERR_TIMEOUT;
  }

  FILE *f = fopen(SD_MOUNT_POINT "/settings.cfg", "r");
  if (f != NULL) {
    char buffer[1024] = {0};
    size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, f);

    fclose(f);

    // Release mutex after file operations
    xSemaphoreGive(sd_card_mutex);

    ESP_LOGI(TAG, "Read %d bytes from settings.cfg: %s", bytes_read, buffer);

    if (settings_from_json(buffer, &current_settings)) {
      ESP_LOGI(TAG, "Settings loaded from settings.cfg successfully.");
      ESP_LOGI(
          TAG,
          "Loaded settings: Demo=%s, Screen3=%s, NavButtons=%s, Sensitivity=%d",
          current_settings.demo_mode_enabled ? "ON" : "OFF",
          current_settings.screen3_enabled ? "ON" : "OFF",
          current_settings.nav_buttons_enabled ? "ON" : "OFF",
          current_settings.touch_sensitivity_level);

      // Apply platform setting
      can_parser_set_platform(current_settings.can_platform);

      return ESP_OK;
    } else {
      ESP_LOGW(TAG, "Failed to parse settings.cfg, using defaults.");
      settings_init_defaults(&current_settings);
      return ESP_FAIL;
    }
  }

  // Release mutex if file couldn't be opened
  xSemaphoreGive(sd_card_mutex);

  // If file doesn't exist or can't be opened, use defaults and try to create
  // the file.
  ESP_LOGI(TAG,
           "settings.cfg not found on SD card, initializing with defaults.");
  settings_init_defaults(&current_settings);

  // Attempt to save the new default settings to the SD card.
  // This is a blocking call, but it only happens once on the very first boot.
  ESP_LOGI(TAG, "Attempting to create default settings file...");
  settings_save(&current_settings);

  return ESP_ERR_NOT_FOUND; // Return NOT_FOUND to indicate that defaults were
                            // loaded.
}

// ... other functions like settings_validate, getters/setters, etc. remain the
// same ...

bool settings_validate(touch_settings_t *settings) {
  if (settings == NULL)
    return false;
  if (settings->touch_sensitivity_level < MIN_TOUCH_SENSITIVITY ||
      settings->touch_sensitivity_level > MAX_TOUCH_SENSITIVITY)
    return false;
  return true;
}
void settings_print_debug(touch_settings_t *settings) {
  if (settings == NULL)
    return;
  ESP_LOGI(TAG, "Settings Debug: Touch=%d, Demo=%s, Screen3=%s",
           settings->touch_sensitivity_level,
           settings->demo_mode_enabled ? "ON" : "OFF",
           settings->screen3_enabled ? "ON" : "OFF");
}
bool demo_mode_get_enabled(void) { return current_settings.demo_mode_enabled; }
void demo_mode_set_enabled(bool enabled) {
  current_settings.demo_mode_enabled = enabled;
}
bool screen3_get_enabled(void) { return current_settings.screen3_enabled; }
void screen3_set_enabled(bool enabled) {
  current_settings.screen3_enabled = enabled;
}
bool nav_buttons_get_enabled(void) {
  return current_settings.nav_buttons_enabled;
}
void nav_buttons_set_enabled(bool enabled) {
  current_settings.nav_buttons_enabled = enabled;
}

CanPlatform settings_get_can_platform(void) {
  return current_settings.can_platform;
}
void settings_set_can_platform(CanPlatform platform) {
  current_settings.can_platform = platform;
  // Apply immediately
  can_parser_set_platform(platform);
}
void settings_apply_changes(void) {
  ESP_LOGI(TAG, "Applying settings changes...");
}
void settings_reset_to_defaults(void) {
  ESP_LOGI(TAG, "Resetting settings to defaults in memory");
  settings_init_defaults(&current_settings);
  settings_apply_changes();
}
bool screen1_arc_get_enabled(int arc_index) {
  if (arc_index < 0 || arc_index >= SCREEN1_ARCS_COUNT)
    return false;
  return current_settings.screen1_arcs_enabled[arc_index];
}
void screen1_arc_set_enabled(int arc_index, bool enabled) {
  if (arc_index < 0 || arc_index >= SCREEN1_ARCS_COUNT)
    return;
  current_settings.screen1_arcs_enabled[arc_index] = enabled;
}
bool screen2_arc_get_enabled(int arc_index) {
  if (arc_index < 0 || arc_index >= SCREEN2_ARCS_COUNT)
    return false;
  return current_settings.screen2_arcs_enabled[arc_index];
}
void screen2_arc_set_enabled(int arc_index, bool enabled) {
  if (arc_index < 0 || arc_index >= SCREEN2_ARCS_COUNT)
    return;
  current_settings.screen2_arcs_enabled[arc_index] = enabled;
}
void ui_Screen1_update_arcs_visibility(void) {
  ESP_LOGD(TAG, "Screen1 arcs visibility update requested");
}
void ui_Screen2_update_arcs_visibility(void) {
  ESP_LOGD(TAG, "Screen2 arcs visibility update requested");
}
void demo_mode_test_toggle(void) {
  current_settings.demo_mode_enabled = !current_settings.demo_mode_enabled;
}
void demo_mode_status_report(void) {
  ESP_LOGI(TAG, "Demo Mode Status: %s",
           current_settings.demo_mode_enabled ? "ENABLED" : "DISABLED");
}
