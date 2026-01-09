
// ECU Dashboard Screen with 6 animated gauges
// Based on test project structure

#include "ui_Screen1.h"
#include "../ui.h"
#include "../ui_screen_manager.h"
#include "ecu_data.h" // Include settings
#include "esp_log.h"
#include "ui_Screen3.h"
#include <stdio.h>

lv_obj_t *ui_Screen1 = NULL;
lv_obj_t *ui_Arc_MAP = NULL;
lv_obj_t *ui_Arc_Wastegate = NULL;
lv_obj_t *ui_Arc_TPS = NULL;
lv_obj_t *ui_Arc_RPM = NULL;
lv_obj_t *ui_Arc_Boost = NULL;
// Intake Air Temp убран, возвращен TCU
lv_obj_t *ui_LED_TCU = NULL;
lv_obj_t *ui_Label_TCU_Status = NULL;
lv_obj_t *ui_Label_Gear_S1 = NULL;
lv_obj_t *ui_Label_Selector_S1 = NULL;

lv_obj_t *ui_Label_MAP_Value = NULL;
lv_obj_t *ui_Label_Wastegate_Value = NULL;
lv_obj_t *ui_Label_TPS_Value = NULL;
lv_obj_t *ui_Label_RPM_Value = NULL;
lv_obj_t *ui_Label_Boost_Value = NULL;
// Intake Air Temp label убран

static lv_anim_t anim_map;
static lv_anim_t anim_wastegate;
static lv_anim_t anim_tps;
static lv_anim_t anim_rpm;
static lv_anim_t anim_boost;

// Forward declarations for touch cursor animation callbacks
// Removed unused function declarations to fix compiler warnings
// static void fade_anim_cb_screen1(void * var, int32_t v);
// static void fade_ready_cb_screen1(lv_anim_t * a);

// Forward declarations for splash screen animation callbacks - REMOVED UNUSED
// FUNCTIONS

static void anim_value_cb(void *var, int32_t v) {
  lv_arc_set_value((lv_obj_t *)var, v);

  char buf[16];
  snprintf(buf, sizeof(buf), "%d", (int)v);

  if (var == ui_Arc_MAP) {
    lv_label_set_text(ui_Label_MAP_Value, buf);
  } else if (var == ui_Arc_Wastegate) {
    lv_label_set_text(ui_Label_Wastegate_Value, buf);
  } else if (var == ui_Arc_TPS) {
    lv_label_set_text(ui_Label_TPS_Value, buf);
  } else if (var == ui_Arc_RPM) {
    lv_label_set_text(ui_Label_RPM_Value, buf);

    // Update Arc RPM color based on RPM range
    if (v >= 0 && v < 5000) {
      // 0-5000: голубой/cyan
      lv_obj_set_style_arc_color(ui_Arc_RPM, lv_color_hex(0x00D4FF),
                                 LV_PART_INDICATOR);
    } else if (v >= 5000 && v < 6500) {
      // 5000-6500: желтый
      lv_obj_set_style_arc_color(ui_Arc_RPM, lv_color_hex(0xFFD700),
                                 LV_PART_INDICATOR);
    } else if (v >= 6500 && v <= 8000) {
      // 6500-8000: красный
      lv_obj_set_style_arc_color(ui_Arc_RPM, lv_color_hex(0xFF0000),
                                 LV_PART_INDICATOR);
    }

    // Update TCU status based on RPM
    if (v > 5500) {
      lv_led_set_color(ui_LED_TCU, lv_color_hex(0xFF0000));
      lv_label_set_text(ui_Label_TCU_Status, "ERROR");
      lv_obj_set_style_text_color(ui_Label_TCU_Status, lv_color_hex(0xFF0000),
                                  0);
    } else if (v > 4500) {
      lv_led_set_color(ui_LED_TCU, lv_color_hex(0xFFAA00));
      lv_label_set_text(ui_Label_TCU_Status, "WARNING");
      lv_obj_set_style_text_color(ui_Label_TCU_Status, lv_color_hex(0xFFAA00),
                                  0);
    } else {
      lv_led_set_color(ui_LED_TCU, lv_color_hex(0x00FF00));
      lv_label_set_text(ui_Label_TCU_Status, "OK");
      lv_obj_set_style_text_color(ui_Label_TCU_Status, lv_color_hex(0x00FF00),
                                  0);
    }
  } else if (var == ui_Arc_Boost) {
    lv_label_set_text(ui_Label_Boost_Value, buf);
  }
  // Intake Air Temp обработка убрана
  // Intake Air Temp обработка убрана
}

// Helper to get container of a gauge
static lv_obj_t *get_gauge_container(lv_obj_t *gauge) __attribute__((unused));
static lv_obj_t *get_gauge_container(lv_obj_t *gauge) {
  if (!gauge)
    return NULL;
  return lv_obj_get_parent(gauge);
}

// Update layout based on visible gauges
void ui_Screen1_update_layout(void) {
  // Legacy function - now handled by ui_layout_manager.c
  // Kept for compatibility with existing calls
  ESP_LOGI("SCREEN1", "Legacy update_layout called - ignored");
}

static void create_gauge(lv_obj_t *parent, lv_obj_t **arc, lv_obj_t **label,
                         const char *title, const char *unit, lv_color_t color,
                         int32_t min_val, int32_t max_val, int x, int y) {
  // Container - возвращаем оригинальный размер 250x225 для лучших пропорций
  lv_obj_t *cont = lv_obj_create(parent);
  lv_obj_set_width(cont, 250);
  lv_obj_set_height(cont, 225);
  lv_obj_set_x(cont, x);
  lv_obj_set_y(cont, y);
  lv_obj_set_align(cont, LV_ALIGN_TOP_LEFT);
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(cont, lv_color_hex(0x2a2a2a), 0);
  lv_obj_set_style_border_color(cont, color, 0);
  lv_obj_set_style_border_width(cont, 2, 0);
  lv_obj_set_style_radius(cont, 15, 0);
  lv_obj_set_style_pad_all(cont, 10, 0);

  // Title
  lv_obj_t *label_title = lv_label_create(cont);
  lv_label_set_text(label_title, title);
  lv_obj_set_style_text_color(label_title, lv_color_white(), 0);
  lv_obj_align(label_title, LV_ALIGN_BOTTOM_MID, 0, -15);

  // Arc - возвращаем оригинальный размер дуги
  *arc = lv_arc_create(cont);
  lv_obj_set_size(*arc, 160, 160);
  lv_arc_set_rotation(*arc, 135);
  lv_arc_set_bg_angles(*arc, 0, 270);
  lv_arc_set_range(*arc, min_val, max_val);
  lv_arc_set_value(*arc, min_val);
  lv_obj_set_style_arc_color(*arc, color, LV_PART_INDICATOR);
  lv_obj_set_style_arc_width(*arc, 15, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(*arc, lv_color_hex(0x4a4a4a), LV_PART_MAIN);
  lv_obj_set_style_arc_width(*arc, 15, LV_PART_MAIN);
  lv_obj_center(*arc);
  lv_obj_remove_style(*arc, NULL, LV_PART_KNOB);
  lv_obj_clear_flag(*arc, LV_OBJ_FLAG_CLICKABLE);

  // Value label
  *label = lv_label_create(cont);
  lv_label_set_text(*label, "0");
  lv_obj_set_style_text_color(*label, lv_color_white(), 0);
  lv_obj_set_style_text_font(*label, &lv_font_montserrat_24, 0);
  lv_obj_center(*label);
  lv_obj_align(*label, LV_ALIGN_CENTER, 0, -5);

  // Unit label
  lv_obj_t *label_unit = lv_label_create(cont);
  lv_label_set_text(label_unit, unit);
  lv_obj_set_style_text_color(label_unit, lv_color_hex(0xcccccc), 0);
  lv_obj_align_to(label_unit, *label, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
}

void ui_Screen1_screen_init(void) {
  ui_Screen1 = lv_obj_create(NULL);

  // ЖЕСТКО ФИКСИРУЕМ РАЗМЕР ЭКРАНА - 1280x720 (Landscape)
  lv_obj_set_size(ui_Screen1, 1280, 720);
  lv_obj_set_pos(ui_Screen1, 0, 0);

  lv_obj_clear_flag(ui_Screen1, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(ui_Screen1, lv_color_hex(0x1a1a1a), 0);

  // Проверяем границы экрана - все элементы должны быть внутри 1280x720
  ESP_LOGI("SCREEN1", "Screen1 initialized with HARD FIXED size: 1280x720");
  ESP_LOGI("SCREEN1", "All gauges will be constrained within these boundaries");

  // Create gauges in 3x2 grid (Landscape mode 1280x720)
  // Row 1 (y=40)
  create_gauge(ui_Screen1, &ui_Arc_MAP, &ui_Label_MAP_Value, "MAP Pressure",
               "kPa", lv_color_hex(0x00D4FF), 100, 250, 60, 40);
  create_gauge(ui_Screen1, &ui_Arc_Wastegate, &ui_Label_Wastegate_Value,
               "Wastegate", "%", lv_color_hex(0x00D4FF), 0, 100, 480, 40);
  create_gauge(ui_Screen1, &ui_Arc_TPS, &ui_Label_TPS_Value, "TPS Position",
               "%", lv_color_hex(0x00D4FF), 0, 100, 900, 40);

  // Row 2 (y=380)
  create_gauge(ui_Screen1, &ui_Arc_RPM, &ui_Label_RPM_Value, "Engine RPM",
               "RPM", lv_color_hex(0x00D4FF), 0, 8000, 60, 380);
  create_gauge(ui_Screen1, &ui_Arc_Boost, &ui_Label_Boost_Value, "Target Boost",
               "kPa", lv_color_hex(0x00D4FF), 100, 250, 480, 380);
  // Датчик 5: x=285 to 535, расстояние от датчика 4: 285-265=20px

  // Датчик 6: x=545 to 795, расстояние от датчика 5: 545-535=10px, 5px от
  // границы

  // TCU Status indicator (Row 3, Col 2)
  lv_obj_t *tcu_cont = lv_obj_create(ui_Screen1);
  lv_obj_set_width(tcu_cont, 250);
  lv_obj_set_height(tcu_cont, 225);
  lv_obj_set_x(tcu_cont, 425);
  lv_obj_set_y(tcu_cont, 495);
  lv_obj_set_align(tcu_cont, LV_ALIGN_TOP_LEFT);
  lv_obj_clear_flag(tcu_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(tcu_cont, lv_color_hex(0x2a2a2a), 0);
  lv_obj_set_style_border_color(tcu_cont, lv_color_hex(0x00D4FF), 0);
  lv_obj_set_style_border_width(tcu_cont, 2, 0);
  lv_obj_set_style_radius(tcu_cont, 15, 0);
  lv_obj_set_style_pad_all(tcu_cont, 10, 0);

  lv_obj_t *tcu_title = lv_label_create(tcu_cont);
  lv_label_set_text(tcu_title, "TCU Status");
  lv_obj_set_style_text_color(tcu_title, lv_color_white(), 0);
  lv_obj_align(tcu_title, LV_ALIGN_BOTTOM_MID, 0, -15);

  ui_LED_TCU = lv_led_create(tcu_cont);
  lv_obj_set_size(ui_LED_TCU, 30, 30); // Reduced size
  lv_obj_align(ui_LED_TCU, LV_ALIGN_TOP_RIGHT, -10, 10);
  lv_led_set_color(ui_LED_TCU, lv_color_hex(0x00FF00));
  lv_led_on(ui_LED_TCU);

  ui_Label_TCU_Status = lv_label_create(tcu_cont);
  lv_label_set_text(ui_Label_TCU_Status, "OK");
  lv_obj_set_style_text_color(ui_Label_TCU_Status, lv_color_hex(0x00FF00), 0);
  lv_obj_align(ui_Label_TCU_Status, LV_ALIGN_TOP_RIGHT, -50, 15);

  // New Labels for Gear and Selector
  ui_Label_Gear_S1 = lv_label_create(tcu_cont);
  lv_label_set_text(ui_Label_Gear_S1, "Gear: -");
  lv_obj_set_style_text_color(ui_Label_Gear_S1, lv_color_white(), 0);
  lv_obj_set_style_text_font(ui_Label_Gear_S1, &lv_font_montserrat_24, 0);
  lv_obj_align(ui_Label_Gear_S1, LV_ALIGN_LEFT_MID, 10, -20);

  ui_Label_Selector_S1 = lv_label_create(tcu_cont);
  lv_label_set_text(ui_Label_Selector_S1, "Sel: -");
  lv_obj_set_style_text_color(ui_Label_Selector_S1, lv_color_white(), 0);
  lv_obj_set_style_text_font(ui_Label_Selector_S1, &lv_font_montserrat_24, 0);
  lv_obj_align(ui_Label_Selector_S1, LV_ALIGN_LEFT_MID, 10, 20);

  // Setup animations
  lv_anim_init(&anim_map);
  lv_anim_set_var(&anim_map, ui_Arc_MAP);
  lv_anim_set_values(&anim_map, 100, 250);
  lv_anim_set_time(&anim_map, 3000);
  lv_anim_set_playback_time(&anim_map, 3000);
  lv_anim_set_repeat_count(&anim_map, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_exec_cb(&anim_map, anim_value_cb);

  // Start animation only if demo mode is enabled
  if (demo_mode_get_enabled()) {
    lv_anim_start(&anim_map);
  }

  lv_anim_init(&anim_wastegate);
  lv_anim_set_var(&anim_wastegate, ui_Arc_Wastegate);
  lv_anim_set_values(&anim_wastegate, 0, 100);
  lv_anim_set_time(&anim_wastegate, 2500);
  lv_anim_set_playback_time(&anim_wastegate, 2500);
  lv_anim_set_repeat_count(&anim_wastegate, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_exec_cb(&anim_wastegate, anim_value_cb);

  // Start animation only if demo mode is enabled
  if (demo_mode_get_enabled()) {
    lv_anim_start(&anim_wastegate);
  }

  lv_anim_init(&anim_tps);
  lv_anim_set_var(&anim_tps, ui_Arc_TPS);
  lv_anim_set_values(&anim_tps, 0, 100);
  lv_anim_set_time(&anim_tps, 2000);
  lv_anim_set_playback_time(&anim_tps, 2000);
  lv_anim_set_repeat_count(&anim_tps, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_exec_cb(&anim_tps, anim_value_cb);

  // Start animation only if demo mode is enabled
  if (demo_mode_get_enabled()) {
    lv_anim_start(&anim_tps);
  }

  lv_anim_init(&anim_rpm);
  lv_anim_set_var(&anim_rpm, ui_Arc_RPM);
  lv_anim_set_values(&anim_rpm, 0, 8000);
  lv_anim_set_time(&anim_rpm, 4000);
  lv_anim_set_playback_time(&anim_rpm, 4000);
  lv_anim_set_repeat_count(&anim_rpm, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_exec_cb(&anim_rpm, anim_value_cb);

  // Start animation only if demo mode is enabled
  if (demo_mode_get_enabled()) {
    lv_anim_start(&anim_rpm);
  }

  lv_anim_init(&anim_boost);
  lv_anim_set_var(&anim_boost, ui_Arc_Boost);
  lv_anim_set_values(&anim_boost, 100, 250);
  lv_anim_set_time(&anim_boost, 3500);
  lv_anim_set_playback_time(&anim_boost, 3500);
  lv_anim_set_repeat_count(&anim_boost, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_exec_cb(&anim_boost, anim_value_cb);

  // Start animation only if demo mode is enabled
  if (demo_mode_get_enabled()) {
    lv_anim_start(&anim_boost);
  }

  // Touch gauges functionality removed - no longer needed

  // Apply initial layout
  ui_Screen1_update_layout();

  ESP_LOGI("SCREEN1", "Screen 1 initialized with basic touch functionality, "
                      "swipe gestures, and navigation buttons");
}

// Splash screen animation callbacks - REMOVED UNUSED FUNCTIONS

// Function to control animations based on demo mode
void ui_Screen1_update_animations(bool demo_enabled) {
  // Safety check - ensure UI objects are initialized
  if (!ui_Arc_MAP || !ui_Arc_Wastegate || !ui_Arc_TPS || !ui_Arc_RPM ||
      !ui_Arc_Boost) {
    ESP_LOGW("SCREEN1",
             "UI objects not initialized yet, skipping animation update");
    return;
  }

  if (demo_enabled) {
    // Start all animations
    lv_anim_start(&anim_map);
    lv_anim_start(&anim_wastegate);
    lv_anim_start(&anim_tps);
    lv_anim_start(&anim_rpm);
    lv_anim_start(&anim_boost);
    ESP_LOGI("SCREEN1", "Started all animations (demo mode enabled)");
  } else {
    // Stop all animations and reset to minimum values
    lv_anim_del(ui_Arc_MAP, anim_value_cb);
    lv_anim_del(ui_Arc_Wastegate, anim_value_cb);
    lv_anim_del(ui_Arc_TPS, anim_value_cb);
    lv_anim_del(ui_Arc_RPM, anim_value_cb);
    lv_anim_del(ui_Arc_Boost, anim_value_cb);

    // Reset to minimum values
    lv_arc_set_value(ui_Arc_MAP, 100);
    lv_arc_set_value(ui_Arc_Wastegate, 0);
    lv_arc_set_value(ui_Arc_TPS, 0);
    lv_arc_set_value(ui_Arc_RPM, 0);
    lv_arc_set_value(ui_Arc_Boost, 100);

    // Update labels
    lv_label_set_text_fmt(ui_Label_MAP_Value, "%d", 100);
    lv_label_set_text_fmt(ui_Label_Wastegate_Value, "%d", 0);
    lv_label_set_text_fmt(ui_Label_TPS_Value, "%d", 0);
    lv_label_set_text_fmt(ui_Label_RPM_Value, "%d", 0);
    lv_label_set_text_fmt(ui_Label_Boost_Value, "%d", 100);

    ESP_LOGI("SCREEN1", "Stopped all animations and reset to minimum values "
                        "(demo mode disabled)");
  }
}

// Function to control individual arc visibility
void ui_Screen1_update_arc_visibility(int arc_index, bool visible) {
  lv_obj_t *arc_container = NULL;
  const char *arc_name = NULL;

  // Map arc index to container and name
  switch (arc_index) {
  case 0: // MAP
    arc_container = lv_obj_get_parent(ui_Arc_MAP);
    arc_name = "MAP Pressure";
    break;
  case 1: // Wastegate
    arc_container = lv_obj_get_parent(ui_Arc_Wastegate);
    arc_name = "Wastegate";
    break;
  case 2: // TPS
    arc_container = lv_obj_get_parent(ui_Arc_TPS);
    arc_name = "TPS Position";
    break;
  case 3: // RPM
    arc_container = lv_obj_get_parent(ui_Arc_RPM);
    arc_name = "Engine RPM";
    break;
  case 4: // Boost
    arc_container = lv_obj_get_parent(ui_Arc_Boost);
    arc_name = "Target Boost";
    break;
  // case 5 Intake Air Temp убран
  case 5: // TCU Status
    arc_container = lv_obj_get_parent(ui_LED_TCU);
    arc_name = "TCU Status";
    break;
  default:
    ESP_LOGW("SCREEN1", "Invalid arc index: %d", arc_index);
    return;
  }

  if (!arc_container) {
    ESP_LOGW("SCREEN1", "Arc container not found for index %d", arc_index);
    return;
  }

  if (visible) {
    lv_obj_set_style_opa(arc_container, LV_OPA_COVER, 0);
    ESP_LOGI("SCREEN1", "%s gauge is now VISIBLE", arc_name);

    // 🔍 ТРАБЛШУТИНГ: Логируем состояние видимости
    ESP_LOGI("TROUBLESHOOTING", "👁️ SCREEN1: %s стал ВИДИМЫМ", arc_name);
    ESP_LOGI("TROUBLESHOOTING", "   Прозрачность: LV_OPA_COVER (255)");
    ESP_LOGI("TROUBLESHOOTING", "   Контейнер: %p", arc_container);
  } else {
    lv_obj_set_style_opa(arc_container, LV_OPA_TRANSP, 0);
    ESP_LOGI("SCREEN1", "%s gauge is now HIDDEN", arc_name);

    // 🔍 ТРАБЛШУТИНГ: Логируем состояние видимости
    ESP_LOGI("TROUBLESHOOTING", "🙈 SCREEN1: %s стал НЕВИДИМЫМ", arc_name);
    ESP_LOGI("TROUBLESHOOTING", "   Прозрачность: LV_OPA_TRANSP (0)");
    ESP_LOGI("TROUBLESHOOTING", "   Контейнер: %p", arc_container);
  }
}

void ui_Screen1_screen_destroy(void) {
  if (ui_Screen1)
    lv_obj_del(ui_Screen1);
  ui_Screen1 = NULL;
}
