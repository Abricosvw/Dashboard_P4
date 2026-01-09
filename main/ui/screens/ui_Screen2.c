// ECU Dashboard Screen 2 - Additional Gauges
// Second screen accessible by swiping left/right

#include "ui_Screen2.h"
#include "../ui.h"
#include "../ui_screen_manager.h"
#include "ecu_data.h"
#include "esp_log.h"
#include "ui_Screen3.h"
#include <stdio.h>

// Forward declarations for touch cursor animation callbacks
// Removed unused function declarations to fix compiler warnings
// static void fade_anim_cb_screen2(void * var, int32_t v);
// static void fade_ready_cb_screen2(lv_anim_t * a);

lv_obj_t *ui_Screen2 = NULL;

// Additional ECU Gauge Objects (5 датчиков)
lv_obj_t *ui_Arc_Oil_Pressure = NULL;
lv_obj_t *ui_Arc_Oil_Temp = NULL;
lv_obj_t *ui_Arc_Water_Temp = NULL;
lv_obj_t *ui_Arc_Fuel_Pressure = NULL;
lv_obj_t *ui_Arc_Battery_Voltage = NULL; // Вернули Battery датчик

// Additional Label Objects
lv_obj_t *ui_Label_Oil_Pressure_Value = NULL;
lv_obj_t *ui_Label_Oil_Temp_Value = NULL;
lv_obj_t *ui_Label_Water_Temp_Value = NULL;
lv_obj_t *ui_Label_Fuel_Pressure_Value = NULL;
lv_obj_t *ui_Label_Battery_Voltage_Value = NULL; // Вернули Battery label

// Animation objects
static lv_anim_t anim_oil_pressure;
static lv_anim_t anim_oil_temp;
static lv_anim_t anim_water_temp;
static lv_anim_t anim_fuel_pressure;
static lv_anim_t anim_battery_voltage; // Вернули Battery анимацию

static void anim_value_cb(void *var, int32_t v) {
  lv_arc_set_value((lv_obj_t *)var, v);

  if (var == ui_Arc_Oil_Pressure) {
    lv_label_set_text_fmt(ui_Label_Oil_Pressure_Value, "%ld", v);
  } else if (var == ui_Arc_Oil_Temp) {
    lv_label_set_text_fmt(ui_Label_Oil_Temp_Value, "%ld°C", v);
  } else if (var == ui_Arc_Water_Temp) {
    lv_label_set_text_fmt(ui_Label_Water_Temp_Value, "%ld°C", v);
  } else if (var == ui_Arc_Fuel_Pressure) {
    lv_label_set_text_fmt(ui_Label_Fuel_Pressure_Value, "%ld", v);
  } else if (var == ui_Arc_Battery_Voltage) {
    lv_label_set_text_fmt(ui_Label_Battery_Voltage_Value, "%.1f", v / 10.0f);
  }
  // Coolant Temp убран, Battery Voltage возвращен
  // Coolant Temp убран, Battery Voltage возвращен
}

// Helper to get container of a gauge
static lv_obj_t *get_gauge_container(lv_obj_t *gauge) __attribute__((unused));
static lv_obj_t *get_gauge_container(lv_obj_t *gauge) {
  if (!gauge)
    return NULL;
  return lv_obj_get_parent(gauge);
}

// Update layout based on visible gauges
void ui_Screen2_update_layout(void) {
  // Legacy function - now handled by ui_layout_manager.c
  ESP_LOGI("SCREEN2", "Legacy update_layout called - ignored");
}

static void create_gauge(lv_obj_t *parent, lv_obj_t **arc, lv_obj_t **label,
                         const char *title, const char *unit, lv_color_t color,
                         int32_t min_val, int32_t max_val, int x, int y) {
  // Container - такой же размер как на Screen1
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

  // Arc - такой же размер как на Screen1
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
  lv_obj_center(*label);
  lv_obj_align(*label, LV_ALIGN_CENTER, 0, -5);

  // Unit label
  lv_obj_t *label_unit = lv_label_create(cont);
  lv_label_set_text(label_unit, unit);
  lv_obj_set_style_text_color(label_unit, lv_color_hex(0xcccccc), 0);
  lv_obj_align_to(label_unit, *label, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
}

void ui_Screen2_screen_init(void) {
  ui_Screen2 = lv_obj_create(NULL);

  // ЖЕСТКО ФИКСИРУЕМ РАЗМЕР ЭКРАНА - 800x480
  lv_obj_set_size(ui_Screen2, 720, 1280);
  lv_obj_set_pos(ui_Screen2, 0, 0);

  lv_obj_clear_flag(ui_Screen2, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(ui_Screen2, lv_color_hex(0x1a1a1a),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_Screen2, LV_OPA_COVER,
                          LV_PART_MAIN | LV_STATE_DEFAULT);

  // Проверяем границы экрана - все элементы должны быть внутри 800x480
  ESP_LOGI("SCREEN2", "Screen2 initialized with HARD FIXED size: 800x480");
  ESP_LOGI("SCREEN2", "All gauges will be constrained within these boundaries");

  // Create gauges in 3x2 grid (5 датчиков + место для Battery) - одинаковый
  // размер с Screen1 (250x225) ВСЕ ПОЗИЦИИ ПРОВЕРЕНЫ - НЕ ВЫХОДЯТ ЗА ГРАНИЦЫ
  // 800x480

  // Первый ряд (y=40)
  create_gauge(ui_Screen2, &ui_Arc_Oil_Pressure, &ui_Label_Oil_Pressure_Value,
               "Oil Pressure", "bar", lv_color_hex(0xFF6B35), 0, 10, 60, 40);

  create_gauge(ui_Screen2, &ui_Arc_Oil_Temp, &ui_Label_Oil_Temp_Value,
               "Oil Temp", "°C", lv_color_hex(0xFFD700), 60, 140, 480, 40);

  create_gauge(ui_Screen2, &ui_Arc_Water_Temp, &ui_Label_Water_Temp_Value,
               "Water Temp", "°C", lv_color_hex(0x00D4FF), 60, 120, 900, 40);

  // Второй ряд (y=380)
  create_gauge(ui_Screen2, &ui_Arc_Fuel_Pressure, &ui_Label_Fuel_Pressure_Value,
               "Fuel Pressure", "bar", lv_color_hex(0x00FF88), 0, 8, 60, 380);

  create_gauge(ui_Screen2, &ui_Arc_Battery_Voltage,
               &ui_Label_Battery_Voltage_Value, "Battery", "V",
               lv_color_hex(0xFFD700), 11, 15, 480, 380);
  // Датчик 5: x=285 to 535, расстояние от датчика 4: 285-265=20px

  // Всего 5 датчиков в 3x2 сетке

  // Apply initial layout
  ui_Screen2_update_layout();

  ESP_LOGI("SCREEN2", "Screen 2 initialized with basic touch functionality, "
                      "swipe gestures, and navigation buttons");

  // Setup animations for additional gauges
  lv_anim_init(&anim_oil_pressure);
  lv_anim_set_var(&anim_oil_pressure, ui_Arc_Oil_Pressure);
  lv_anim_set_values(&anim_oil_pressure, 0, 10);
  lv_anim_set_time(&anim_oil_pressure, 4000);
  lv_anim_set_playback_time(&anim_oil_pressure, 4000);
  lv_anim_set_repeat_count(&anim_oil_pressure, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_exec_cb(&anim_oil_pressure, anim_value_cb);

  // Start animation only if demo mode is enabled
  if (demo_mode_get_enabled()) {
    lv_anim_start(&anim_oil_pressure);
  }

  lv_anim_init(&anim_oil_temp);
  lv_anim_set_var(&anim_oil_temp, ui_Arc_Oil_Temp);
  lv_anim_set_values(&anim_oil_temp, 60, 140);
  lv_anim_set_time(&anim_oil_temp, 5000);
  lv_anim_set_playback_time(&anim_oil_temp, 5000);
  lv_anim_set_repeat_count(&anim_oil_temp, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_exec_cb(&anim_oil_temp, anim_value_cb);

  // Start animation only if demo mode is enabled
  if (demo_mode_get_enabled()) {
    lv_anim_start(&anim_oil_temp);
  }

  lv_anim_init(&anim_water_temp);
  lv_anim_set_var(&anim_water_temp, ui_Arc_Water_Temp);
  lv_anim_set_values(&anim_water_temp, 60, 120);
  lv_anim_set_time(&anim_water_temp, 6000);
  lv_anim_set_playback_time(&anim_water_temp, 6000);
  lv_anim_set_repeat_count(&anim_water_temp, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_exec_cb(&anim_water_temp, anim_value_cb);

  // Start animation only if demo mode is enabled
  if (demo_mode_get_enabled()) {
    lv_anim_start(&anim_water_temp);
  }

  lv_anim_init(&anim_fuel_pressure);
  lv_anim_set_var(&anim_fuel_pressure, ui_Arc_Fuel_Pressure);
  lv_anim_set_values(&anim_fuel_pressure, 0, 8);
  lv_anim_set_time(&anim_fuel_pressure, 3000);
  lv_anim_set_playback_time(&anim_fuel_pressure, 3000);
  lv_anim_set_repeat_count(&anim_fuel_pressure, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_exec_cb(&anim_fuel_pressure, anim_value_cb);

  // Start animation only if demo mode is enabled
  if (demo_mode_get_enabled()) {
    lv_anim_start(&anim_fuel_pressure);
  }

  lv_anim_init(&anim_battery_voltage);
  lv_anim_set_var(&anim_battery_voltage, ui_Arc_Battery_Voltage);
  lv_anim_set_values(&anim_battery_voltage, 110, 150); // 11.0V - 15.0V
  lv_anim_set_time(&anim_battery_voltage, 3500);
  lv_anim_set_playback_time(&anim_battery_voltage, 3500);
  lv_anim_set_repeat_count(&anim_battery_voltage, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_exec_cb(&anim_battery_voltage, anim_value_cb);

  // Start animation only if demo mode is enabled
  if (demo_mode_get_enabled()) {
    lv_anim_start(&anim_battery_voltage);
  }
}

// Function to control animations based on demo mode
void ui_Screen2_update_animations(bool demo_enabled) {
  // Safety check - ensure UI objects are initialized
  if (!ui_Arc_Oil_Pressure || !ui_Arc_Oil_Temp || !ui_Arc_Water_Temp ||
      !ui_Arc_Fuel_Pressure || !ui_Arc_Battery_Voltage) {
    ESP_LOGW("SCREEN2",
             "UI objects not initialized yet, skipping animation update");
    return;
  }

  if (demo_enabled) {
    // Start all animations
    lv_anim_start(&anim_oil_pressure);
    lv_anim_start(&anim_oil_temp);
    lv_anim_start(&anim_water_temp);
    lv_anim_start(&anim_fuel_pressure);
    lv_anim_start(&anim_battery_voltage);
    ESP_LOGI("SCREEN2", "Started all animations (demo mode enabled)");
  } else {
    // Stop all animations and reset to minimum values
    lv_anim_del(ui_Arc_Oil_Pressure, anim_value_cb);
    lv_anim_del(ui_Arc_Oil_Temp, anim_value_cb);
    lv_anim_del(ui_Arc_Water_Temp, anim_value_cb);
    lv_anim_del(ui_Arc_Fuel_Pressure, anim_value_cb);
    lv_anim_del(ui_Arc_Battery_Voltage, anim_value_cb);

    // Reset to minimum values
    lv_arc_set_value(ui_Arc_Oil_Pressure, 0);
    lv_arc_set_value(ui_Arc_Oil_Temp, 60);
    lv_arc_set_value(ui_Arc_Water_Temp, 60);
    lv_arc_set_value(ui_Arc_Fuel_Pressure, 0);
    lv_arc_set_value(ui_Arc_Battery_Voltage, 110);

    // Update labels
    lv_label_set_text_fmt(ui_Label_Oil_Pressure_Value, "%d", 0);
    lv_label_set_text_fmt(ui_Label_Oil_Temp_Value, "%d°C", 60);
    lv_label_set_text_fmt(ui_Label_Water_Temp_Value, "%d°C", 60);
    lv_label_set_text_fmt(ui_Label_Fuel_Pressure_Value, "%d", 0);
    lv_label_set_text_fmt(ui_Label_Battery_Voltage_Value, "%.1f", 11.0f);

    ESP_LOGI("SCREEN2", "Stopped all animations and reset to minimum values "
                        "(demo mode disabled)");
  }
}

// Function to control individual arc visibility
void ui_Screen2_update_arc_visibility(int arc_index, bool visible) {
  lv_obj_t *arc_container = NULL;
  const char *arc_name = NULL;

  // Map arc index to container and name
  switch (arc_index) {
  case 0: // Oil Pressure
    arc_container = lv_obj_get_parent(ui_Arc_Oil_Pressure);
    arc_name = "Oil Pressure";
    break;
  case 1: // Oil Temp
    arc_container = lv_obj_get_parent(ui_Arc_Oil_Temp);
    arc_name = "Oil Temp";
    break;
  case 2: // Water Temp
    arc_container = lv_obj_get_parent(ui_Arc_Water_Temp);
    arc_name = "Water Temp";
    break;
  case 3: // Fuel Pressure
    arc_container = lv_obj_get_parent(ui_Arc_Fuel_Pressure);
    arc_name = "Fuel Pressure";
    break;
  case 4: // Battery Voltage
    arc_container = lv_obj_get_parent(ui_Arc_Battery_Voltage);
    arc_name = "Battery Voltage";
    break;
  // case 5 убрана - теперь 5 датчиков
  default:
    ESP_LOGW("SCREEN2", "Invalid arc index: %d", arc_index);
    return;
  }

  if (!arc_container) {
    ESP_LOGW("SCREEN2", "Arc container not found for index %d", arc_index);
    return;
  }

  if (visible) {
    lv_obj_set_style_opa(arc_container, LV_OPA_COVER, 0);
    ESP_LOGI("SCREEN2", "%s gauge is now VISIBLE", arc_name);
  } else {
    lv_obj_set_style_opa(arc_container, LV_OPA_TRANSP, 0);
    ESP_LOGI("SCREEN2", "%s gauge is now HIDDEN", arc_name);
  }
}

void ui_Screen2_screen_destroy(void) {
  if (ui_Screen2)
    lv_obj_del(ui_Screen2);
  ui_Screen2 = NULL;
}
