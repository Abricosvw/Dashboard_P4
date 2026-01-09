#include "ai_manager.h"
#include "audio_manager.h"
#include "background_task.h"
#include "can_manager.h"
#include "display_init.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_flash.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "include/can_websocket.h"
#include "main_gui.h"
#include "sd_card_manager.h"
#include "settings_manager.h"
#include "wifi_init.h"
#include <dirent.h>
#include <stdio.h>


static const char *TAG = "MAIN";

// WIFI Credentials
#define WIFI_SSID "ESP32P4_Dashboard"
#define WIFI_PASS "12345678"

// Shared I2C Bus Handle (Used by main_gui and audio)
i2c_master_bus_handle_t i2c1_bus = NULL;

// Helper to list files on SD card
void list_sd_files(const char *path) {
  ESP_LOGI(TAG, "Listing files in %s:", path);
  DIR *dir = opendir(path);
  if (!dir) {
    ESP_LOGE(TAG, "Failed to open directory %s", path);
    return;
  }
  struct dirent *de;
  while ((de = readdir(dir)) != NULL) {
    ESP_LOGI(TAG, "  %s", de->d_name);
  }
  closedir(dir);
}

void app_main(void) {
  // SET ACTIVE VARIANT HERE
  // Target: VARIANT 1 (ESP32-P4-WIFI6-DEV-KIT)
  ESP_LOGI(TAG, "Starting Dashboard_P4 (VARIANT 1: P4-WIFI6-DEV-KIT)...");

  // 1. Initialize Power (LDOs)
  // LDO4 is required for SD card and some peripherals
  ESP_LOGI(TAG, "Initializing LDOs...");
  esp_ldo_channel_handle_t ldo4 = NULL;
  esp_ldo_channel_config_t cfg4 = {
      .chan_id = 4,
      .voltage_mv = 3300,
  };
  ESP_ERROR_CHECK(esp_ldo_acquire_channel(&cfg4, &ldo4));
  vTaskDelay(pdMS_TO_TICKS(500));

  // 2. Initialize Master I2C Bus on Port 1 (Shared Pins 7, 8)
  ESP_LOGI(TAG, "Initializing I2C Master Bus on Port 1...");
  i2c_master_bus_config_t i2c_bus_config = {
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .i2c_port = I2C_NUM_1,
      .scl_io_num = 8,
      .sda_io_num = 7,
      .glitch_ignore_cnt = 7,
      .flags.enable_internal_pullup = true,
  };
  ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &i2c1_bus));

  // 2.1 Touchscreen Hardware Reset (CRITICAL: Must happen BEFORE I2C scan)
  ESP_LOGI(TAG, "Standard reset via GPIO 5 and 4 (Variant 1)...");
  gpio_config_t touch_io_conf = {
      .pin_bit_mask = (1ULL << 5) | (1ULL << 4), // 5=RST, 4=INT
      .mode = GPIO_MODE_OUTPUT,
      .intr_type = GPIO_INTR_DISABLE,
      .pull_down_en = 0,
      .pull_up_en = 0,
  };
  gpio_config(&touch_io_conf);
  gpio_set_level(5, 0);
  gpio_set_level(4, 0);
  vTaskDelay(pdMS_TO_TICKS(20));
  gpio_set_level(5, 1);
  vTaskDelay(pdMS_TO_TICKS(20));
  gpio_set_direction(4, GPIO_MODE_INPUT); // INT back to input
  vTaskDelay(pdMS_TO_TICKS(100));         // Stabilization

  ESP_LOGW(TAG, "Scanning I2C on Port 1...");
  for (int i = 1; i < 127; i++) {
    if (i2c_master_probe(i2c1_bus, i, 50) == ESP_OK) {
      ESP_LOGW(TAG, "Found 0x%02X", i);
    }
  }

  // 3. Initialize Shared Peripherals
  board_init_backlight(I2C_NUM_1);
  board_set_backlight(80);

  // 4. Initialize Audio (Early to avoid bus contention)
  audio_init();

  // 5. Initialize SD Card and Settings (BEFORE WIFI to get host priority)
  ESP_LOGI(TAG, "Initializing SD Card...");
  // Ensure GPIO 42 (D3) is an input with pullup before init
  gpio_config_t sd_d3_conf = {
      .pin_bit_mask = (1ULL << 42),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = 1,
      .intr_type = GPIO_INTR_DISABLE,
  };
  gpio_config(&sd_d3_conf);

  if (sd_card_init() == ESP_OK) {
    ESP_LOGI(TAG, "Sd Card OK. Listing files...");
    list_sd_files("/sdcard");
    app_settings_init();
  } else {
    ESP_LOGE(TAG, "SD Card initialization failed!");
  }

  // 6. Initialize WiFi
  ESP_LOGI(TAG, "Initializing WiFi...");
  wifi_init_sta(WIFI_SSID, WIFI_PASS);
  vTaskDelay(pdMS_TO_TICKS(1000));

  // 7. Initialize Display and GUI
  esp_lcd_panel_handle_t panel = NULL;
  if (board_init_display(&panel) == ESP_OK) {
    main_gui_init(panel);
  }

  // 8. Start Component Managers
  can_init();
  // ai_assistant_init(); // Needs specific configuration

  ESP_LOGI(TAG, "System Ready!");

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
