#include "display_init.h"
#include "driver/ledc.h" // Needed for PWM
#include "esp_check.h"
#include "esp_lcd_ili9881c.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BOARD_INIT";
static bool s_backlight_init_done = false;

esp_err_t board_init_backlight(i2c_port_t port) {
  // Initialize LEDC PWM for backlight control on GPIO 26
  ledc_timer_config_t ledc_timer = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .duty_resolution = LEDC_TIMER_8_BIT,
      .timer_num = LEDC_TIMER_0,
      .freq_hz = 5000,
      .clk_cfg = LEDC_AUTO_CLK,
  };
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  ledc_channel_config_t ledc_channel = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = LEDC_CHANNEL_0,
      .timer_sel = LEDC_TIMER_0,
      .intr_type = LEDC_INTR_DISABLE,
      .gpio_num = LCD_BK_LIGHT_IO, // GPIO 26
      .duty = 0,
      .hpoint = 0,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  s_backlight_init_done = true;
  ESP_LOGI(TAG, "Backlight PWM initialized on GPIO %d", LCD_BK_LIGHT_IO);
  return ESP_OK;
}

esp_err_t board_set_backlight(uint32_t level_percent) {
  if (level_percent > 100)
    level_percent = 100;

  // Map 0-100% to 0-255 duty cycle
  uint32_t duty = (uint32_t)((255 * level_percent) / 100);

  ESP_LOGI(TAG, "Setting backlight PWM duty to %u%% (Val=%u)",
           (unsigned int)level_percent, (unsigned int)duty);

  if (!s_backlight_init_done) {
    ESP_LOGE(TAG, "Backlight not initialized");
    return ESP_ERR_INVALID_STATE;
  }

  ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty));
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));

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

  // 3. Hardware Reset (GPIO 33)
  ESP_LOGI(TAG, "Performing hardware reset on GPIO 33...");
  gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << LCD_RST_IO),
      .mode = GPIO_MODE_OUTPUT,
  };
  gpio_config(&io_conf);
  gpio_set_level(LCD_RST_IO, 0);
  vTaskDelay(pdMS_TO_TICKS(100));
  gpio_set_level(LCD_RST_IO, 1);
  vTaskDelay(pdMS_TO_TICKS(200));

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
  // Note: DSI panels don't support hardware swap_xy/mirror
  // Rotation is handled via LVGL software rotation

  return ESP_OK;
}
