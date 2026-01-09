// ECU Dashboard Screen 6 - Device Parameters Settings
// Sixth screen dedicated to device configuration and settings

#include "ui_Screen6.h"
#include "../ui.h"
#include "ai_manager.h" // Include AI Manager
#include "ui_Screen1.h"
#include "ui_Screen2.h"
#include "ui_Screen4.h"
#include "ui_Screen5.h"
#include "ui_screen_manager.h"

#include "../background_task.h" // Фоновая задача для асинхронных операций
#include "can_definitions.h"    // Platform definitions
#include "can_parser.h"         // For can_parser_set_platform
#include "ecu_data.h"           // For system_settings_t
#include "settings_config.h"    // Убедитесь, что этот файл подключен
#include "ui_events.h"
#include "ui_helpers.h"

#include <esp_log.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

LV_FONT_DECLARE(lv_font_montserrat_24);

// Screen object
lv_obj_t *ui_Screen6;

// Device Parameters Settings Objects
void *ui_Label_Device_Title;
void *ui_Button_Demo_Mode;
void *ui_Button_Enable_Screen3;
void *ui_Button_Nav_Buttons;
void *ui_Button_Save_Settings;
void *ui_Button_Reset_Settings;
void *ui_Button_AI; // AI Button

// Touch cursor object
lv_obj_t *ui_Touch_Cursor_Screen6;

// Gauge List Container
static lv_obj_t *ui_Container_GaugeList = NULL;
// Platform List Container
static lv_obj_t *ui_Container_PlatformList = NULL;

// Settings state
static int settings_modified = 0;
static bool demo_mode_enabled = false;
static bool screen3_enabled = false;
static bool nav_buttons_enabled = true;
static bool show_map = true;
static bool show_wastegate = true;
static bool show_tps = true;
static bool show_rpm = true;
static bool show_boost = true;
static bool show_tcu = true;

// Screen 2
static bool show_oil_press = true;
static bool show_oil_temp = true;
static bool show_water_temp = true;
static bool show_fuel_press = true;
static bool show_battery = true;

// Screen 4
static bool show_pedal = true;
static bool show_wg_pos = true;
static bool show_bov = true;
static bool show_tcu_req = true;
static bool show_tcu_act = true;
static bool show_eng_req = true;

// Screen 5
static bool show_eng_act = true;
static bool show_limit_tq = true;

// Function prototypes
static void screen6_touch_handler(lv_event_t *e);

static void save_settings_event_cb(lv_event_t *e);
static void reset_settings_event_cb(lv_event_t *e);
static void reset_settings_event_cb(lv_event_t *e);
static void demo_mode_event_cb(lv_event_t *e);
static void enable_screen3_event_cb(lv_event_t *e);
static void nav_buttons_event_cb(lv_event_t *e);
static void gauge_checkbox_event_cb(lv_event_t *e);
static void platform_checkbox_event_cb(lv_event_t *e);
static void ai_button_event_cb(lv_event_t *e); // New callback

void ui_update_touch_cursor_screen6(void *point);

// Screen6 utility functions
void ui_Screen6_load_settings(void);
void ui_Screen6_save_settings(void);
void ui_Screen6_update_button_states(void);
void ui_save_device_settings(void);
void ui_reset_device_settings(void);

// Touch handler for general touch events
static void screen6_touch_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_PRESSED) {
    lv_point_t point;
    lv_indev_t *indev = lv_indev_get_act();
    if (indev) {
      lv_indev_get_point(indev, &point);
      ui_update_touch_cursor_screen6(&point);
    }
  } else if (code == LV_EVENT_RELEASED) {
    lv_obj_add_flag((lv_obj_t *)ui_Touch_Cursor_Screen6, LV_OBJ_FLAG_HIDDEN);
  }
}

// Custom swipe handler removed in favor of global ui_screen_swipe_event_cb

// Save settings button event callback
static void save_settings_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    ui_save_device_settings();
    settings_modified = 0;
    ESP_LOGI("SCREEN6", "Device settings save triggered");
  }
}

// Reset settings button event callback
static void reset_settings_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    ui_reset_device_settings();
    settings_modified = 0;
    ESP_LOGI("SCREEN6", "Device settings reset triggered");
  }
}

// Demo mode button event callback
static void demo_mode_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    demo_mode_enabled = !demo_mode_enabled;

    // Update button visuals on this screen
    ui_Screen6_update_button_states();

    // Update animations on all screens
    ui_set_global_demo_mode(demo_mode_enabled);

    settings_modified = 1;
    ESP_LOGI("SCREEN6", "Global demo mode toggled to: %s",
             demo_mode_enabled ? "ON" : "OFF");
  }
}

// Enable Screen3 button event callback
static void enable_screen3_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    screen3_enabled = !screen3_enabled;
    ui_Screen6_update_button_states();
    settings_modified = 1;
    ESP_LOGI("SCREEN6", "Screen3 toggled to: %s",
             screen3_enabled ? "ON" : "OFF");
  }
}

// AI Button event callback
static void ai_button_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    ai_manager_trigger_listening();

    // Feedback
    lv_obj_t *btn = (lv_obj_t *)e->target;
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xFFFFFF), 0); // Flash

    ESP_LOGI("SCREEN6", "AI Listening Triggered");
  }
}

// Navigation buttons toggle event callback
static void nav_buttons_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    nav_buttons_enabled = !nav_buttons_enabled;
    ui_Screen6_update_button_states();
    settings_modified = 1;
    ESP_LOGI("SCREEN6", "Nav buttons toggled to: %s",
             nav_buttons_enabled ? "ON" : "OFF");
  }
}

// Helper to find gauge ID from text
static gauge_id_t get_gauge_id_from_text(const char *txt) {
  if (strcmp(txt, "MAP") == 0)
    return GAUGE_MAP;
  if (strcmp(txt, "Wastegate") == 0)
    return GAUGE_WASTEGATE;
  if (strcmp(txt, "TPS") == 0)
    return GAUGE_TPS;
  if (strcmp(txt, "RPM") == 0)
    return GAUGE_RPM;
  if (strcmp(txt, "Boost") == 0)
    return GAUGE_BOOST;
  if (strcmp(txt, "TCU") == 0)
    return GAUGE_TCU;

  if (strcmp(txt, "Oil Press") == 0)
    return GAUGE_OIL_PRESS;
  if (strcmp(txt, "Oil Temp") == 0)
    return GAUGE_OIL_TEMP;
  if (strcmp(txt, "Water Temp") == 0)
    return GAUGE_WATER_TEMP;
  if (strcmp(txt, "Fuel Press") == 0)
    return GAUGE_FUEL_PRESS;
  if (strcmp(txt, "Battery") == 0)
    return GAUGE_BATTERY;

  if (strcmp(txt, "Pedal") == 0)
    return GAUGE_PEDAL;
  if (strcmp(txt, "WG Pos") == 0)
    return GAUGE_WG_POS;
  if (strcmp(txt, "BOV") == 0)
    return GAUGE_BOV;
  if (strcmp(txt, "TCU Req") == 0)
    return GAUGE_TCU_REQ;
  if (strcmp(txt, "TCU Act") == 0)
    return GAUGE_TCU_ACT;
  if (strcmp(txt, "Eng Req") == 0)
    return GAUGE_ENG_REQ;

  if (strcmp(txt, "Eng Act") == 0)
    return GAUGE_ENG_ACT;
  if (strcmp(txt, "Limit TQ") == 0)
    return GAUGE_LIMIT_TQ;

  return GAUGE_NONE;
}

// Helper to add gauge to ordered list
static void add_gauge_to_list(system_settings_t *settings, gauge_id_t id) {
  if (!settings || id == GAUGE_NONE)
    return;

  // Check if already exists
  for (int i = 0; i < settings->active_gauge_count; i++) {
    if (settings->active_gauge_ids[i] == id)
      return;
  }

  if (settings->active_gauge_count < 24) {
    settings->active_gauge_ids[settings->active_gauge_count++] = id;
  }
}

// Helper to remove gauge from ordered list
static void remove_gauge_from_list(system_settings_t *settings, gauge_id_t id) {
  if (!settings || id == GAUGE_NONE)
    return;

  int found_idx = -1;
  for (int i = 0; i < settings->active_gauge_count; i++) {
    if (settings->active_gauge_ids[i] == id) {
      found_idx = i;
      break;
    }
  }

  if (found_idx != -1) {
    // Shift remaining items
    for (int i = found_idx; i < settings->active_gauge_count - 1; i++) {
      settings->active_gauge_ids[i] = settings->active_gauge_ids[i + 1];
    }
    settings->active_gauge_count--;
  }
}

// Gauge checkbox event callback
static void gauge_checkbox_event_cb(lv_event_t *e) {
  lv_obj_t *cb = lv_event_get_target(e);
  const char *txt = lv_checkbox_get_text(cb);
  bool checked = lv_obj_has_state(cb, LV_STATE_CHECKED);

  if (strcmp(txt, "MAP") == 0)
    show_map = checked;
  else if (strcmp(txt, "Wastegate") == 0)
    show_wastegate = checked;
  else if (strcmp(txt, "TPS") == 0)
    show_tps = checked;
  else if (strcmp(txt, "RPM") == 0)
    show_rpm = checked;
  else if (strcmp(txt, "Boost") == 0)
    show_boost = checked;
  else if (strcmp(txt, "TCU") == 0)
    show_tcu = checked;

  // Screen 2
  else if (strcmp(txt, "Oil Press") == 0)
    show_oil_press = checked;
  else if (strcmp(txt, "Oil Temp") == 0)
    show_oil_temp = checked;
  else if (strcmp(txt, "Water Temp") == 0)
    show_water_temp = checked;
  else if (strcmp(txt, "Fuel Press") == 0)
    show_fuel_press = checked;
  else if (strcmp(txt, "Battery") == 0)
    show_battery = checked;

  // Screen 4
  else if (strcmp(txt, "Pedal") == 0)
    show_pedal = checked;
  else if (strcmp(txt, "WG Pos") == 0)
    show_wg_pos = checked;
  else if (strcmp(txt, "BOV") == 0)
    show_bov = checked;
  else if (strcmp(txt, "TCU Req") == 0)
    show_tcu_req = checked;
  else if (strcmp(txt, "TCU Act") == 0)
    show_tcu_act = checked;
  else if (strcmp(txt, "Eng Req") == 0)
    show_eng_req = checked;

  // Screen 5
  else if (strcmp(txt, "Eng Act") == 0)
    show_eng_act = checked;
  else if (strcmp(txt, "Limit TQ") == 0)
    show_limit_tq = checked;

  settings_modified = 1;
  ESP_LOGI("SCREEN6", "Gauge %s toggled to: %s", txt, checked ? "ON" : "OFF");

  // Update settings immediately in memory
  system_settings_t *settings = system_settings_get();
  if (settings) {
    if (strcmp(txt, "MAP") == 0)
      settings->show_map = checked;
    else if (strcmp(txt, "Wastegate") == 0)
      settings->show_wastegate = checked;
    else if (strcmp(txt, "TPS") == 0)
      settings->show_tps = checked;
    else if (strcmp(txt, "RPM") == 0)
      settings->show_rpm = checked;
    else if (strcmp(txt, "Boost") == 0)
      settings->show_boost = checked;
    else if (strcmp(txt, "TCU") == 0)
      settings->show_tcu = checked;

    else if (strcmp(txt, "Oil Press") == 0)
      settings->show_oil_press = checked;
    else if (strcmp(txt, "Oil Temp") == 0)
      settings->show_oil_temp = checked;
    else if (strcmp(txt, "Water Temp") == 0)
      settings->show_water_temp = checked;
    else if (strcmp(txt, "Fuel Press") == 0)
      settings->show_fuel_press = checked;
    else if (strcmp(txt, "Battery") == 0)
      settings->show_battery = checked;

    else if (strcmp(txt, "Pedal") == 0)
      settings->show_pedal = checked;
    else if (strcmp(txt, "WG Pos") == 0)
      settings->show_wg_pos = checked;
    else if (strcmp(txt, "BOV") == 0)
      settings->show_bov = checked;
    else if (strcmp(txt, "TCU Req") == 0)
      settings->show_tcu_req = checked;
    else if (strcmp(txt, "TCU Act") == 0)
      settings->show_tcu_act = checked;
    else if (strcmp(txt, "Eng Req") == 0)
      settings->show_eng_req = checked;

    else if (strcmp(txt, "Eng Act") == 0)
      settings->show_eng_act = checked;
    else if (strcmp(txt, "Limit TQ") == 0)
      settings->show_limit_tq = checked;

    // Update ordered list
    gauge_id_t id = get_gauge_id_from_text(txt);
    if (checked) {
      add_gauge_to_list(settings, id);
    } else {
      remove_gauge_from_list(settings, id);
    }
  }

  // Force global layout update
  // We need to declare this function or include header
  extern void ui_update_global_layout(void);
  ui_update_global_layout();
}

// Platform checkbox event callback (Mutual Exclusion)
static void platform_checkbox_event_cb(lv_event_t *e) {
  lv_obj_t *cb = lv_event_get_target(e);
  const char *txt = lv_checkbox_get_text(cb);
  bool checked = lv_obj_has_state(cb, LV_STATE_CHECKED);

  // If unchecked, force reference back to checked (radio button behavior)
  if (!checked) {
    lv_obj_add_state(cb, LV_STATE_CHECKED);
    return;
  }

  // touch_settings_t *settings = &current_settings; // Removed direct access

  CanPlatform selected_platform = PLATFORM_VW_PQ35_46;
  if (strcmp(txt, "VW PQ35/46") == 0)
    selected_platform = PLATFORM_VW_PQ35_46;
  else if (strcmp(txt, "VW PQ25") == 0)
    selected_platform = PLATFORM_VW_PQ25;
  else if (strcmp(txt, "VW MQB") == 0)
    selected_platform = PLATFORM_VW_MQB;
  else if (strcmp(txt, "BMW Exx") == 0)
    selected_platform = PLATFORM_BMW_E9X;
  else if (strcmp(txt, "BMW E46") == 0)
    selected_platform = PLATFORM_BMW_E46;
  else if (strcmp(txt, "BMW Fxx") == 0)
    selected_platform = PLATFORM_BMW_F_SERIES;

  // Apply via setter (also updates parser)
  settings_set_can_platform(selected_platform);

  // Apply immediately to parser
  // #include "include/can_parser.h" // Moved to top of file
  can_parser_set_platform(selected_platform);

  settings_modified = 1;
  ESP_LOGI("SCREEN6", "Platform switched to: %s (%d)", txt, selected_platform);

  // Uncheck other checkboxes
  if (ui_Container_PlatformList) {
    uint32_t child_cnt = lv_obj_get_child_cnt(ui_Container_PlatformList);
    for (uint32_t i = 0; i < child_cnt; i++) {
      lv_obj_t *child = lv_obj_get_child(ui_Container_PlatformList, i);
      if (lv_obj_check_type(child, &lv_checkbox_class)) {
        if (child != cb) {
          lv_obj_clear_state(child, LV_STATE_CHECKED);
        }
      }
    }
  }
}

// Initialize Screen6
void ui_Screen6_screen_init(void) {
  ESP_LOGI("SCREEN6", "Starting Screen6 initialization");
  ui_Screen6 = lv_obj_create(NULL);
  lv_obj_set_size(ui_Screen6, 1280, 720);
  ESP_LOGI("SCREEN6", "ui_Screen6 created: %p", (void *)ui_Screen6);
  if (ui_Screen6 == NULL) {
    ESP_LOGE("SCREEN6", "FATAL: lv_obj_create(NULL) returned NULL!");
    return;
  }
  lv_obj_clear_flag(ui_Screen6, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(ui_Screen6, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(ui_Screen6, LV_OPA_COVER, 0);

  // Main title
  ui_Label_Device_Title = lv_label_create(ui_Screen6);
  lv_label_set_text((lv_obj_t *)ui_Label_Device_Title,
                    "Device Parameters Settings");
  lv_obj_set_style_text_color((lv_obj_t *)ui_Label_Device_Title,
                              lv_color_hex(0x00D4FF), 0);
  lv_obj_set_style_text_font((lv_obj_t *)ui_Label_Device_Title,
                             &lv_font_montserrat_24, 0);
  lv_obj_align((lv_obj_t *)ui_Label_Device_Title, LV_ALIGN_TOP_MID, 0, 20);

  // Demo Mode (Arcs) button - Top Left
  ui_Button_Demo_Mode = lv_btn_create(ui_Screen6);
  lv_obj_set_size((lv_obj_t *)ui_Button_Demo_Mode, 200, 50);
  lv_obj_align((lv_obj_t *)ui_Button_Demo_Mode, LV_ALIGN_TOP_LEFT, 50, 100);
  lv_obj_set_style_radius((lv_obj_t *)ui_Button_Demo_Mode, 25, 0);
  lv_obj_add_event_cb((lv_obj_t *)ui_Button_Demo_Mode, demo_mode_event_cb,
                      LV_EVENT_CLICKED, NULL);

  lv_obj_t *demo_label = lv_label_create((lv_obj_t *)ui_Button_Demo_Mode);
  lv_label_set_text(demo_label, "Demo Mode");
  lv_obj_set_style_text_color(demo_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(demo_label, &lv_font_montserrat_14, 0);
  lv_obj_center(demo_label);

  // Enable Screen3 button - Below Demo Mode
  ui_Button_Enable_Screen3 = lv_btn_create(ui_Screen6);
  lv_obj_set_size((lv_obj_t *)ui_Button_Enable_Screen3, 200, 50);
  lv_obj_align((lv_obj_t *)ui_Button_Enable_Screen3, LV_ALIGN_TOP_LEFT, 50,
               170);
  lv_obj_set_style_radius((lv_obj_t *)ui_Button_Enable_Screen3, 25, 0);
  lv_obj_add_event_cb((lv_obj_t *)ui_Button_Enable_Screen3,
                      enable_screen3_event_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *screen3_label =
      lv_label_create((lv_obj_t *)ui_Button_Enable_Screen3);
  lv_label_set_text(screen3_label, "Terminal Screen");
  lv_obj_set_style_text_color(screen3_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(screen3_label, &lv_font_montserrat_14, 0);
  lv_obj_center(screen3_label);

  // Nav Buttons Toggle - Below Screen3
  ui_Button_Nav_Buttons = lv_btn_create(ui_Screen6);
  lv_obj_set_size((lv_obj_t *)ui_Button_Nav_Buttons, 200, 50);
  lv_obj_align((lv_obj_t *)ui_Button_Nav_Buttons, LV_ALIGN_TOP_LEFT, 50, 240);
  lv_obj_set_style_radius((lv_obj_t *)ui_Button_Nav_Buttons, 25, 0);
  lv_obj_add_event_cb((lv_obj_t *)ui_Button_Nav_Buttons, nav_buttons_event_cb,
                      LV_EVENT_CLICKED, NULL);

  lv_obj_t *nav_btn_label = lv_label_create((lv_obj_t *)ui_Button_Nav_Buttons);
  lv_label_set_text(nav_btn_label, "Nav Buttons");
  lv_obj_set_style_text_color(nav_btn_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(nav_btn_label, &lv_font_montserrat_14, 0);
  lv_obj_center(nav_btn_label);

  // Save Settings button - Below Screen3
  ui_Button_Save_Settings = lv_btn_create(ui_Screen6);
  lv_obj_set_size((lv_obj_t *)ui_Button_Save_Settings, 200, 50);
  lv_obj_align((lv_obj_t *)ui_Button_Save_Settings, LV_ALIGN_TOP_LEFT, 50,
               380); // Moved down
  lv_obj_set_style_bg_color((lv_obj_t *)ui_Button_Save_Settings,
                            lv_color_hex(0x00FF88), 0);
  lv_obj_set_style_radius((lv_obj_t *)ui_Button_Save_Settings, 25, 0);
  lv_obj_add_event_cb((lv_obj_t *)ui_Button_Save_Settings,
                      save_settings_event_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *save_label = lv_label_create((lv_obj_t *)ui_Button_Save_Settings);
  lv_label_set_text(save_label, "SAVE SETTINGS");
  lv_obj_set_style_text_color(save_label, lv_color_black(), 0);
  lv_obj_set_style_text_font(save_label, &lv_font_montserrat_14, 0);
  lv_obj_center(save_label);

  // Gauge Selection Checkboxes - Right Side (Scrollable Container)
  ESP_LOGI("SCREEN6", "Before gauge_label, ui_Screen6=%p", (void *)ui_Screen6);
  lv_obj_t *gauge_label = lv_label_create(ui_Screen6);
  lv_label_set_text(gauge_label, "Visible Gauges:");
  lv_obj_set_style_text_color(gauge_label, lv_color_hex(0x00D4FF), 0);
  lv_obj_align(gauge_label, LV_ALIGN_TOP_RIGHT, -150, 60);

  // TEMPORARY: Skip GaugeList and PlatformList to debug crash
  ESP_LOGW("SCREEN6", "SKIPPING GaugeList and PlatformList for debugging");

  /* TEMPORARILY DISABLED - START
  ESP_LOGI("SCREEN6", "Before GaugeList, ui_Screen6=%p", (void *)ui_Screen6);
  ui_Container_GaugeList = lv_obj_create(ui_Screen6);
  lv_obj_set_size(ui_Container_GaugeList, 250, 350);
  lv_obj_align(ui_Container_GaugeList, LV_ALIGN_TOP_RIGHT, -10, 90);
  lv_obj_set_style_bg_color(ui_Container_GaugeList, lv_color_hex(0x1a1a1a), 0);
  lv_obj_set_style_border_width(ui_Container_GaugeList, 1, 0);
  lv_obj_set_style_border_color(ui_Container_GaugeList, lv_color_hex(0x333333),
                                0);
  lv_obj_set_flex_flow(ui_Container_GaugeList, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(ui_Container_GaugeList, 10, 0);
  lv_obj_set_style_pad_gap(ui_Container_GaugeList, 10, 0);

  const char *gauges[] = {
      "Screen 1:",  "MAP",        "Wastegate", "TPS",       "RPM",
      "Boost",      "TCU",        "Screen 2:", "Oil Press", "Oil Temp",
      "Water Temp", "Fuel Press", "Battery",   "Screen 4:", "Pedal",
      "WG Pos",     "BOV",        "TCU Req",   "TCU Act",   "Eng Req",
      "Screen 5:",  "Eng Act",    "Limit TQ"};

  for (int i = 0; i < 23; i++) {
    if (strchr(gauges[i], ':')) {
      // Section Header
      lv_obj_t *header = lv_label_create(ui_Container_GaugeList);
      lv_label_set_text(header, gauges[i]);

      lv_obj_set_style_text_color(header, lv_color_hex(0x00FF88), 0);
      lv_obj_set_style_text_font(header, &lv_font_montserrat_14, 0);
      lv_obj_set_style_pad_top(header, 10, 0);
    } else {
      // Checkbox
      lv_obj_t *cb = lv_checkbox_create(ui_Container_GaugeList);
      lv_checkbox_set_text(cb, gauges[i]);

      lv_obj_set_style_text_color(cb, lv_color_white(), 0);
      lv_obj_add_event_cb(cb, gauge_checkbox_event_cb, LV_EVENT_VALUE_CHANGED,
                          NULL);
    }
  }

  // Platform Selection - Center Logic
  //
  ----------------------------------------------------------------------------------
  lv_obj_t *platform_label = lv_label_create(ui_Screen6);
  lv_label_set_text(platform_label, "Vehicle Platform:");
  lv_obj_set_style_text_color(platform_label, lv_color_hex(0x00FF88), 0);
  lv_obj_align(platform_label, LV_ALIGN_TOP_MID, 0, 60);

  ui_Container_PlatformList = lv_obj_create(ui_Screen6);
  lv_obj_set_size(ui_Container_PlatformList, 220, 350);
  lv_obj_align(ui_Container_PlatformList, LV_ALIGN_TOP_MID, 0, 90);
  lv_obj_set_style_bg_color(ui_Container_PlatformList, lv_color_hex(0x1a1a1a),
                            0);
  lv_obj_set_style_border_width(ui_Container_PlatformList, 1, 0);
  lv_obj_set_style_border_color(ui_Container_PlatformList,
                                lv_color_hex(0x333333), 0);
  lv_obj_set_flex_flow(ui_Container_PlatformList, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(ui_Container_PlatformList, 10, 0);

  const char *platforms[] = {"VW PQ35/46", "VW PQ25", "VW MQB",
                             "BMW Exx",    "BMW E46", "BMW Fxx"};

  for (int i = 0; i < 6; i++) {
    lv_obj_t *cb = lv_checkbox_create(ui_Container_PlatformList);
    lv_checkbox_set_text(cb, platforms[i]);
    lv_obj_set_style_text_color(cb, lv_color_white(), 0);
    lv_obj_add_event_cb(cb, platform_checkbox_event_cb, LV_EVENT_VALUE_CHANGED,
                        NULL);
  }
  TEMPORARILY DISABLED - END */
  // ----------------------------------------------------------------------------------

  // Reset Settings button - Bottom Left
  ui_Button_Reset_Settings = lv_btn_create(ui_Screen6);
  lv_obj_set_size((lv_obj_t *)ui_Button_Reset_Settings, 200, 50);
  lv_obj_align((lv_obj_t *)ui_Button_Reset_Settings, LV_ALIGN_TOP_LEFT, 50,
               310);
  lv_obj_set_style_bg_color((lv_obj_t *)ui_Button_Reset_Settings,
                            lv_color_hex(0xFF3366), 0);
  lv_obj_set_style_radius((lv_obj_t *)ui_Button_Reset_Settings, 25, 0);
  lv_obj_add_event_cb((lv_obj_t *)ui_Button_Reset_Settings,
                      reset_settings_event_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *reset_label = lv_label_create((lv_obj_t *)ui_Button_Reset_Settings);
  lv_label_set_text(reset_label, "RESET SETTINGS");
  lv_obj_set_style_text_color(reset_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(reset_label, &lv_font_montserrat_14, 0);
  lv_obj_center(reset_label);

  // Initialize touch cursor
  ui_Touch_Cursor_Screen6 = lv_obj_create(ui_Screen6);
  lv_obj_set_size(ui_Touch_Cursor_Screen6, 30, 30);
  lv_obj_set_style_bg_color(ui_Touch_Cursor_Screen6, lv_color_hex(0x00D4FF), 0);
  lv_obj_set_style_radius(ui_Touch_Cursor_Screen6, 15, 0);
  lv_obj_add_flag(ui_Touch_Cursor_Screen6, LV_OBJ_FLAG_HIDDEN);

  // Add event handlers
  lv_obj_add_event_cb(ui_Screen6, screen6_touch_handler, LV_EVENT_PRESSED,
                      NULL);
  lv_obj_add_event_cb(ui_Screen6, screen6_touch_handler, LV_EVENT_RELEASED,
                      NULL);

  // Toggle AI Button (Top Right next to title)
  ui_Button_AI = lv_btn_create(ui_Screen6);
  lv_obj_set_size((lv_obj_t *)ui_Button_AI, 50, 40);
  lv_obj_align((lv_obj_t *)ui_Button_AI, LV_ALIGN_TOP_RIGHT, -20, 15);
  lv_obj_set_style_bg_color((lv_obj_t *)ui_Button_AI, lv_color_hex(0x00D4FF),
                            0);
  lv_obj_set_style_radius((lv_obj_t *)ui_Button_AI, 10, 0);
  lv_obj_add_event_cb((lv_obj_t *)ui_Button_AI, ai_button_event_cb,
                      LV_EVENT_CLICKED, NULL);

  lv_obj_t *ai_lbl = lv_label_create((lv_obj_t *)ui_Button_AI);
  lv_label_set_text(ai_lbl, "AI");
  lv_obj_center(ai_lbl);
  lv_obj_set_style_text_color(ai_lbl, lv_color_black(), 0);

  // Load saved settings and update button states
  ui_Screen6_load_settings();
  ui_Screen6_update_button_states();
}

// Destroy Screen6
void ui_Screen6_screen_destroy(void) {
  if (ui_Screen6) {
    lv_obj_del(ui_Screen6);
    ui_Screen6 = NULL;
  }
}

// Load Screen6 settings from the configuration system (which uses SD card)
void ui_Screen6_load_settings(void) {
  demo_mode_enabled = demo_mode_get_enabled();
  screen3_enabled = screen3_get_enabled();
  nav_buttons_enabled = nav_buttons_get_enabled();

  // Load gauge settings (assuming getters exist or adding them)
  system_settings_t *settings = system_settings_get();
  if (settings) {
    show_map = settings->show_map;
    show_wastegate = settings->show_wastegate;
    show_tps = settings->show_tps;
    show_rpm = settings->show_rpm;
    show_boost = settings->show_boost;
    show_tcu = settings->show_tcu;

    show_oil_press = settings->show_oil_press;
    show_oil_temp = settings->show_oil_temp;
    show_water_temp = settings->show_water_temp;
    show_fuel_press = settings->show_fuel_press;
    show_battery = settings->show_battery;

    show_pedal = settings->show_pedal;
    show_wg_pos = settings->show_wg_pos;
    show_bov = settings->show_bov;
    show_tcu_req = settings->show_tcu_req;
    show_tcu_act = settings->show_tcu_act;
    show_eng_req = settings->show_eng_req;

    show_eng_act = settings->show_eng_act;
    show_limit_tq = settings->show_limit_tq;

    // Legacy support: If count is 0 but booleans are true, populate list in
    // default order
    if (settings->active_gauge_count == 0) {
      if (show_map)
        add_gauge_to_list(settings, GAUGE_MAP);
      if (show_wastegate)
        add_gauge_to_list(settings, GAUGE_WASTEGATE);
      if (show_tps)
        add_gauge_to_list(settings, GAUGE_TPS);
      if (show_rpm)
        add_gauge_to_list(settings, GAUGE_RPM);
      if (show_boost)
        add_gauge_to_list(settings, GAUGE_BOOST);
      if (show_tcu)
        add_gauge_to_list(settings, GAUGE_TCU);

      if (show_oil_press)
        add_gauge_to_list(settings, GAUGE_OIL_PRESS);
      if (show_oil_temp)
        add_gauge_to_list(settings, GAUGE_OIL_TEMP);
      if (show_water_temp)
        add_gauge_to_list(settings, GAUGE_WATER_TEMP);
      if (show_fuel_press)
        add_gauge_to_list(settings, GAUGE_FUEL_PRESS);
      if (show_battery)
        add_gauge_to_list(settings, GAUGE_BATTERY);

      if (show_pedal)
        add_gauge_to_list(settings, GAUGE_PEDAL);
      if (show_wg_pos)
        add_gauge_to_list(settings, GAUGE_WG_POS);
      if (show_bov)
        add_gauge_to_list(settings, GAUGE_BOV);
      if (show_tcu_req)
        add_gauge_to_list(settings, GAUGE_TCU_REQ);
      if (show_tcu_act)
        add_gauge_to_list(settings, GAUGE_TCU_ACT);
      if (show_eng_req)
        add_gauge_to_list(settings, GAUGE_ENG_REQ);

      if (show_eng_act)
        add_gauge_to_list(settings, GAUGE_ENG_ACT);
      if (show_limit_tq)
        add_gauge_to_list(settings, GAUGE_LIMIT_TQ);
    }
  }

  ESP_LOGI("SCREEN6", "Loaded settings - Demo: %s, Screen3: %s",
           demo_mode_enabled ? "ON" : "OFF", screen3_enabled ? "ON" : "OFF");
}

// Save Screen6 settings to the SD Card via the configuration system
void ui_Screen6_save_settings(void) {
  demo_mode_set_enabled(demo_mode_enabled);
  screen3_set_enabled(screen3_enabled);
  nav_buttons_set_enabled(nav_buttons_enabled);

  system_settings_t *settings = system_settings_get();
  if (settings) {
    settings->show_map = show_map;
    settings->show_wastegate = show_wastegate;
    settings->show_tps = show_tps;
    settings->show_rpm = show_rpm;
    settings->show_boost = show_boost;
    settings->show_tcu = show_tcu;

    settings->show_oil_press = show_oil_press;
    settings->show_oil_temp = show_oil_temp;
    settings->show_water_temp = show_water_temp;
    settings->show_fuel_press = show_fuel_press;
    settings->show_battery = show_battery;

    settings->show_pedal = show_pedal;
    settings->show_wg_pos = show_wg_pos;
    settings->show_bov = show_bov;
    settings->show_tcu_req = show_tcu_req;
    settings->show_tcu_act = show_tcu_act;
    settings->show_eng_req = show_eng_req;

    settings->show_eng_act = show_eng_act;
    settings->show_limit_tq = show_limit_tq;

    // system_settings_save(settings); // Removed redundant call
  }

  trigger_settings_save(); // Use the non-blocking trigger

  ESP_LOGI("SCREEN6", "Triggered save settings - Demo: %s, Screen3: %s",
           demo_mode_enabled ? "ON" : "OFF", screen3_enabled ? "ON" : "OFF");
}

// Update button states based on loaded settings
void ui_Screen6_update_button_states(void) {
  lv_obj_t *btn;
  lv_obj_t *label;

  // Update Demo Mode button
  btn = (lv_obj_t *)ui_Button_Demo_Mode;
  if (btn) {
    label = lv_obj_get_child(btn, 0);
    if (demo_mode_enabled) {
      lv_obj_set_style_bg_color(btn, lv_color_hex(0x00FF88), 0);
      if (label)
        lv_label_set_text(label, "Demo Mode: ON");
    } else {
      lv_obj_set_style_bg_color(btn, lv_color_hex(0xFF3366), 0);
      if (label)
        lv_label_set_text(label, "Demo Mode: OFF");
    }
  }

  // Update Screen3 button
  btn = (lv_obj_t *)ui_Button_Enable_Screen3;
  if (btn) {
    label = lv_obj_get_child(btn, 0);
    if (screen3_enabled) {
      lv_obj_set_style_bg_color(btn, lv_color_hex(0x00FF88), 0);
      if (label)
        lv_label_set_text(label, "Screen3: ON");
    } else {
      lv_obj_set_style_bg_color(btn, lv_color_hex(0xFF3366), 0);
      if (label)
        lv_label_set_text(label, "Screen3: OFF");
    }
  }

  // Update Nav Buttons button
  btn = (lv_obj_t *)ui_Button_Nav_Buttons;
  if (btn) {
    label = lv_obj_get_child(btn, 0);
    if (nav_buttons_enabled) {
      lv_obj_set_style_bg_color(btn, lv_color_hex(0x00FF88), 0);
      if (label)
        lv_label_set_text(label, "Nav Buttons: ON");
    } else {
      lv_obj_set_style_bg_color(btn, lv_color_hex(0xFF3366), 0);
      if (label)
        lv_label_set_text(label, "Nav Buttons: OFF");
    }
  }

  // Update Gauge Checkboxes
  if (ui_Container_GaugeList) {
    uint32_t child_cnt = lv_obj_get_child_cnt(ui_Container_GaugeList);
    for (uint32_t i = 0; i < child_cnt; i++) {
      lv_obj_t *child = lv_obj_get_child(ui_Container_GaugeList, i);
      if (lv_obj_check_type(child, &lv_checkbox_class)) {
        // ... (Existing Gauge logic remains) ...
        const char *txt = lv_checkbox_get_text(child);
        bool should_check = false;

        if (strcmp(txt, "MAP") == 0)
          should_check = show_map;
        // ... (abbreviated for brevity, tool handles context) ...
        else if (strcmp(txt, "Limit TQ") == 0)
          should_check = show_limit_tq;
        else {
          // Fallback to check full list if abbreviated
          if (strcmp(txt, "Wastegate") == 0)
            should_check = show_wastegate;
          else if (strcmp(txt, "TPS") == 0)
            should_check = show_tps;
          else if (strcmp(txt, "RPM") == 0)
            should_check = show_rpm;
          else if (strcmp(txt, "Boost") == 0)
            should_check = show_boost;
          else if (strcmp(txt, "TCU") == 0)
            should_check = show_tcu;
          else if (strcmp(txt, "Oil Press") == 0)
            should_check = show_oil_press;
          else if (strcmp(txt, "Oil Temp") == 0)
            should_check = show_oil_temp;
          else if (strcmp(txt, "Water Temp") == 0)
            should_check = show_water_temp;
          else if (strcmp(txt, "Fuel Press") == 0)
            should_check = show_fuel_press;
          else if (strcmp(txt, "Battery") == 0)
            should_check = show_battery;
          else if (strcmp(txt, "Pedal") == 0)
            should_check = show_pedal;
          else if (strcmp(txt, "WG Pos") == 0)
            should_check = show_wg_pos;
          else if (strcmp(txt, "BOV") == 0)
            should_check = show_bov;
          else if (strcmp(txt, "TCU Req") == 0)
            should_check = show_tcu_req;
          else if (strcmp(txt, "TCU Act") == 0)
            should_check = show_tcu_act;
          else if (strcmp(txt, "Eng Req") == 0)
            should_check = show_eng_req;
          else if (strcmp(txt, "Eng Act") == 0)
            should_check = show_eng_act;
        }

        if (should_check)
          lv_obj_add_state(child, LV_STATE_CHECKED);
        else
          lv_obj_clear_state(child, LV_STATE_CHECKED);
      }
    }
  }

  // Update Platform Checkboxes
  if (ui_Container_PlatformList) {
    // Use getter
    CanPlatform current_plat = settings_get_can_platform();

    uint32_t child_cnt = lv_obj_get_child_cnt(ui_Container_PlatformList);
    for (uint32_t i = 0; i < child_cnt; i++) {
      lv_obj_t *child = lv_obj_get_child(ui_Container_PlatformList, i);
      if (lv_obj_check_type(child, &lv_checkbox_class)) {
        const char *txt = lv_checkbox_get_text(child);
        bool should_check = false;

        if (strcmp(txt, "VW PQ35/46") == 0 &&
            current_plat == PLATFORM_VW_PQ35_46)
          should_check = true;
        else if (strcmp(txt, "VW PQ25") == 0 &&
                 current_plat == PLATFORM_VW_PQ25)
          should_check = true;
        else if (strcmp(txt, "VW MQB") == 0 && current_plat == PLATFORM_VW_MQB)
          should_check = true;
        else if (strcmp(txt, "BMW Exx") == 0 &&
                 current_plat == PLATFORM_BMW_E9X)
          should_check = true;
        else if (strcmp(txt, "BMW E46") == 0 &&
                 current_plat == PLATFORM_BMW_E46)
          should_check = true;
        else if (strcmp(txt, "BMW Fxx") == 0 &&
                 current_plat == PLATFORM_BMW_F_SERIES)
          should_check = true;

        if (should_check)
          lv_obj_add_state(child, LV_STATE_CHECKED);
        else
          lv_obj_clear_state(child, LV_STATE_CHECKED);
      }
    }
  }
}

// Save device settings (wrapper)
void ui_save_device_settings(void) {
  ESP_LOGI("SCREEN6", "Saving device settings to SD Card");
  ui_Screen6_save_settings();
}

// =================================================================
// ИСПРАВЛЕННАЯ ФУНКЦИЯ / FIXED FUNCTION
// =================================================================
void ui_reset_device_settings(void) {
  ESP_LOGI("SCREEN6", "Resetting device settings to defaults");

  // 1. Устанавливаем локальные переменные в значения по умолчанию
  // Вместо вызова settings_reset_to_defaults(), который может вызывать
  // блокирующую запись, мы просто меняем состояние здесь и затем вызываем нашу
  // асинхронную функцию сохранения.
  demo_mode_enabled = DEFAULT_DEMO_MODE_ENABLED;
  screen3_enabled = DEFAULT_SCREEN3_ENABLED;
  nav_buttons_enabled = DEFAULT_NAV_BUTTONS_ENABLED;

  // 2. Обновляем внешний вид кнопок на экране, чтобы отразить сброс
  ui_Screen6_update_button_states();

  // 3. Сохраняем эти новые значения по умолчанию на SD-карту асинхронно
  // Эта функция уже вызывает trigger_settings_save() и не блокирует UI.
  ui_Screen6_save_settings();
}

// Update touch cursor position for Screen6
void ui_update_touch_cursor_screen6(void *point) {
  if (ui_Touch_Cursor_Screen6 && point) {
    lv_point_t *p = (lv_point_t *)point;
    lv_obj_set_pos((lv_obj_t *)ui_Touch_Cursor_Screen6, p->x - 15, p->y - 15);
    lv_obj_clear_flag((lv_obj_t *)ui_Touch_Cursor_Screen6, LV_OBJ_FLAG_HIDDEN);
  }
}
