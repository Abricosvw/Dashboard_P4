// ECU Dashboard Screen 4 - MRE Data Gauges (Page 1)
#include "ui_Screen4.h"
#include "../ui.h"
#include "ecu_data.h"
#include "ui_helpers.h"
#include "ui_screen_manager.h"
#include <esp_log.h>
#include <stdio.h>

// Screen object
lv_obj_t *ui_Screen4;

// MRE Gauge Objects
lv_obj_t *ui_Arc_Abs_Pedal;
lv_obj_t *ui_Arc_WG_Pos;
lv_obj_t *ui_Arc_BOV;
lv_obj_t *ui_Arc_TCU_TQ_Req;
lv_obj_t *ui_Arc_TCU_TQ_Act;
lv_obj_t *ui_Arc_Eng_TQ_Req;

// MRE Label Objects
lv_obj_t *ui_Label_Abs_Pedal_Value;
lv_obj_t *ui_Label_WG_Pos_Value;
lv_obj_t *ui_Label_BOV_Value;
lv_obj_t *ui_Label_TCU_TQ_Req_Value;
lv_obj_t *ui_Label_TCU_TQ_Act_Value;
lv_obj_t *ui_Label_TCU_TQ_Act_Value;
lv_obj_t *ui_Label_Eng_TQ_Req_Value;
lv_obj_t *ui_Label_Gear;

// Animation variables
static lv_anim_t anim_abs_pedal;
static lv_anim_t anim_wg_pos;
static lv_anim_t anim_bov;
static lv_anim_t anim_tcu_tq_req;
static lv_anim_t anim_tcu_tq_act;
static lv_anim_t anim_eng_tq_req;

// Function prototypes

static void anim_value_cb_screen4(void *var, int32_t v);

// Helper to get container of a gauge
static lv_obj_t *get_gauge_container(lv_obj_t *gauge) __attribute__((unused));
static lv_obj_t *get_gauge_container(lv_obj_t *gauge) {
  if (!gauge)
    return NULL;
  return lv_obj_get_parent(gauge);
}

// Update layout based on visible gauges
void ui_Screen4_update_layout(void) {
  // Legacy function - now handled by ui_layout_manager.c
  ESP_LOGI("SCREEN4", "Legacy update_layout called - ignored");
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
void ui_Screen4_screen_init(void) {
  ui_Screen4 = lv_obj_create(NULL);
  lv_obj_set_size(ui_Screen4, 1280, 720);
  lv_obj_set_pos(ui_Screen4, 0, 0);
  lv_obj_clear_flag(ui_Screen4, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(ui_Screen4, lv_color_hex(0x1a1a1a), 0);

  // Title removed as per user request to provide more space.

  // Row 1 (y=40)
  create_gauge(ui_Screen4, &ui_Arc_Abs_Pedal, &ui_Label_Abs_Pedal_Value,
               "Abs. Pedal Pos", "%", lv_color_hex(0x00D4FF), 0, 100, 60, 40);
  create_gauge(ui_Screen4, &ui_Arc_WG_Pos, &ui_Label_WG_Pos_Value,
               "Wastegate Pos", "%", lv_color_hex(0x00FF88), 0, 100, 480, 40);
  create_gauge(ui_Screen4, &ui_Arc_BOV, &ui_Label_BOV_Value, "BOV", "%",
               lv_color_hex(0xFFD700), 0, 100, 900, 40);
  // Row 2 (y=380)
  create_gauge(ui_Screen4, &ui_Arc_TCU_TQ_Req, &ui_Label_TCU_TQ_Req_Value,
               "TCU Tq Req", "Nm", lv_color_hex(0xFF6B35), 0, 500, 60, 380);
  create_gauge(ui_Screen4, &ui_Arc_TCU_TQ_Act, &ui_Label_TCU_TQ_Act_Value,
               "TCU Tq Act", "Nm", lv_color_hex(0xFF3366), 0, 500, 480, 380);
  create_gauge(ui_Screen4, &ui_Arc_Eng_TQ_Req, &ui_Label_Eng_TQ_Req_Value,
               "Eng Tq Req", "Nm", lv_color_hex(0x8A2BE2), 0, 500, 900, 380);

  // Gear Label
  ui_Label_Gear = lv_label_create(ui_Screen4);
  lv_label_set_text(ui_Label_Gear, "Gear: -");
  lv_obj_set_style_text_color(ui_Label_Gear, lv_color_white(), 0);
  lv_obj_set_style_text_font(ui_Label_Gear, &lv_font_montserrat_24, 0);
  lv_obj_align(ui_Label_Gear, LV_ALIGN_BOTTOM_MID, 0, -10);

  // Apply initial layout
  ui_Screen4_update_layout();

  // Initialize animations
  lv_anim_init(&anim_abs_pedal);
  lv_anim_set_var(&anim_abs_pedal, ui_Arc_Abs_Pedal);
  lv_anim_set_values(&anim_abs_pedal, 0, 100);
  lv_anim_set_time(&anim_abs_pedal, 3000);
  lv_anim_set_playback_time(&anim_abs_pedal, 3000);
  lv_anim_set_repeat_count(&anim_abs_pedal, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_exec_cb(&anim_abs_pedal, anim_value_cb_screen4);

  lv_anim_init(&anim_wg_pos);
  lv_anim_set_var(&anim_wg_pos, ui_Arc_WG_Pos);
  lv_anim_set_values(&anim_wg_pos, 0, 100);
  lv_anim_set_time(&anim_wg_pos, 2500);
  lv_anim_set_playback_time(&anim_wg_pos, 2500);
  lv_anim_set_repeat_count(&anim_wg_pos, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_exec_cb(&anim_wg_pos, anim_value_cb_screen4);

  lv_anim_init(&anim_bov);
  lv_anim_set_var(&anim_bov, ui_Arc_BOV);
  lv_anim_set_values(&anim_bov, 0, 100);
  lv_anim_set_time(&anim_bov, 2000);
  lv_anim_set_playback_time(&anim_bov, 2000);
  lv_anim_set_repeat_count(&anim_bov, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_exec_cb(&anim_bov, anim_value_cb_screen4);

  lv_anim_init(&anim_tcu_tq_req);
  lv_anim_set_var(&anim_tcu_tq_req, ui_Arc_TCU_TQ_Req);
  lv_anim_set_values(&anim_tcu_tq_req, 0, 500);
  lv_anim_set_time(&anim_tcu_tq_req, 4000);
  lv_anim_set_playback_time(&anim_tcu_tq_req, 4000);
  lv_anim_set_repeat_count(&anim_tcu_tq_req, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_exec_cb(&anim_tcu_tq_req, anim_value_cb_screen4);

  lv_anim_init(&anim_tcu_tq_act);
  lv_anim_set_var(&anim_tcu_tq_act, ui_Arc_TCU_TQ_Act);
  lv_anim_set_values(&anim_tcu_tq_act, 0, 500);
  lv_anim_set_time(&anim_tcu_tq_act, 3500);
  lv_anim_set_playback_time(&anim_tcu_tq_act, 3500);
  lv_anim_set_repeat_count(&anim_tcu_tq_act, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_exec_cb(&anim_tcu_tq_act, anim_value_cb_screen4);

  lv_anim_init(&anim_eng_tq_req);
  lv_anim_set_var(&anim_eng_tq_req, ui_Arc_Eng_TQ_Req);
  lv_anim_set_values(&anim_eng_tq_req, 0, 500);
  lv_anim_set_time(&anim_eng_tq_req, 4500);
  lv_anim_set_playback_time(&anim_eng_tq_req, 4500);
  lv_anim_set_repeat_count(&anim_eng_tq_req, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_exec_cb(&anim_eng_tq_req, anim_value_cb_screen4);

  if (demo_mode_get_enabled()) {
    ui_Screen4_update_animations(true);
  }

  ESP_LOGI("SCREEN4", "Screen 4 initialized with ECU gauges");
}

static void anim_value_cb_screen4(void *var, int32_t v) {
  lv_arc_set_value((lv_obj_t *)var, v);

  char buf[16];
  snprintf(buf, sizeof(buf), "%d", (int)v);

  if (var == ui_Arc_Abs_Pedal)
    lv_label_set_text(ui_Label_Abs_Pedal_Value, buf);
  else if (var == ui_Arc_WG_Pos)
    lv_label_set_text(ui_Label_WG_Pos_Value, buf);
  else if (var == ui_Arc_BOV)
    lv_label_set_text(ui_Label_BOV_Value, buf);
  else if (var == ui_Arc_TCU_TQ_Req)
    lv_label_set_text(ui_Label_TCU_TQ_Req_Value, buf);
  else if (var == ui_Arc_TCU_TQ_Act)
    lv_label_set_text(ui_Label_TCU_TQ_Act_Value, buf);
  else if (var == ui_Arc_Eng_TQ_Req)
    lv_label_set_text(ui_Label_Eng_TQ_Req_Value, buf);
}

void ui_Screen4_update_animations(bool demo_enabled) {
  if (demo_enabled) {
    lv_anim_start(&anim_abs_pedal);
    lv_anim_start(&anim_wg_pos);
    lv_anim_start(&anim_bov);
    lv_anim_start(&anim_tcu_tq_req);
    lv_anim_start(&anim_tcu_tq_act);
    lv_anim_start(&anim_eng_tq_req);
  } else {
    lv_anim_del(ui_Arc_Abs_Pedal, anim_value_cb_screen4);
    lv_anim_del(ui_Arc_WG_Pos, anim_value_cb_screen4);
    lv_anim_del(ui_Arc_BOV, anim_value_cb_screen4);
    lv_anim_del(ui_Arc_TCU_TQ_Req, anim_value_cb_screen4);
    lv_anim_del(ui_Arc_TCU_TQ_Act, anim_value_cb_screen4);
    lv_anim_del(ui_Arc_Eng_TQ_Req, anim_value_cb_screen4);
    // Here you might want to reset the arcs to their minimum values
  }
}

// Function to control individual arc visibility
void ui_Screen4_update_arc_visibility(int arc_index, bool visible) {
  lv_obj_t *arc_container = NULL;
  const char *arc_name = NULL;

  // Map arc index to container and name
  switch (arc_index) {
  case 0: // Abs Pedal
    arc_container = lv_obj_get_parent(ui_Arc_Abs_Pedal);
    arc_name = "Abs Pedal";
    break;
  case 1: // WG Pos
    arc_container = lv_obj_get_parent(ui_Arc_WG_Pos);
    arc_name = "WG Pos";
    break;
  case 2: // BOV
    arc_container = lv_obj_get_parent(ui_Arc_BOV);
    arc_name = "BOV";
    break;
  case 3: // TCU TQ Req
    arc_container = lv_obj_get_parent(ui_Arc_TCU_TQ_Req);
    arc_name = "TCU TQ Req";
    break;
  case 4: // TCU TQ Act
    arc_container = lv_obj_get_parent(ui_Arc_TCU_TQ_Act);
    arc_name = "TCU TQ Act";
    break;
  case 5: // Eng TQ Req
    arc_container = lv_obj_get_parent(ui_Arc_Eng_TQ_Req);
    arc_name = "Eng TQ Req";
    break;
  default:
    ESP_LOGW("SCREEN4", "Invalid arc index: %d", arc_index);
    return;
  }

  if (!arc_container) {
    ESP_LOGW("SCREEN4", "Arc container not found for index %d", arc_index);
    return;
  }

  if (visible) {
    lv_obj_set_style_opa(arc_container, LV_OPA_COVER, 0);
    ESP_LOGI("SCREEN4", "%s gauge is now VISIBLE", arc_name);
  } else {
    lv_obj_set_style_opa(arc_container, LV_OPA_TRANSP, 0);
    ESP_LOGI("SCREEN4", "%s gauge is now HIDDEN", arc_name);
  }
}

void ui_Screen4_screen_destroy(void) {
  if (ui_Screen4) {
    lv_obj_del(ui_Screen4);
    ui_Screen4 = NULL;
  }
}
