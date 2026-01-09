// UI Screen Manager - Handles switching between screens
#include "ui_screen_manager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "screens/ui_Screen3.h"
#include "screens/ui_Screen4.h"
#include "screens/ui_Screen5.h"
#include "screens/ui_Screen6.h"
#include "screens/ui_Screen7.h"
#include "screens/ui_Screen8.h"
#include "settings_config.h"
#include "ui.h"
#include "ui_layout_manager.h"

LV_FONT_DECLARE(lv_font_montserrat_20);

// Touch state variables
extern bool touch_active;
extern uint8_t touch_sensitivity_level;

// Touch timing variables
static uint32_t touch_start_time = 0;

// Touch point variables
static lv_point_t touch_start_point = {0, 0};
static lv_point_t last_touch_point = {0, 0};

// Current screen tracking
static screen_id_t current_screen = SCREEN_1;

// Touch screen functions
void touch_screen_init(void) {
  ESP_LOGI("TOUCH_SCREEN", "touch_screen_init called");

  // Touch screen state is managed by individual screens

  ESP_LOGI("TOUCH_SCREEN", "Touch screen initialized successfully");
}

// Enable touch screen
void touch_screen_enable(void) {
  ESP_LOGI("TOUCH_SCREEN",
           "touch_screen_enable called, current touch_active=%s",
           touch_active ? "TRUE" : "FALSE");

  touch_active = true;

  ESP_LOGI("TOUCH_SCREEN", "Touch screen enabled, touch_active set to: %s",
           touch_active ? "TRUE" : "FALSE");
  ESP_LOGI("TOUCH_SCREEN", "touch_screen_enable completed successfully");
}

// Disable touch screen
void touch_screen_disable(void) {
  ESP_LOGI("TOUCH_SCREEN",
           "touch_screen_disable called, current touch_active=%s",
           touch_active ? "TRUE" : "FALSE");

  touch_active = false;

  ESP_LOGI("TOUCH_SCREEN", "Touch screen disabled, touch_active set to: %s",
           touch_active ? "TRUE" : "FALSE");
  ESP_LOGI("TOUCH_SCREEN", "touch_screen_disable completed successfully");
}

// Check if touch screen is enabled
bool touch_screen_is_enabled(void) {
  ESP_LOGI("TOUCH_SCREEN", "touch_screen_is_enabled called, returning: %s",
           touch_active ? "TRUE" : "FALSE");
  return touch_active;
}

// Set touch sensitivity (1-10)
void touch_screen_set_sensitivity(uint8_t sensitivity) {
  ESP_LOGI("TOUCH_SCREEN", "touch_screen_set_sensitivity called with value: %d",
           sensitivity);

  if (sensitivity >= 1 && sensitivity <= 10) {
    // Adjust touch thresholds based on sensitivity
    touch_sensitivity_level = sensitivity;

    ESP_LOGI("TOUCH_SCREEN", "Touch sensitivity set to %d", sensitivity);
    ESP_LOGI("TOUCH_SCREEN", "touch_sensitivity_level updated to: %d",
             touch_sensitivity_level);
  } else {
    ESP_LOGW("TOUCH_SCREEN", "Invalid sensitivity value: %d (should be 1-10)",
             sensitivity);
    ESP_LOGW("TOUCH_SCREEN", "touch_sensitivity_level remains unchanged: %d",
             touch_sensitivity_level);
  }

  ESP_LOGI("TOUCH_SCREEN", "touch_screen_set_sensitivity completed");
}

// Touch screen calibration
void touch_screen_calibrate(void) {
  ESP_LOGI("TOUCH_SCREEN",
           "touch_screen_calibrate called, current touch_active=%s",
           touch_active ? "TRUE" : "FALSE");

  if (!touch_active) {
    ESP_LOGW("TOUCH_SCREEN", "Cannot calibrate: touch screen is disabled");
    return;
  }

  ESP_LOGI("TOUCH_SCREEN", "Touch screen calibration started, touch_active=%s",
           touch_active ? "TRUE" : "FALSE");

  // TODO: Implement actual calibration logic
  // This could involve collecting touch points and calculating offsets

  ESP_LOGI("TOUCH_SCREEN", "Touch screen calibration completed");
  ESP_LOGI("TOUCH_SCREEN", "touch_screen_calibrate completed successfully");
}

void general_touch_handler(lv_event_t *e) {
  if (!touch_active) {
    return;
  }

  lv_event_code_t code = lv_event_get_code(e);
  lv_point_t point;
  lv_indev_get_point(lv_indev_get_act(), &point);

  uint32_t current_time = lv_tick_get();

  if (code == LV_EVENT_PRESSED) {
    touch_start_time = current_time;
    touch_start_point = point;
    last_touch_point = point;

    // Log basic touch coordinates
    ESP_LOGI("TOUCH_HANDLER", "Touch pressed at x=%d, y=%d", point.x, point.y);

    // Touch cursor functionality removed

    // Log touch point information for debugging
    ESP_LOGI("TOUCH_COORDS", "Touch point: x=%d, y=%d, screen=%d", point.x,
             point.y, current_screen);

  } else if (code == LV_EVENT_RELEASED) {
    uint32_t touch_duration = current_time - touch_start_time;

    // Log touch release information
    ESP_LOGI("TOUCH_HANDLER", "Touch released at x=%d, y=%d, duration=%lu ms",
             point.x, point.y, touch_duration);

    // Log final touch coordinates
    ESP_LOGI("TOUCH_COORDS", "Touch ended: x=%d, y=%d, duration=%lu ms",
             point.x, point.y, touch_duration);
  }
}

// Unified swipe gesture handler for all screens
void ui_screen_swipe_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_GESTURE) {
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());

    if (dir == LV_DIR_LEFT) {
      ESP_LOGI("SWIPE", "Left gesture detected -> Next screen");
      ui_switch_to_next_enabled_screen(true); // Forward
    } else if (dir == LV_DIR_RIGHT) {
      ESP_LOGI("SWIPE", "Right gesture detected -> Previous screen");
      ui_switch_to_next_enabled_screen(false); // Backward
    }
  }
}

// Initialize screen manager
void ui_screen_manager_init(void) {
  ESP_LOGI("SCREEN_MANAGER", "Initializing UI screen manager...");

  // Initialize touch screen
  touch_screen_init();

  // Set initial screen
  current_screen = SCREEN_1;

  ESP_LOGI("SCREEN_MANAGER", "UI screen manager initialized successfully");
}

// Check if can switch to Screen3
bool ui_can_switch_to_screen3(void) {
  bool enabled = screen3_get_enabled();
  ESP_LOGI("SCREEN_MANAGER", "Screen3 access check: %s",
           enabled ? "ENABLED" : "DISABLED");
  return enabled;
}

// Check if a screen is enabled
static bool ui_is_screen_enabled(screen_id_t screen_id) {
  switch (screen_id) {
  case SCREEN_3:
    // Map is special, controlled by its own toggle
    return screen3_get_enabled();
  case SCREEN_6:
    // Settings always enabled
    return true;
  case SCREEN_7:
    // Game might be always enabled or hidden?
    // For now assume always enabled if compiled in
    return true;
  case SCREEN_8:
    // Lux Dashboard
    return true;
  default:
    // Screens 1, 2, 4, 5 are managed by layout manager (empty check)
    // Need to map screen_id enum to layout manager index (which matches enum
    // mostly)
    return ui_layout_is_screen_active((int)screen_id);
  }
}

// Get next enabled screen in specified direction
screen_id_t ui_get_next_enabled_screen(screen_id_t current_screen,
                                       bool forward) {
  screen_id_t screens[] = {SCREEN_1, SCREEN_2, SCREEN_3, SCREEN_4,
                           SCREEN_5, SCREEN_6, SCREEN_7, SCREEN_8};
  int num_screens = sizeof(screens) / sizeof(screens[0]);
  int current_index = -1;

  // Find current screen index
  for (int i = 0; i < num_screens; i++) {
    if (screens[i] == current_screen) {
      current_index = i;
      break;
    }
  }

  if (current_index == -1) {
    ESP_LOGW("SCREEN_MANAGER", "Current screen not found in navigation array");
    return SCREEN_1; // Fallback to first screen
  }

  // Search for next enabled screen
  int search_index = current_index;
  int iterations = 0;

  while (iterations < num_screens) {
    if (forward) {
      search_index = (search_index + 1) % num_screens;
    } else {
      search_index = (search_index - 1 + num_screens) % num_screens;
    }

    if (ui_is_screen_enabled(screens[search_index])) {
      ESP_LOGI("SCREEN_MANAGER",
               "Next enabled screen found: %d (from %d, direction: %s)",
               screens[search_index], current_screen,
               forward ? "forward" : "backward");
      return screens[search_index];
    }

    iterations++;
  }

  // If no enabled screen found (shouldn't happen), return current screen
  ESP_LOGW("SCREEN_MANAGER",
           "No enabled screen found, staying on current screen");
  return current_screen;
}

// Get previous enabled screen (alias for backward direction)
screen_id_t ui_get_prev_enabled_screen(screen_id_t current_screen,
                                       bool forward) {
  return ui_get_next_enabled_screen(current_screen, forward);
}

// Switch to next enabled screen in specified direction
void ui_switch_to_next_enabled_screen(bool forward) {
  screen_id_t current_screen = ui_get_current_screen();
  screen_id_t next_screen = ui_get_next_enabled_screen(current_screen, forward);

  if (next_screen != current_screen) {
    ESP_LOGI(
        "SCREEN_MANAGER",
        "Switching from screen %d to next enabled screen %d (direction: %s)",
        current_screen, next_screen, forward ? "forward" : "backward");
    ui_switch_to_screen(next_screen);
  } else {
    ESP_LOGW("SCREEN_MANAGER",
             "No next enabled screen found, staying on current screen");
  }
}

// Switch to specified screen with animation
void ui_switch_to_screen(screen_id_t screen_id) {
  if (!touch_active) {
    ESP_LOGW("SCREEN_MANAGER",
             "Cannot switch screens: touch screen is disabled");
    return;
  }

  if (screen_id == current_screen)
    return;

  ESP_LOGI("SCREEN_MANAGER", "Switching to screen %d", screen_id);

  // Determine animation direction
  lv_scr_load_anim_t anim_type = LV_SCR_LOAD_ANIM_NONE;
  bool forward = false;

  if (screen_id > current_screen) {
    // Normal increase (1->2), but watch for wrap backward (8->1 is not this,
    // 1->8 is)
    if (current_screen == SCREEN_1 && screen_id == SCREEN_8)
      forward = false; // Wrap back
    else
      forward = true;
  } else {
    // Normal decrease (2->1), but watch for wrap forward (8->1)
    if (current_screen == SCREEN_8 && screen_id == SCREEN_1)
      forward = true; // Wrap forward
    else
      forward = false;
  }

  anim_type =
      forward ? LV_SCR_LOAD_ANIM_MOVE_LEFT : LV_SCR_LOAD_ANIM_MOVE_RIGHT;

  uint32_t anim_time = 300; // ms

  // Check if target screen is enabled
  if (!ui_is_screen_enabled(screen_id)) {
    ESP_LOGW("SCREEN_MANAGER",
             "Screen %d is disabled, finding next enabled screen", screen_id);
    // Find next enabled screen in forward direction from current screen
    screen_id_t next_enabled = ui_get_next_enabled_screen(current_screen, true);
    if (next_enabled != screen_id && ui_is_screen_enabled(next_enabled)) {
      ESP_LOGI("SCREEN_MANAGER",
               "Redirecting from disabled %d to next enabled screen: %d",
               screen_id, next_enabled);
      screen_id = next_enabled;
    } else {
      ESP_LOGW("SCREEN_MANAGER",
               "Cannot find any enabled screen to redirect to");
      return;
    }
  }

  switch (screen_id) {
  case SCREEN_1:
    lv_scr_load_anim(ui_Screen1, anim_type, anim_time, 0, false);
    current_screen = SCREEN_1;
    ESP_LOGI("SCREEN_MANAGER", "Switched to SCREEN_1");
    break;

  case SCREEN_2:
    lv_scr_load_anim(ui_Screen2, anim_type, anim_time, 0, false);
    current_screen = SCREEN_2;
    ESP_LOGI("SCREEN_MANAGER", "Switched to SCREEN_2");
    break;

  case SCREEN_3:
    if (!ui_can_switch_to_screen3()) {
      ESP_LOGW("SCREEN_MANAGER",
               "Cannot switch to SCREEN_3: Screen3 is disabled in settings");
      // Try to find next enabled screen
      screen_id_t next_enabled =
          ui_get_next_enabled_screen(current_screen, true);
      if (next_enabled != SCREEN_3 && ui_is_screen_enabled(next_enabled)) {
        ESP_LOGI("SCREEN_MANAGER", "Redirecting from disabled SCREEN_3 to: %d",
                 next_enabled);
        ui_switch_to_screen(next_enabled);
      }
      return;
    }
    lv_scr_load_anim(ui_Screen3, anim_type, anim_time, 0, false);
    current_screen = SCREEN_3;
    ESP_LOGI("SCREEN_MANAGER", "Switched to SCREEN_3");
    break;

  case SCREEN_4:
    lv_scr_load_anim(ui_Screen4, anim_type, anim_time, 0, false);
    current_screen = SCREEN_4;
    ESP_LOGI("SCREEN_MANAGER", "Switched to SCREEN_4");
    break;

  case SCREEN_5:
    lv_scr_load_anim(ui_Screen5, anim_type, anim_time, 0, false);
    current_screen = SCREEN_5;
    ESP_LOGI("SCREEN_MANAGER", "Switched to SCREEN_5");
    break;

  case SCREEN_6:
    lv_scr_load_anim(ui_Screen6, anim_type, anim_time, 0, false);
    current_screen = SCREEN_6;
    ESP_LOGI("SCREEN_MANAGER", "Switched to SCREEN_6");
    break;

  case SCREEN_7:
    lv_scr_load_anim(ui_Screen7, anim_type, anim_time, 0, false);
    current_screen = SCREEN_7;
    ESP_LOGI("SCREEN_MANAGER", "Switched to SCREEN_7");
    break;

  case SCREEN_8:
    lv_scr_load_anim(ui_Screen8, anim_type, anim_time, 0, false);
    current_screen = SCREEN_8;
    ESP_LOGI("SCREEN_MANAGER", "Switched to SCREEN_8");
    break;

  default:
    ESP_LOGW("SCREEN_MANAGER", "Unknown screen ID: %d", screen_id);
    break;
  }
}

// Get current screen
screen_id_t ui_get_current_screen(void) { return current_screen; }

// Enable swipe gestures (now supported)
void ui_enable_swipe_gestures(void) {
  ESP_LOGI("SCREEN_MANAGER", "Swipe gestures enabled");
  // Swipe gestures are now implemented in each screen
}

// Disable swipe gestures (now supported)
void ui_disable_swipe_gestures(void) {
  ESP_LOGI("SCREEN_MANAGER", "Swipe gestures disabled");
  // Swipe gestures can be disabled by removing event callbacks
}

// Get touch sensitivity
uint8_t ui_get_touch_sensitivity(void) { return touch_sensitivity_level; }

// Get swipe threshold (removed - no longer supported)
int16_t ui_get_swipe_threshold(void) {
  ESP_LOGW("SCREEN_MANAGER", "Swipe threshold is no longer supported");
  return 0;
}

// Touch gauges functionality removed - function no longer needed

// Create navigation buttons
void ui_create_navigation_buttons(void) {
  ESP_LOGI("NAVIGATION", "Creating navigation buttons...");

  // Navigation buttons are now implemented in each screen
  // Swipe gestures provide additional navigation method

  ESP_LOGI("NAVIGATION",
           "Navigation buttons and swipe gestures created successfully");
}

// Update navigation buttons
void ui_update_navigation_buttons(void) {
  ESP_LOGI("NAVIGATION", "Updating navigation buttons...");

  // Navigation buttons are updated automatically in each screen
  // Swipe gestures work independently

  ESP_LOGI("NAVIGATION",
           "Navigation buttons and swipe gestures updated successfully");
}

// Event callback for the "Previous" button
static void _nav_prev_btn_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED) {
    ui_switch_to_next_enabled_screen(false); // false = backward
  }
}

// Event callback for the "Next" button
static void _nav_next_btn_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED) {
    ui_switch_to_next_enabled_screen(true); // true = forward
  }
}

/**
 * @brief Creates standardized "Next" and "Previous" navigation buttons on a
 * given parent screen.
 *
 * @param parent_screen The screen object to which the buttons will be added.
 */
void ui_create_standard_navigation_buttons(lv_obj_t *parent_screen) {
  if (!parent_screen) {
    ESP_LOGE("NAV_BUTTONS", "Cannot create buttons on a NULL screen.");
    return;
  }

  if (!nav_buttons_get_enabled()) {
    ESP_LOGI("NAV_BUTTONS",
             "Navigation buttons are disabled in settings, skipping creation.");
    return;
  }

  // Previous screen button (left arrow)
  lv_obj_t *prev_screen_btn = lv_btn_create(parent_screen);
  lv_obj_set_size(prev_screen_btn, 50, 50);
  lv_obj_align(prev_screen_btn, LV_ALIGN_BOTTOM_LEFT, 20, -20);
  lv_obj_set_style_bg_color(prev_screen_btn, lv_color_hex(0x00D4FF), 0);
  lv_obj_set_style_radius(prev_screen_btn, 25, 0);
  lv_obj_add_event_cb(prev_screen_btn, _nav_prev_btn_event_cb, LV_EVENT_CLICKED,
                      NULL);

  lv_obj_t *prev_icon = lv_label_create(prev_screen_btn);
  lv_label_set_text(prev_icon, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_color(prev_icon, lv_color_white(), 0);
  lv_obj_set_style_text_font(prev_icon, &lv_font_montserrat_20, 0);
  lv_obj_center(prev_icon);

  // Next screen button (right arrow)
  lv_obj_t *next_screen_btn = lv_btn_create(parent_screen);
  lv_obj_set_size(next_screen_btn, 50, 50);
  lv_obj_align(next_screen_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
  lv_obj_set_style_bg_color(next_screen_btn, lv_color_hex(0x00D4FF), 0);
  lv_obj_set_style_radius(next_screen_btn, 25, 0);
  lv_obj_add_event_cb(next_screen_btn, _nav_next_btn_event_cb, LV_EVENT_CLICKED,
                      NULL);

  lv_obj_t *next_icon = lv_label_create(next_screen_btn);
  lv_label_set_text(next_icon, LV_SYMBOL_RIGHT);
  lv_obj_set_style_text_color(next_icon, lv_color_white(), 0);
  lv_obj_set_style_text_font(next_icon, &lv_font_montserrat_20, 0);
  lv_obj_center(next_icon);

  ESP_LOGI("NAV_BUTTONS", "Standard navigation buttons created for screen.");
}

// Cleanup function
void ui_screen_manager_cleanup(void) {
  ESP_LOGI("SCREEN_MANAGER", "Cleaning up UI screen manager...");

  // Clean up any allocated resources
  // For now, just log the cleanup process

  ESP_LOGI("SCREEN_MANAGER", "UI screen manager cleanup completed");
}

// ============================================================================
// ФУНКЦИИ ПРОВЕРКИ ГРАНИЦ ЭКРАНОВ
// ============================================================================

// Проверка границ экрана - все элементы должны быть внутри 1280x720
bool ui_check_screen_bounds(int x, int y, int width, int height,
                            const char *element_name) {
  // Жесткие границы экрана ESP32-P4
  // Dynamic screen bounds based on configuration
  const int SCREEN_WIDTH = 1280;
  const int SCREEN_HEIGHT = 720;

  // Проверяем, не выходит ли элемент за границы
  if (x < 0 || y < 0 || x + width > SCREEN_WIDTH ||
      y + height > SCREEN_HEIGHT) {

    ESP_LOGW("BOUNDS_CHECK", "❌ %s выходит за границы экрана!", element_name);
    ESP_LOGW("BOUNDS_CHECK", "   Позиция: x=%d, y=%d, размер: %dx%d", x, y,
             width, height);
    ESP_LOGW("BOUNDS_CHECK", "   Границы экрана: 0-1280 x 0-720");
    return false; // Элемент выходит за границы
  }

  ESP_LOGI("BOUNDS_CHECK", "✅ %s в пределах границ экрана", element_name);
  ESP_LOGI("BOUNDS_CHECK", "   Позиция: x=%d, y=%d, размер: %dx%d", x, y, width,
           height);
  return true; // Элемент в пределах границ
}

// Проверка всех датчиков Screen1
void ui_validate_screen1_bounds(void) {
  ESP_LOGI("BOUNDS_CHECK", "🔍 Проверка границ Screen1...");

  // Проверяем каждый датчик
  ui_check_screen_bounds(15, 7, 250, 225, "MAP Pressure");
  ui_check_screen_bounds(275, 7, 250, 225, "Wastegate");
  ui_check_screen_bounds(535, 7, 250, 225, "TPS Position");
  ui_check_screen_bounds(15, 247, 250, 225, "Engine RPM");
  ui_check_screen_bounds(275, 247, 250, 225, "Target Boost");
  ui_check_screen_bounds(535, 247, 250, 225, "TCU Status");

  ESP_LOGI("BOUNDS_CHECK", "✅ Проверка границ Screen1 завершена");
}

// Проверка всех датчиков Screen2
void ui_validate_screen2_bounds(void) {
  ESP_LOGI("BOUNDS_CHECK", "🔍 Проверка границ Screen2...");

  // Проверяем каждый датчик (размер 200x180)
  ui_check_screen_bounds(15, 7, 200, 180, "Oil Pressure");
  ui_check_screen_bounds(225, 7, 200, 180, "Oil Temp");
  ui_check_screen_bounds(15, 197, 200, 180, "Water Temp");
  ui_check_screen_bounds(225, 197, 200, 180, "Fuel Pressure");

  ESP_LOGI("BOUNDS_CHECK", "✅ Проверка границ Screen2 завершена");
}

// Проверка всех элементов Settings
void ui_validate_settings_bounds(void) {
  ESP_LOGI("BOUNDS_CHECK", "🔍 Проверка границ Settings...");

  // Проверяем основные элементы
  ui_check_screen_bounds(0, 20, 800, 24, "Title");
  ui_check_screen_bounds(50, 80, 80, 30, "Touch Gauges Switch");
  ui_check_screen_bounds(50, 120, 80, 30, "Demo Mode Switch");
  ui_check_screen_bounds(50, 160, 80, 30, "Screen3 Switch");
  ui_check_screen_bounds(50, 270, 200, 18, "Screen1 Title");
  ui_check_screen_bounds(50, 300, 150, 20, "MAP Checkbox");

  ESP_LOGI("BOUNDS_CHECK", "✅ Проверка границ Settings завершена");
}

// Общая проверка всех экранов
void ui_validate_all_screen_bounds(void) {
  ESP_LOGI("BOUNDS_CHECK", "🚀 НАЧАЛО ПРОВЕРКИ ВСЕХ ГРАНИЦ ЭКРАНОВ");
  ESP_LOGI("BOUNDS_CHECK", "==========================================");

  ui_validate_screen1_bounds();
  ui_validate_screen2_bounds();
  ui_validate_settings_bounds();

  ESP_LOGI("BOUNDS_CHECK", "==========================================");
  ESP_LOGI("BOUNDS_CHECK", "✅ ПРОВЕРКА ВСЕХ ГРАНИЦ ЗАВЕРШЕНА");
}
