#include "main_gui.h"
#include "display_init.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "ui/ui.h"

static const char *TAG = "MAIN_GUI";

// LVGL lock function for thread-safe access
bool example_lvgl_lock(int timeout_ms) { return lvgl_port_lock(timeout_ms); }

// LVGL unlock function
void example_lvgl_unlock(void) { lvgl_port_unlock(); }

static void touch_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_indev_t *indev = lv_event_get_target(e);

  if (code == LV_EVENT_PRESSED) {
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    ESP_LOGI(TAG, "TOUCH: Pressed at (%d, %d)", (int)p.x, (int)p.y);
  } else if (code == LV_EVENT_RELEASED) {
    ESP_LOGI(TAG, "TOUCH: Released");
  }
}

esp_err_t main_gui_init(esp_lcd_panel_handle_t panel_handle,
                        esp_lcd_panel_io_handle_t io_handle,
                        esp_lcd_touch_handle_t touch_handle) {
  ESP_LOGI(TAG, "Initializing GUI...");

  // 1. Initialize LVGL port (creates LVGL task and mutex)
  ESP_LOGI(TAG, "  Initializing LVGL port...");
  const lvgl_port_cfg_t lvgl_cfg = {
      .task_priority = 5,
      .task_stack = 32768,
      .task_affinity = 0, // Привязка к ядру 0 (стабильно для графики)
      .task_max_sleep_ms = 100,
      .timer_period_ms = 33, // ~30fps
  };
  esp_err_t ret = lvgl_port_init(&lvgl_cfg);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "LVGL port init failed: %s", esp_err_to_name(ret));
    return ret;
  }

  // 2. Add display to LVGL with PPA rotation support
  // Display uses physical resolution (720x1280), LVGL will rotate to logical
  // (1280x720)
  ESP_LOGI(TAG, "  Adding DSI display to LVGL (%dx%d) with PPA rotation...",
           LCD_PHYS_H_RES, LCD_PHYS_V_RES);

  const lvgl_port_display_cfg_t disp_cfg = {
      .io_handle = io_handle,
      .panel_handle = panel_handle,
      .buffer_size =
          LCD_PHYS_H_RES *
          LCD_PHYS_V_RES, // Полный буфер для идеальной графики Arc/LED
      .double_buffer = true,
      .trans_size = 0,
      .hres = LCD_PHYS_H_RES, // 720 (portrait width)
      .vres = LCD_PHYS_V_RES, // 1280 (portrait height)
      .monochrome = false,
      .flags =
          {
              .buff_dma = false,
              .buff_spiram = true, // Работаем через PSRAM
              .sw_rotate =
                  true, // Включаем SW ротацию - будет использовать PPA!
          },
  };

  const lvgl_port_display_dsi_cfg_t dsi_cfg = {
      .flags =
          {
              .avoid_tearing = false,
          },
  };

  lv_display_t *disp = lvgl_port_add_disp_dsi(&disp_cfg, &dsi_cfg);
  if (disp == NULL) {
    ESP_LOGE(TAG, "Failed to add DSI display to LVGL");
    return ESP_FAIL;
  }

  // 3. Set rotation to 90 degrees for landscape mode (1280x720)
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);
  lv_display_set_render_mode(
      disp,
      LV_DISPLAY_RENDER_MODE_FULL); // Режим полного кадра (идеально для Arc)
  ESP_LOGI(TAG, "  Display rotated and set to FULL REFRESH mode");

  // 4. Add touch input to LVGL
  if (touch_handle) {
    ESP_LOGI(TAG, "  Adding touch input to LVGL...");
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = disp,
        .handle = touch_handle,
        .scale =
            {
                .x = 1.0f,
                .y = 1.0f,
            },
    };
    lv_indev_t *indev = lvgl_port_add_touch(&touch_cfg);
    if (indev) {
      ESP_LOGI(TAG, "  Touch input device added successfully");
      if (lvgl_port_lock(1000)) {
        lv_indev_add_event_cb(indev, touch_event_cb, LV_EVENT_ALL, NULL);
        lvgl_port_unlock();
      }
    } else {
      ESP_LOGW(TAG, "  Failed to add touch input device");
    }
  }

  // 5. Initialize UI (with LVGL lock)
  ESP_LOGI(TAG, "  Initializing SquareLine UI...");
  if (lvgl_port_lock(1000)) {
    ui_init();
    lvgl_port_unlock();
    ESP_LOGI(TAG, "GUI initialized successfully with PPA rotation!");
    return ESP_OK;
  }

  ESP_LOGE(TAG, "Failed to acquire LVGL lock for UI init");
  return ESP_FAIL;
}
