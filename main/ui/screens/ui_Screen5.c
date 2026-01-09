// ECU Dashboard Screen 5 - ECU Data Gauges (Page 2)
#include "ui_Screen5.h"
#include "../ui.h"
#include "ecu_data.h"
#include "ui_helpers.h"
#include "ui_screen_manager.h"
#include <esp_log.h>
#include <stdio.h>

// Screen object
lv_obj_t *ui_Screen5;

// Gauge Objects
lv_obj_t *ui_Arc_Eng_TQ_Act;
lv_obj_t *ui_Label_Eng_TQ_Act_Value;
lv_obj_t *ui_Arc_Limit_TQ;
lv_obj_t *ui_Label_Limit_TQ_Value;

// Animation variables
static lv_anim_t anim_eng_tq_act;
static lv_anim_t anim_limit_tq;

// Function prototypes

static void anim_value_cb_screen5(void *var, int32_t v);

// Helper to get container of a gauge
static lv_obj_t *get_gauge_container(lv_obj_t *gauge) __attribute__((unused));
static lv_obj_t *get_gauge_container(lv_obj_t *gauge) {
  if (!gauge)
    return NULL;
  return lv_obj_get_parent(gauge);
}

// Update layout based on visible gauges
void ui_Screen5_update_layout(void) {
  // Legacy function - now handled by ui_layout_manager.c
  ESP_LOGI("SCREEN5", "Legacy update_layout called - ignored");
}

// Helper function to create a gauge
static void create_gauge(lv_obj_t *parent, lv_obj_t **arc, lv_obj_t **label,
                         const char *title, const char *unit, lv_color_t color,
                         int32_t min_val, int32_t max_val, int x, int y) {
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

  lv_obj_t *label_title = lv_label_create(cont);
  lv_label_set_text(label_title, title);
  lv_obj_set_style_text_color(label_title, lv_color_white(), 0);
  lv_obj_align(label_title, LV_ALIGN_BOTTOM_MID, 0, -15);

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

  *label = lv_label_create(cont);
  lv_label_set_text(*label, "0");
  lv_obj_set_style_text_color(*label, lv_color_white(), 0);
  lv_obj_set_style_text_font(*label, &lv_font_montserrat_24, 0);
  lv_obj_center(*label);
  lv_obj_align(*label, LV_ALIGN_CENTER, 0, -5);

  lv_obj_t *label_unit = lv_label_create(cont);
  lv_label_set_text(label_unit, unit);
  lv_obj_set_style_text_color(label_unit, lv_color_hex(0xcccccc), 0);
  lv_obj_align_to(label_unit, *label, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
}

// Main screen initialization
void ui_Screen5_screen_init(void) {
  ui_Screen5 = lv_obj_create(NULL);
  lv_obj_set_size(ui_Screen5, 1280, 720);
  lv_obj_set_pos(ui_Screen5, 0, 0);
  lv_obj_clear_flag(ui_Screen5, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(ui_Screen5, lv_color_hex(0x1a1a1a), 0);

  // Title removed as per user request to provide more space.

  // Row 1 (y=40)
  create_gauge(ui_Screen5, &ui_Arc_Eng_TQ_Act, &ui_Label_Eng_TQ_Act_Value,
               "Eng Tq Act", "Nm", lv_color_hex(0x00D4FF), 0, 500, 60, 40);
  create_gauge(ui_Screen5, &ui_Arc_Limit_TQ, &ui_Label_Limit_TQ_Value,
               "Torque Limit", "Nm", lv_color_hex(0x00FF88), 0, 500, 480, 40);

  // Apply initial layout
  ui_Screen5_update_layout();

  // Initialize animations
  lv_anim_init(&anim_eng_tq_act);
  lv_anim_set_var(&anim_eng_tq_act, ui_Arc_Eng_TQ_Act);
  lv_anim_set_values(&anim_eng_tq_act, 0, 500);
  lv_anim_set_time(&anim_eng_tq_act, 3000);
  lv_anim_set_playback_time(&anim_eng_tq_act, 3000);
  lv_anim_set_repeat_count(&anim_eng_tq_act, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_exec_cb(&anim_eng_tq_act, anim_value_cb_screen5);

  lv_anim_init(&anim_limit_tq);
  lv_anim_set_var(&anim_limit_tq, ui_Arc_Limit_TQ);
  lv_anim_set_values(&anim_limit_tq, 0, 500);
  lv_anim_set_time(&anim_limit_tq, 4000);
  lv_anim_set_playback_time(&anim_limit_tq, 4000);
  lv_anim_set_repeat_count(&anim_limit_tq, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_exec_cb(&anim_limit_tq, anim_value_cb_screen5);

  if (demo_mode_get_enabled()) {
    ui_Screen5_update_animations(true);
  }

  ESP_LOGI("SCREEN5", "Screen 5 initialized");
}

static void anim_value_cb_screen5(void *var, int32_t v) {
  lv_arc_set_value((lv_obj_t *)var, v);

  char buf[16];
  snprintf(buf, sizeof(buf), "%d", (int)v);

  if (var == ui_Arc_Eng_TQ_Act)
    lv_label_set_text(ui_Label_Eng_TQ_Act_Value, buf);
  else if (var == ui_Arc_Limit_TQ)
    lv_label_set_text(ui_Label_Limit_TQ_Value, buf);
}

void ui_Screen5_update_animations(bool demo_enabled) {
  if (demo_enabled) {
    lv_anim_start(&anim_eng_tq_act);
    lv_anim_start(&anim_limit_tq);
  } else {
    lv_anim_del(ui_Arc_Eng_TQ_Act, anim_value_cb_screen5);
    lv_anim_del(ui_Arc_Limit_TQ, anim_value_cb_screen5);
  }
}

// Function to control individual arc visibility
void ui_Screen5_update_arc_visibility(int arc_index, bool visible) {
  lv_obj_t *arc_container = NULL;
  const char *arc_name = NULL;

  // Map arc index to container and name
  switch (arc_index) {
  case 0: // Eng TQ Act
    arc_container = lv_obj_get_parent(ui_Arc_Eng_TQ_Act);
    arc_name = "Eng TQ Act";
    break;
  case 1: // Limit TQ
    arc_container = lv_obj_get_parent(ui_Arc_Limit_TQ);
    arc_name = "Limit TQ";
    break;
  default:
    ESP_LOGW("SCREEN5", "Invalid arc index: %d", arc_index);
    return;
  }

  if (!arc_container) {
    ESP_LOGW("SCREEN5", "Arc container not found for index %d", arc_index);
    return;
  }

  if (visible) {
    lv_obj_set_style_opa(arc_container, LV_OPA_COVER, 0);
    ESP_LOGI("SCREEN5", "%s gauge is now VISIBLE", arc_name);
  } else {
    lv_obj_set_style_opa(arc_container, LV_OPA_TRANSP, 0);
    ESP_LOGI("SCREEN5", "%s gauge is now HIDDEN", arc_name);
  }
}

void ui_Screen5_screen_destroy(void) {
  if (ui_Screen5) {
    lv_obj_del(ui_Screen5);
    ui_Screen5 = NULL;
  }
}
