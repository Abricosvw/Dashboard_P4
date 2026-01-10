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

  // 1. Initialize Power (LDOs) - CRITICAL: Must be first (~5 ms)
  // LDO4 (3.3V) is required for SD card, Display Logic, and Peripherals
  ESP_LOGI(TAG, "1. Initializing Power (LDOs)...");
  esp_ldo_channel_handle_t ldo4 = NULL;
  esp_ldo_channel_config_t cfg4 = {
      .chan_id = 4,
      .voltage_mv = 3300,
  };
  ESP_ERROR_CHECK(esp_ldo_acquire_channel(&cfg4, &ldo4));
  vTaskDelay(pdMS_TO_TICKS(10)); // Allow power rails to stabilize

  // 2. Initialize Master I2C Bus (~50 ms)
  // Required for Touch, Audio Codec Control, and Backlight
  ESP_LOGI(TAG, "2. Initializing I2C Master Bus...");
  i2c_master_bus_config_t i2c_bus_config = {
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .i2c_port = I2C_NUM_0, // Port 0 for Waveshare P4 Boards
      .scl_io_num = LCD_I2C_SCL_IO,
      .sda_io_num = LCD_I2C_SDA_IO,
      .glitch_ignore_cnt = 7,
      .flags.enable_internal_pullup = true,
  };
  ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &i2c1_bus));

  // 2.1 Hardware Resets (Display and Touch) - CRITICAL
  // Must happen before we try to talk to these devices over I2C/DSI
  if (TOUCH_RST_IO >= 0 && TOUCH_INT_IO >= 0) {
      ESP_LOGI(TAG, "   Performing Touch Reset & Strapping...");
      gpio_config_t touch_io_conf = {
          .pin_bit_mask = (1ULL << TOUCH_RST_IO) | (1ULL << TOUCH_INT_IO),
          .mode = GPIO_MODE_OUTPUT,
          .intr_type = GPIO_INTR_DISABLE,
          .pull_down_en = 0,
          .pull_up_en = 0,
      };
      gpio_config(&touch_io_conf);

      // Reset Sequence for Address 0x5D (INT High during Reset rising edge)
      gpio_set_level(TOUCH_RST_IO, 0);
      gpio_set_level(TOUCH_INT_IO, 0);
      vTaskDelay(pdMS_TO_TICKS(20));

      gpio_set_level(TOUCH_INT_IO, 1);
      vTaskDelay(pdMS_TO_TICKS(5));

      gpio_set_level(TOUCH_RST_IO, 1);
      vTaskDelay(pdMS_TO_TICKS(50));

      // Restore INT to Input for driver usage
      gpio_set_direction(TOUCH_INT_IO, GPIO_MODE_INPUT);
  }

  // 3. Initialize Display & GUI (~300-500 ms)
  // Priority: Get the screen showing something ASAP.
  ESP_LOGI(TAG, "3. Initializing Display & GUI...");
  board_init_backlight(i2c1_bus);
  board_set_backlight(0); // Keep backlight off during initialization to avoid artifacts

  board_init_touch(i2c1_bus); // Init touch driver

  esp_lcd_panel_handle_t panel = NULL;
  if (board_init_display(&panel) == ESP_OK) {
    // Initialize LVGL, create UI, start drawing task
    main_gui_init(panel);

    // Now that UI is active, turn on backlight
    vTaskDelay(pdMS_TO_TICKS(100)); // Allow one frame to render
    board_set_backlight(100);
    ESP_LOGI(TAG, "   Display is ON.");
  }

  // 4. Initialize SD Card (~100 ms)
  // Load settings and assets.
  ESP_LOGI(TAG, "4. Initializing SD Card...");
  gpio_config_t sd_d3_conf = {
      .pin_bit_mask = (1ULL << 42),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = 1,
      .intr_type = GPIO_INTR_DISABLE,
  };
  gpio_config(&sd_d3_conf);

  if (sd_card_init() == ESP_OK) {
    ESP_LOGI(TAG, "   SD Card Mounted.");
    app_settings_init();
  } else {
    ESP_LOGW(TAG, "   SD Card failed/missing.");
  }

  // 5. Audio Init (~20 ms)
  // Initialize I2S and Codec. Now can access SD card for sounds.
  ESP_LOGI(TAG, "5. Initializing Audio...");
  audio_init();
  // Optional: Play boot sound here if available
  // audio_play_wav("/sdcard/boot.wav");

  // 6. Initialize WiFi (Last / Background) (~1000-3000 ms)
  // Connecting to WiFi is slow, so we do it last to not block the UI startup.
  ESP_LOGI(TAG, "6. Initializing WiFi (Background)...");
  wifi_init_sta(WIFI_SSID, WIFI_PASS);

  // 7. Start Component Managers (Everything else)
  can_init();
  // ai_assistant_init();

  ESP_LOGI(TAG, "System Boot Complete!");

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
