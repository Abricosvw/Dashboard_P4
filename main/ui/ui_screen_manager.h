// UI Screen Manager - Handles switching between screens
// Supports basic touch functionality, swipe gestures, and button navigation

#ifndef UI_SCREEN_MANAGER_H
#define UI_SCREEN_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

// Screen IDs
typedef enum {
    SCREEN_1 = 0,      // Main ECU Dashboard
    SCREEN_2 = 1,      // Additional Gauges
    SCREEN_3 = 2,      // CAN Bus Terminal
    SCREEN_4 = 3,      // ECU Data Page 1
    SCREEN_5 = 4,      // ECU Data Page 2
    SCREEN_6 = 5,      // Device Parameters Settings
    SCREEN_7 = 6,      // New Screen 7
    SCREEN_8 = 7       // Luxury Sport Dashboard (New)
} screen_id_t;

// Screen management functions
void ui_screen_manager_init(void);
void ui_switch_to_screen(screen_id_t screen_id);
screen_id_t ui_get_current_screen(void);
bool ui_can_switch_to_screen3(void);
screen_id_t ui_get_next_enabled_screen(screen_id_t current_screen, bool forward);
screen_id_t ui_get_prev_enabled_screen(screen_id_t current_screen, bool forward);
void ui_switch_to_next_enabled_screen(bool forward);

// Touch screen functions
void touch_screen_init(void);
void touch_screen_enable(void);
void touch_screen_disable(void);
bool touch_screen_is_enabled(void);
void touch_screen_set_sensitivity(uint8_t sensitivity);
void touch_screen_calibrate(void);
// Get swipe threshold (removed - no longer supported)
int16_t ui_get_swipe_threshold(void);

// Add touch functionality to gauge
void add_touch_to_gauge(lv_obj_t * gauge, const char * gauge_name);

// Touch event handlers
void general_touch_handler(lv_event_t * e);
void ui_screen_swipe_event_cb(lv_event_t * e);

// Touch sensitivity functions
uint8_t ui_get_touch_sensitivity(void);

// Swipe gesture functions
void ui_enable_swipe_gestures(void);
void ui_disable_swipe_gestures(void);

// Navigation buttons
void ui_create_navigation_buttons(void); // Legacy, to be removed
void ui_update_navigation_buttons(void); // Legacy, to be removed
void ui_create_standard_navigation_buttons(lv_obj_t * parent_screen);

// Cleanup function
void ui_screen_manager_cleanup(void);

// ============================================================================
// ФУНКЦИИ ПРОВЕРКИ ГРАНИЦ ЭКРАНОВ
// ============================================================================

// Проверка границ экрана - все элементы должны быть внутри 800x480
bool ui_check_screen_bounds(int x, int y, int width, int height, const char* element_name);

// Проверка всех датчиков Screen1
void ui_validate_screen1_bounds(void);

// Проверка всех датчиков Screen2
void ui_validate_screen2_bounds(void);

// Общая проверка всех экранов
void ui_validate_all_screen_bounds(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
