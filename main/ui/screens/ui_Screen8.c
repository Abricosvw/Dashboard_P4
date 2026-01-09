#include "ui_Screen8.h"
#include "../ui.h"
#include "../ui_screen_manager.h"
#include "esp_log.h"
#include "settings_config.h"
#include <stdio.h>

lv_obj_t *ui_Screen8 = NULL;

// Gauge objects
lv_obj_t *ui_Gauge_RPM_S8 = NULL;
lv_obj_t *ui_Gauge_Speed_S8 = NULL;
lv_obj_t *ui_Gauge_Boost_S8 = NULL; // Unused in new design maybe? Or used for
                                    // something else? Kept for compatibility.
lv_obj_t *ui_Gauge_Temp_S8 = NULL;  // Unused
lv_obj_t *ui_Gauge_Fuel_S8 = NULL;

// New Objects
lv_obj_t *ui_Bar_Boost_S8 = NULL;

// Label objects
lv_obj_t *ui_Label_RPM_Val_S8 = NULL;
lv_obj_t *ui_Label_Speed_Val_S8 = NULL;
lv_obj_t *ui_Label_Gear_S8 = NULL;

lv_obj_t *ui_Label_Boost_Val_S8 = NULL;
lv_obj_t *ui_Label_OilTemp_Val_S8 = NULL;
lv_obj_t *ui_Label_OilPress_Val_S8 = NULL;
lv_obj_t *ui_Label_WaterTemp_Val_S8 = NULL;
lv_obj_t *ui_Label_AirTemp_Val_S8 = NULL;

LV_FONT_DECLARE(lv_font_montserrat_14);
LV_FONT_DECLARE(lv_font_montserrat_20);
LV_FONT_DECLARE(lv_font_montserrat_24);
LV_FONT_DECLARE(lv_font_montserrat_48);

// Animation objects
static lv_anim_t anim_rpm_s8;
static lv_anim_t anim_speed_s8;
static lv_anim_t anim_boost_s8;

// Callback for demo animation
static void anim_rpm_cb_s8(void *var, int32_t v) {
  if (ui_Gauge_RPM_S8)
    lv_arc_set_value(ui_Gauge_RPM_S8, v);
  if (ui_Label_RPM_Val_S8)
    lv_label_set_text_fmt(ui_Label_RPM_Val_S8, "%d", (int)v);
}
static void anim_speed_cb_s8(void *var, int32_t v) {
  if (ui_Gauge_Speed_S8)
    lv_arc_set_value(ui_Gauge_Speed_S8, v);
  if (ui_Label_Speed_Val_S8)
    lv_label_set_text_fmt(ui_Label_Speed_Val_S8, "%d", (int)v);
}
static void anim_boost_cb_s8(void *var, int32_t v) {
  if (ui_Bar_Boost_S8)
    lv_bar_set_value(ui_Bar_Boost_S8, v, LV_ANIM_OFF);
  if (ui_Label_Boost_Val_S8)
    lv_label_set_text_fmt(ui_Label_Boost_Val_S8, "%d", (int)v);
}

void ui_Screen8_update_animations(bool demo_enabled) {
  if (demo_enabled) {
    // RPM Animation
    lv_anim_init(&anim_rpm_s8);
    lv_anim_set_var(&anim_rpm_s8, ui_Gauge_RPM_S8);
    lv_anim_set_values(&anim_rpm_s8, 0, 7000); // 0-7000 RPM
    lv_anim_set_time(&anim_rpm_s8, 3000);
    lv_anim_set_playback_time(&anim_rpm_s8, 3000);
    lv_anim_set_repeat_count(&anim_rpm_s8, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&anim_rpm_s8, anim_rpm_cb_s8);
    lv_anim_start(&anim_rpm_s8);

    // Speed Animation
    lv_anim_init(&anim_speed_s8);
    lv_anim_set_var(&anim_speed_s8, ui_Gauge_Speed_S8);
    lv_anim_set_values(&anim_speed_s8, 0, 280); // 0-280 km/h
    lv_anim_set_time(&anim_speed_s8, 5000);
    lv_anim_set_playback_time(&anim_speed_s8, 5000);
    lv_anim_set_repeat_count(&anim_speed_s8, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&anim_speed_s8, anim_speed_cb_s8);
    lv_anim_start(&anim_speed_s8);

    // Boost Animation
    lv_anim_init(&anim_boost_s8);
    lv_anim_set_var(&anim_boost_s8, ui_Bar_Boost_S8);
    lv_anim_set_values(&anim_boost_s8, 0, 250);
    lv_anim_set_time(&anim_boost_s8, 4000);
    lv_anim_set_playback_time(&anim_boost_s8, 4000);
    lv_anim_set_repeat_count(&anim_boost_s8, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&anim_boost_s8, anim_boost_cb_s8);
    lv_anim_start(&anim_boost_s8);
  } else {
    lv_anim_del(ui_Gauge_RPM_S8, anim_rpm_cb_s8);
    lv_anim_del(ui_Gauge_Speed_S8, anim_speed_cb_s8);
    lv_anim_del(ui_Bar_Boost_S8, anim_boost_cb_s8);

    if (ui_Gauge_RPM_S8)
      lv_arc_set_value(ui_Gauge_RPM_S8, 0);
    if (ui_Gauge_Speed_S8)
      lv_arc_set_value(ui_Gauge_Speed_S8, 0);
    if (ui_Bar_Boost_S8)
      lv_bar_set_value(ui_Bar_Boost_S8, 0, LV_ANIM_OFF);

    if (ui_Label_RPM_Val_S8)
      lv_label_set_text(ui_Label_RPM_Val_S8, "0");
    if (ui_Label_Speed_Val_S8)
      lv_label_set_text(ui_Label_Speed_Val_S8, "0");
    if (ui_Label_Boost_Val_S8)
      lv_label_set_text(ui_Label_Boost_Val_S8, "0");
  }
}

// Navigation Button Callback
static void nav_button_cb_s8(lv_event_t *e) {
  long forward = (long)lv_event_get_user_data(e);
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    ui_switch_to_next_enabled_screen((bool)forward);
  }
}

// Swipe handler

// Helper to create center panel rows
static void create_center_row(lv_obj_t *parent, const char *title,
                              const char *unit, lv_obj_t **label_obj,
                              const char *def_val) {
  lv_obj_t *row_cont = lv_obj_create(parent);
  lv_obj_set_size(row_cont, 280, 50);
  lv_obj_set_style_bg_opa(row_cont, 0, 0);
  lv_obj_set_style_border_width(row_cont, 0, 0);
  lv_obj_set_flex_flow(row_cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row_cont, LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(row_cont, 5, 0);

  lv_obj_t *lbl_title = lv_label_create(row_cont);
  lv_label_set_text(lbl_title, title);
  lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xAAAAAA), 0);
  lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, 0);

  lv_obj_t *val_cont = lv_obj_create(row_cont);
  lv_obj_set_size(val_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(val_cont, 0, 0);
  lv_obj_set_style_border_width(val_cont, 0, 0);
  lv_obj_set_flex_flow(val_cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(val_cont, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END,
                        LV_FLEX_ALIGN_END);
  lv_obj_set_style_pad_gap(val_cont, 5, 0);

  *label_obj = lv_label_create(val_cont);
  lv_label_set_text(*label_obj, def_val);
  lv_obj_set_style_text_color(*label_obj, lv_color_white(), 0);
  lv_obj_set_style_text_font(*label_obj, &lv_font_montserrat_24, 0);

  lv_obj_t *lbl_unit = lv_label_create(val_cont);
  lv_label_set_text(lbl_unit, unit);
  lv_obj_set_style_text_color(lbl_unit, lv_color_hex(0xAAAAAA), 0);
  lv_obj_set_style_text_font(lbl_unit, &lv_font_montserrat_14, 0);
}

void ui_Screen8_screen_init(void) {
  ESP_LOGI("SCREEN8", "Initializing Classic Sports Dashboard");

  ui_Screen8 = lv_obj_create(NULL);
  lv_obj_set_size(ui_Screen8, 1280, 720);
  lv_obj_clear_flag(ui_Screen8, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(ui_Screen8, lv_color_hex(0x121212),
                            0); // Background Dark

  // --- Header ---
  lv_obj_t *header = lv_obj_create(ui_Screen8);
  lv_obj_set_size(header, 1200, 50);
  lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);
  lv_obj_set_style_bg_opa(header, 0, 0);
  lv_obj_set_style_border_width(header, 0, 0);

  lv_obj_t *track_mode_lbl = lv_label_create(header);
  lv_label_set_text(track_mode_lbl, "TRACK MODE");
  lv_obj_center(track_mode_lbl);
  lv_obj_set_style_text_color(track_mode_lbl, lv_color_hex(0x666666), 0);
  lv_obj_set_style_text_letter_space(track_mode_lbl, 2, 0);

  // --- LEFT: RPM Gauge ---
  lv_obj_t *rpm_cont = lv_obj_create(ui_Screen8);
  lv_obj_set_size(rpm_cont, 300, 300);
  lv_obj_align(rpm_cont, LV_ALIGN_LEFT_MID, 20, 0);
  lv_obj_set_style_bg_opa(rpm_cont, 0, 0);
  lv_obj_set_style_border_width(rpm_cont, 0, 0);

  ui_Gauge_RPM_S8 = lv_arc_create(rpm_cont);
  lv_obj_set_size(ui_Gauge_RPM_S8, 280, 280);
  lv_obj_center(ui_Gauge_RPM_S8);
  lv_arc_set_rotation(ui_Gauge_RPM_S8, 135);
  lv_arc_set_bg_angles(ui_Gauge_RPM_S8, 0, 270);
  lv_arc_set_range(ui_Gauge_RPM_S8, 0, 7000); // 0-7000 RPM
  // Style
  lv_obj_set_style_arc_color(ui_Gauge_RPM_S8, lv_color_hex(0x333333),
                             LV_PART_MAIN);
  lv_obj_set_style_arc_width(ui_Gauge_RPM_S8, 20, LV_PART_MAIN);
  lv_obj_set_style_arc_color(ui_Gauge_RPM_S8, lv_color_hex(0xFF0000),
                             LV_PART_INDICATOR); // Primary Red
  lv_obj_set_style_arc_width(ui_Gauge_RPM_S8, 20, LV_PART_INDICATOR);
  lv_obj_remove_style(ui_Gauge_RPM_S8, NULL, LV_PART_KNOB);
  lv_obj_clear_flag(ui_Gauge_RPM_S8, LV_OBJ_FLAG_CLICKABLE);

  // Inner Value
  ui_Label_RPM_Val_S8 = lv_label_create(rpm_cont);
  lv_label_set_text(ui_Label_RPM_Val_S8, "0");
  lv_obj_set_style_text_font(ui_Label_RPM_Val_S8, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(ui_Label_RPM_Val_S8, lv_color_white(), 0);
  lv_obj_center(ui_Label_RPM_Val_S8);

  lv_obj_t *rpm_lbl = lv_label_create(rpm_cont);
  lv_label_set_text(rpm_lbl, "RPM");
  lv_obj_set_style_text_color(rpm_lbl, lv_color_hex(0xFF0000), 0);
  lv_obj_align(rpm_lbl, LV_ALIGN_CENTER, 0, 40);

  // --- RIGHT: Speed Gauge ---
  lv_obj_t *speed_cont = lv_obj_create(ui_Screen8);
  lv_obj_set_size(speed_cont, 300, 300);
  lv_obj_align(speed_cont, LV_ALIGN_RIGHT_MID, -20, 0);
  lv_obj_set_style_bg_opa(speed_cont, 0, 0);
  lv_obj_set_style_border_width(speed_cont, 0, 0);

  ui_Gauge_Speed_S8 = lv_arc_create(speed_cont);
  lv_obj_set_size(ui_Gauge_Speed_S8, 280, 280);
  lv_obj_center(ui_Gauge_Speed_S8);
  lv_arc_set_rotation(ui_Gauge_Speed_S8, 135);
  lv_arc_set_bg_angles(ui_Gauge_Speed_S8, 0, 270);
  lv_arc_set_range(ui_Gauge_Speed_S8, 0, 300); // 0-300 km/h
  // Style
  lv_obj_set_style_arc_color(ui_Gauge_Speed_S8, lv_color_hex(0x333333),
                             LV_PART_MAIN);
  lv_obj_set_style_arc_width(ui_Gauge_Speed_S8, 20, LV_PART_MAIN);
  lv_obj_set_style_arc_color(ui_Gauge_Speed_S8, lv_color_white(),
                             LV_PART_INDICATOR); // White
  lv_obj_set_style_arc_width(ui_Gauge_Speed_S8, 20, LV_PART_INDICATOR);
  lv_obj_remove_style(ui_Gauge_Speed_S8, NULL, LV_PART_KNOB);
  lv_obj_clear_flag(ui_Gauge_Speed_S8, LV_OBJ_FLAG_CLICKABLE);

  // Inner Value
  ui_Label_Speed_Val_S8 = lv_label_create(speed_cont);
  lv_label_set_text(ui_Label_Speed_Val_S8, "0");
  lv_obj_set_style_text_font(ui_Label_Speed_Val_S8, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(ui_Label_Speed_Val_S8, lv_color_white(), 0);
  lv_obj_center(ui_Label_Speed_Val_S8);

  lv_obj_t *speed_lbl = lv_label_create(speed_cont);
  lv_label_set_text(speed_lbl, "KM/H");
  lv_obj_set_style_text_color(speed_lbl, lv_color_hex(0x888888), 0);
  lv_obj_align(speed_lbl, LV_ALIGN_CENTER, 0, 40);

  // --- CENTER: Data Panel ---
  lv_obj_t *center_panel = lv_obj_create(ui_Screen8);
  lv_obj_set_size(center_panel, 300, 320);
  lv_obj_align(center_panel, LV_ALIGN_CENTER, 0, 10);
  lv_obj_set_style_bg_color(center_panel, lv_color_hex(0x1E1E1E), 0);
  lv_obj_set_style_border_color(center_panel, lv_color_hex(0x333333), 0);
  lv_obj_set_style_radius(center_panel, 15, 0);
  lv_obj_set_flex_flow(center_panel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(center_panel, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(center_panel, 10, 0);

  // Boost Row (Special with Bar)
  lv_obj_t *boost_row = lv_obj_create(center_panel);
  lv_obj_set_size(boost_row, 280, 60);
  lv_obj_set_style_bg_opa(boost_row, 0, 0);
  lv_obj_set_style_border_width(boost_row, 0, 0);
  lv_obj_set_flex_flow(boost_row, LV_FLEX_FLOW_COLUMN);

  lv_obj_t *boost_header = lv_obj_create(boost_row);
  lv_obj_set_size(boost_header, 280, 30);
  lv_obj_set_style_bg_opa(boost_header, 0, 0);
  lv_obj_set_style_border_width(boost_header, 0, 0);

  lv_obj_t *bt_lbl = lv_label_create(boost_header);
  lv_label_set_text(bt_lbl, "BOOST");
  lv_obj_set_style_text_color(bt_lbl, lv_color_hex(0xAAAAAA), 0);
  lv_obj_align(bt_lbl, LV_ALIGN_LEFT_MID, 0, 0);

  ui_Label_Boost_Val_S8 = lv_label_create(boost_header);
  lv_label_set_text(ui_Label_Boost_Val_S8, "0");
  lv_obj_set_style_text_color(ui_Label_Boost_Val_S8, lv_color_white(), 0);
  lv_obj_set_style_text_font(ui_Label_Boost_Val_S8, &lv_font_montserrat_24, 0);
  lv_obj_align(ui_Label_Boost_Val_S8, LV_ALIGN_RIGHT_MID, -30, 0);

  lv_obj_t *kuag_lbl = lv_label_create(boost_header);
  lv_label_set_text(kuag_lbl, "kPa");
  lv_obj_set_style_text_font(kuag_lbl, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(kuag_lbl, lv_color_hex(0xFF0000), 0);
  lv_obj_align(kuag_lbl, LV_ALIGN_RIGHT_MID, 0, 2);

  // Boost Bar
  ui_Bar_Boost_S8 = lv_bar_create(boost_row);
  lv_obj_set_size(ui_Bar_Boost_S8, 280, 8);
  lv_bar_set_range(ui_Bar_Boost_S8, 0, 250);
  lv_obj_set_style_bg_color(ui_Bar_Boost_S8, lv_color_hex(0x333333),
                            LV_PART_MAIN);
  lv_obj_set_style_bg_color(ui_Bar_Boost_S8, lv_color_hex(0xFF0000),
                            LV_PART_INDICATOR);
  lv_obj_align(ui_Bar_Boost_S8, LV_ALIGN_BOTTOM_MID, 0, 0);

  // Separator line
  lv_obj_t *line = lv_obj_create(center_panel);
  lv_obj_set_size(line, 260, 1);
  lv_obj_set_style_bg_color(line, lv_color_hex(0x333333), 0);

  // Other Rows
  create_center_row(center_panel, "OIL TEMP", "C", &ui_Label_OilTemp_Val_S8,
                    "0");
  create_center_row(center_panel, "OIL PRESS", "kPa", &ui_Label_OilPress_Val_S8,
                    "0");
  create_center_row(center_panel, "WATER TEMP", "C", &ui_Label_WaterTemp_Val_S8,
                    "0");
  create_center_row(center_panel, "AIR TEMP", "C", &ui_Label_AirTemp_Val_S8,
                    "0");

  // --- FOOTER: Gear ---
  lv_obj_t *footer = lv_obj_create(ui_Screen8);
  lv_obj_set_size(footer, 300, 60);
  lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_obj_set_style_bg_color(footer, lv_color_hex(0x1E1E1E), 0);
  lv_obj_set_style_radius(footer, 30, 0);
  lv_obj_set_style_border_color(footer, lv_color_hex(0x333333), 0);
  lv_obj_set_style_border_width(footer, 1, 0);
  lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);

  const char *gears[] = {"P", "N", "D"};
  for (int i = 0; i < 3; i++) {
    lv_obj_t *g = lv_label_create(footer);
    lv_label_set_text(g, gears[i]);
    lv_obj_set_style_text_font(g, &lv_font_montserrat_24, 0);

    if (i == 2) { // D selected
      lv_obj_set_style_text_color(g, lv_color_hex(0xFFFFFF), 0);
      lv_obj_set_style_bg_color(g, lv_color_hex(0xFF0000), 0);
      lv_obj_set_style_bg_opa(g, LV_OPA_COVER, 0);
      lv_obj_set_style_pad_all(g, 10, 0);
      lv_obj_set_style_radius(g, 5, 0);
    } else {
      lv_obj_set_style_text_color(g, lv_color_hex(0x444444), 0);
      lv_obj_set_style_pad_all(g, 10, 0);
    }
  }

  if (demo_mode_get_enabled()) {
    ui_Screen8_update_animations(true);
  }

  if (demo_mode_get_enabled()) {
    ui_Screen8_update_animations(true);
  }
}

void ui_Screen8_update(void) {
  // This function can be populated if we have a direct struct to read from
}

void ui_Screen8_screen_destroy(void) {
  if (ui_Screen8)
    lv_obj_del(ui_Screen8);
  ui_Screen8 = NULL;
}
