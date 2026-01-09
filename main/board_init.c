#include "display_init.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_lcd_ili9881c.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BOARD_INIT";
static bool s_backlight_init_done = false;
static i2c_master_dev_handle_t s_bk_i2c_handle = NULL;
static esp_lcd_touch_handle_t s_touch_handle = NULL;

esp_err_t board_init_backlight(i2c_master_bus_handle_t bus_handle) {
  if (bus_handle == NULL) {
    ESP_LOGE(TAG, "I2C bus handle is NULL");
    return ESP_ERR_INVALID_ARG;
  }

  i2c_device_config_t dev_cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = LCD_BK_I2C_ADDR,
      .scl_speed_hz = 400000,
  };

  ESP_RETURN_ON_ERROR(
      i2c_master_bus_add_device(bus_handle, &dev_cfg, &s_bk_i2c_handle), TAG,
      "Failed to add backlight I2C device");
  s_backlight_init_done = true;
  ESP_LOGI(TAG, "Backlight I2C device added at address 0x%02X",
           LCD_BK_I2C_ADDR);
  return ESP_OK;
}

esp_err_t board_set_backlight(uint32_t level_percent) {
  if (level_percent > 100)
    level_percent = 100;

  // Map 0-100% to 0-255
  uint8_t duty = (uint8_t)((255 * level_percent) / 100);

  ESP_LOGI(TAG, "Setting backlight to %u%% (Val=%u)",
           (unsigned int)level_percent, (unsigned int)duty);

  if (!s_backlight_init_done || !s_bk_i2c_handle) {
    ESP_LOGE(TAG, "Backlight not initialized");
    return ESP_ERR_INVALID_STATE;
  }

  uint8_t write_buf[2] = {LCD_BK_I2C_REG, duty};
  ESP_RETURN_ON_ERROR(
      i2c_master_transmit(s_bk_i2c_handle, write_buf, sizeof(write_buf), -1),
      TAG, "Failed to write backlight brightness");

  return ESP_OK;
}

esp_err_t board_init_touch(i2c_master_bus_handle_t bus_handle) {
  if (bus_handle == NULL) {
    ESP_LOGE(TAG, "I2C bus handle is NULL");
    return ESP_ERR_INVALID_ARG;
  }

  esp_lcd_panel_io_handle_t tp_io_handle = NULL;
  esp_lcd_panel_io_i2c_config_t tp_io_config =
      ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
  tp_io_config.scl_speed_hz = 400000;

  // Check if we need to set the address from Kconfig
  if (TOUCH_I2C_ADDR != 0) {
    tp_io_config.dev_addr = TOUCH_I2C_ADDR;
  }

  // Cast dev_addr to unsigned int to fix format string warning/error
  ESP_LOGI(TAG, "Initializing Touch IO at address 0x%02X...",
           (unsigned int)tp_io_config.dev_addr);
  ESP_RETURN_ON_ERROR(
      esp_lcd_new_panel_io_i2c(bus_handle, &tp_io_config, &tp_io_handle), TAG,
      "New Panel IO I2C failed");

  esp_lcd_touch_config_t tp_cfg = {
      .x_max = LCD_PHYS_H_RES,
      .y_max = LCD_PHYS_V_RES,
      .rst_gpio_num = (gpio_num_t)TOUCH_RST_IO,
      .int_gpio_num = (gpio_num_t)TOUCH_INT_IO,
      .levels =
          {
              .reset = 0,
              .interrupt = 0,
          },
      .flags =
          {
              .swap_xy = 0,
              .mirror_x = 0,
              .mirror_y = 0,
          },
  };

  ESP_LOGI(TAG, "Initializing Touch Driver (GT911)...");
  ESP_RETURN_ON_ERROR(
      esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &s_touch_handle), TAG,
      "New Touch GT911 failed");

  ESP_LOGI(TAG, "Touch initialized successfully");
  return ESP_OK;
}

esp_err_t board_init_display(esp_lcd_panel_handle_t *ret_panel) {
  ESP_LOGI(TAG, "Initializing MIPI DSI bus (ILI9881C Driver)");

  // 1. Create DSI Bus
  esp_lcd_dsi_bus_handle_t mipi_dsi_bus = NULL;
  esp_lcd_dsi_bus_config_t bus_config = ILI9881C_PANEL_BUS_DSI_2CH_CONFIG();
  ESP_RETURN_ON_ERROR(esp_lcd_new_dsi_bus(&bus_config, &mipi_dsi_bus), TAG,
                      "New DSI bus failed");

  // 2. Create Panel IO
  ESP_LOGI(TAG, "Installing panel IO");
  esp_lcd_panel_io_handle_t mipi_dbi_io = NULL;
  esp_lcd_dbi_io_config_t dbi_config = ILI9881C_PANEL_IO_DBI_CONFIG();
  ESP_RETURN_ON_ERROR(
      esp_lcd_new_panel_io_dbi(mipi_dsi_bus, &dbi_config, &mipi_dbi_io), TAG,
      "New DSI IO failed");

  // 3. Hardware Reset (if defined)
  if (LCD_RST_IO >= 0) {
    ESP_LOGI(TAG, "Performing hardware reset on GPIO %d...", LCD_RST_IO);
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LCD_RST_IO),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);
    gpio_set_level(LCD_RST_IO, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(LCD_RST_IO, 1);
    vTaskDelay(pdMS_TO_TICKS(200));
  }

  // 4. Configure DPI panel for 720x1280 (reduced bandwidth for slow PSRAM)
  ESP_LOGI(TAG, "Installing ILI9881C driver (720x1280 @ ~20Hz, RGB565)");
  esp_lcd_dpi_panel_config_t dpi_config = {
      .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
      .dpi_clock_freq_mhz = 15, // Drop to 15MHz to match 20MHz PSRAM bandwidth
      .virtual_channel = 0,
      .pixel_format =
          LCD_COLOR_PIXEL_FORMAT_RGB565, // RGB565 uses 33% less bandwidth
      .num_fbs = 2, // Double buffering to prevent flicker with PPA rotation
      .video_timing =
          {
              .h_size = LCD_PHYS_H_RES,
              .v_size = LCD_PHYS_V_RES,
              .hsync_back_porch = 200,
              .hsync_pulse_width = 40,
              .hsync_front_porch = 40,
              .vsync_back_porch = 20,
              .vsync_pulse_width = 10,
              .vsync_front_porch = 10,
          },
      .flags.use_dma2d = true,
  };

  ili9881c_vendor_config_t vendor_config = {
      .mipi_config =
          {
              .dsi_bus = mipi_dsi_bus,
              .dpi_config = &dpi_config,
              .lane_num = 2,
          },
  };

  const esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num = LCD_RST_IO, // GPIO 33 for hardware reset
      .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
      .bits_per_pixel = 16, // Matches RGB565 format
      .vendor_config = &vendor_config,
  };

  ESP_ERROR_CHECK(
      esp_lcd_new_panel_ili9881c(mipi_dbi_io, &panel_config, ret_panel));

  ESP_LOGI(TAG, "Initializing panel...");
  ESP_ERROR_CHECK(esp_lcd_panel_reset(*ret_panel));
  vTaskDelay(pdMS_TO_TICKS(100));
  ESP_ERROR_CHECK(esp_lcd_panel_init(*ret_panel));
  ESP_LOGI(TAG, "Panel initialized successfully!");

  return ESP_OK;
}
