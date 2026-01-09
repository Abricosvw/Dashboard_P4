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
  ESP_LOGI(TAG, "Starting Dashboard_P4...");

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

  // 2. Initialize Master I2C Bus on Port 1 (Shared Pins)
  // Using pins from Kconfig
  ESP_LOGI(TAG, "Initializing I2C Master Bus...");
  i2c_master_bus_config_t i2c_bus_config = {
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .i2c_port = I2C_NUM_1,
      .scl_io_num = LCD_I2C_SCL_IO,
      .sda_io_num = LCD_I2C_SDA_IO,
      .glitch_ignore_cnt = 7,
      .flags.enable_internal_pullup = true,
  };
  ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &i2c1_bus));

  // 2.1 Touchscreen Hardware Reset and Strapping
  // The GT911 requires specific bootstrapping on INT pin during Reset to select I2C address.
  // Address 0x5D: INT High during Reset rising edge.
  // Address 0x14: INT Low during Reset rising edge.
  // We target 0x5D (default Kconfig).
  if (TOUCH_RST_IO >= 0 && TOUCH_INT_IO >= 0) {
      ESP_LOGI(TAG, "Performing Touch Reset & Strapping (RST:%d, INT:%d)...", TOUCH_RST_IO, TOUCH_INT_IO);

      gpio_config_t touch_io_conf = {
          .pin_bit_mask = (1ULL << TOUCH_RST_IO) | (1ULL << TOUCH_INT_IO),
          .mode = GPIO_MODE_OUTPUT,
          .intr_type = GPIO_INTR_DISABLE,
          .pull_down_en = 0,
          .pull_up_en = 0,
      };
      gpio_config(&touch_io_conf);

      // Reset Sequence for Address 0x5D
      // 1. RST Low, INT Low
      gpio_set_level(TOUCH_RST_IO, 0);
      gpio_set_level(TOUCH_INT_IO, 0);
      vTaskDelay(pdMS_TO_TICKS(20));

      // 2. INT High (to select 0x5D)
      gpio_set_level(TOUCH_INT_IO, 1);
      vTaskDelay(pdMS_TO_TICKS(5));

      // 3. RST High (Release Reset)
      gpio_set_level(TOUCH_RST_IO, 1);
      vTaskDelay(pdMS_TO_TICKS(20));

      // 4. Restore INT to Input
      gpio_set_direction(TOUCH_INT_IO, GPIO_MODE_INPUT);
      vTaskDelay(pdMS_TO_TICKS(100)); // Stabilization
  } else if (TOUCH_RST_IO >= 0) {
      // Fallback reset without strapping if INT not defined
      ESP_LOGI(TAG, "Performing Touch Reset (RST:%d)...", TOUCH_RST_IO);
      gpio_config_t touch_rst_conf = {
          .pin_bit_mask = (1ULL << TOUCH_RST_IO),
          .mode = GPIO_MODE_OUTPUT,
      };
      gpio_config(&touch_rst_conf);
      gpio_set_level(TOUCH_RST_IO, 0);
      vTaskDelay(pdMS_TO_TICKS(20));
      gpio_set_level(TOUCH_RST_IO, 1);
      vTaskDelay(pdMS_TO_TICKS(100));
  }

  ESP_LOGW(TAG, "Scanning I2C on Port 1...");
  for (int i = 1; i < 127; i++) {
    if (i2c_master_probe(i2c1_bus, i, 50) == ESP_OK) {
      ESP_LOGW(TAG, "Found 0x%02X", i);
    }
  }

  // 3. Initialize Shared Peripherals
  board_init_backlight(i2c1_bus);
  board_set_backlight(80);

  // Initialize Touch
  board_init_touch(i2c1_bus);

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
