#include "main_gui.h"
#include "display_init.h"
#include "driver/i2c_master.h"
#include "driver/ppa.h"
#include "esp_heap_caps.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch_ft5x06.h"
// #include "esp_lcd_touch_gt911.h" // Removed
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "ui/ui.h"
#include "ui/ui_screen_manager.h"
#include <stdio.h>

static const char *TAG = "MAIN_GUI";

// LVGL Task configuration
#define LVGL_TASK_STACK_SIZE (32 * 1024)
#define LVGL_TASK_PRIORITY (5)
#define LVGL_TICK_MS (5)

static lv_disp_draw_buf_t disp_buf;
static lv_disp_drv_t disp_drv;
static ppa_client_handle_t ppa_srm_handle = NULL;
static void *dsi_fb = NULL;
extern i2c_master_bus_handle_t i2c1_bus;
static SemaphoreHandle_t lvgl_mux = NULL;

bool example_lvgl_lock(int timeout_ms) {
  assert(lvgl_mux && "LVGL mutex not initialized");
  if (xSemaphoreTake(lvgl_mux, pdMS_TO_TICKS(timeout_ms)) == pdTRUE) {
    return true;
  }
  return false;
}

void example_lvgl_unlock(void) {
  assert(lvgl_mux && "LVGL mutex not initialized");
  xSemaphoreGive(lvgl_mux);
}

// LVGL flush callback using PPA for hardware rotation
static void flush_callback(lv_disp_drv_t *drv, const lv_area_t *area,
                           lv_color_t *color_map) {
  int offsetx1 = area->x1;
  int offsety1 = area->y1;
  int offsetx2 = area->x2;
  int offsety2 = area->y2;

  int w = offsetx2 - offsetx1 + 1;
  int h = offsety2 - offsety1 + 1;

  // logical landscape (1280x720) -> physical portrait (720x1280)
  // Logic: logical (x,y) -> physical (y, LCD_PHYS_V_RES - 1 - x)
  int phys_x = offsety1;
  int phys_y = LCD_PHYS_V_RES - 1 - offsetx2;

  ppa_srm_oper_config_t srm_config = {
      .in.buffer = color_map,
      .in.pic_w = w,
      .in.pic_h = h,
      .in.block_w = w,
      .in.block_h = h,
      .in.block_offset_x = 0,
      .in.block_offset_y = 0,
      .in.srm_cm = PPA_SRM_COLOR_MODE_RGB565,

      .out.buffer = dsi_fb + (phys_y * LCD_PHYS_H_RES + phys_x) * 2,
      .out.pic_w = LCD_PHYS_H_RES,
      .out.pic_h = LCD_PHYS_V_RES,
      .out.block_offset_x = 0,
      .out.block_offset_y = 0,
      .out.srm_cm = PPA_SRM_COLOR_MODE_RGB565,

      .rotation_angle = PPA_SRM_ROTATION_ANGLE_90,
      .scale_x = 1.0,
      .scale_y = 1.0,
      .mode = PPA_TRANS_MODE_BLOCKING,
  };

  ppa_do_scale_rotate_mirror(ppa_srm_handle, &srm_config);

  lv_disp_flush_ready(drv);
}

static void lvgl_tick_cb(void *arg) { lv_tick_inc(LVGL_TICK_MS); }

static void lvgl_port_task(void *arg) {
  ESP_LOGI(TAG, "Starting LVGL task");
  while (1) {
    if (example_lvgl_lock(100)) {
      uint32_t delay_ms = lv_timer_handler();
      example_lvgl_unlock();
      vTaskDelay(pdMS_TO_TICKS(delay_ms));
    } else {
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}

// Touchscreen read callback
static void touch_callback(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  // Touch disabled
  data->state = LV_INDEV_STATE_RELEASED;
}

static bool i2c_probe(uint8_t addr) {
  if (i2c1_bus == NULL)
    return false;
  esp_err_t ret = i2c_master_probe(i2c1_bus, addr, 50);
  if (ret == ESP_OK) {
    ESP_LOGI(TAG, "I2C probe 0x%02X found device!", addr);
  }
  return (ret == ESP_OK);
}

esp_err_t main_gui_init(esp_lcd_panel_handle_t panel_handle) {
  ESP_LOGI(TAG, "Initializing GUI...");

  // 0. Create Mutex
  lvgl_mux = xSemaphoreCreateRecursiveMutex();
  if (!lvgl_mux) {
    ESP_LOGE(TAG, "Failed to create LVGL mutex");
    return ESP_ERR_NO_MEM;
  }

  // 1. Initialize LVGL
  lv_init();

  // 2. Allocate draw buffers
  // Use internal RAM for speed if possible
  size_t buf_size = LCD_H_RES * 100 * sizeof(lv_color_t);
  lv_color_t *buf1 =
      heap_caps_malloc(buf_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
  lv_color_t *buf2 =
      heap_caps_malloc(buf_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
  if (!buf1 || !buf2) {
    ESP_LOGE(TAG, "Failed to allocate LVGL draw buffers");
    return ESP_ERR_NO_MEM;
  }
  ESP_LOGI(TAG, "LVGL using fast internal DMA RAM: 2 x %d KB",
           (int)(buf_size / 1024));

  lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LCD_H_RES * 100);

  // 3. Initialize Display Driver
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = LCD_H_RES;
  disp_drv.ver_res = LCD_V_RES;
  disp_drv.flush_cb = flush_callback;
  disp_drv.draw_buf = &disp_buf;
  disp_drv.user_data = panel_handle;
  lv_disp_drv_register(&disp_drv);

  // 4. Initialize PPA for hardware rotation
  // For DPI panels, we get the framebuffer using DPI specific call
  uint32_t fb_num = 1;
  ESP_ERROR_CHECK(
      esp_lcd_dpi_panel_get_frame_buffer(panel_handle, fb_num, &dsi_fb));
  ESP_LOGI(TAG, "DPI Framebuffer(s) found. Using base @%p", dsi_fb);

  ppa_client_config_t ppa_cfg = {
      .oper_type = PPA_OPERATION_SRM,
  };
  ESP_ERROR_CHECK(ppa_register_client(&ppa_cfg, &ppa_srm_handle));
  ESP_LOGI(TAG, "PPA SRM client registered");

  // 5. Initialize Touchscreen - SKIPPED (GT911 Removed)
  ESP_LOGW(TAG, "Touchscreen initialization skipped (GT911 removed)");

  // 6. Initialize UI
  ui_init();
  ui_screen_manager_init();
  ESP_LOGI(TAG, "UI initialized");

  // 7. Start LVGL timer and task
  const esp_timer_create_args_t lvgl_tick_timer_args = {
      .callback = &lvgl_tick_cb,
      .name = "lvgl_tick",
  };
  esp_timer_handle_t lvgl_tick_timer = NULL;
  esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
  esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_MS * 1000);

  xTaskCreatePinnedToCore(lvgl_port_task, "LVGL", LVGL_TASK_STACK_SIZE, NULL,
                          LVGL_TASK_PRIORITY, NULL, 1);

  ESP_LOGI(TAG, "LVGL GUI initialized successfully");
  return ESP_OK;
}
