#include "ui/ui_layout_manager.h"
#include "ecu_data.h"
#include "esp_log.h"
#include "ui/screens/ui_Screen4.h" // Need access to gauge pointers from other screens if not in ui.h
#include "ui/screens/ui_Screen5.h"
#include "ui/settings_config.h"
#include "ui/ui.h"

static const char *TAG = "UI_LAYOUT";

// Track which screens have at least one gauge
static bool screen_active_flags[8] = {false};

void ui_layout_manager_init(void) {
  ESP_LOGI(TAG, "Layout Manager Initialized");
  // Ensure all flags false initially
  for (int i = 0; i < 8; i++)
    screen_active_flags[i] = false;
  // Settings always active
  screen_active_flags[5] = true;
  ui_update_global_layout();
}

bool ui_layout_is_screen_active(int screen_index) {
  if (screen_index < 0 || screen_index >= 8)
    return false;
  return screen_active_flags[screen_index];
}

// Helper to hide gauge and its container
static void hide_gauge(lv_obj_t *obj) {
  if (!obj)
    return;
  lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t *parent = lv_obj_get_parent(obj);
  if (parent) {
    lv_obj_t *grandparent = lv_obj_get_parent(parent);
    if (grandparent) {
      lv_obj_add_flag(parent, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

// Helper to position gauge in a specific slot on a specific screen
static void place_gauge_on_screen(lv_obj_t *obj, lv_obj_t *target_screen,
                                  int slot_index) {
  if (!obj || !target_screen)
    return;

  lv_obj_t *container = lv_obj_get_parent(obj);
  if (!container)
    return;

  // 1. Reparent if necessary
  if (lv_obj_get_parent(container) != target_screen) {
    lv_obj_set_parent(container, target_screen);
    lv_obj_move_background(container); // Keep gauges behind navigation buttons
  }

  // 2. Calculate Position (Same 3x2 grid for all logical screens)
  // Grid Configuration for 1280x720
  // Row 1 (y=40): 60, 480, 900
  // Row 2 (y=380): 60, 480, 900

  int row = slot_index / 3;
  int col = slot_index % 3;

  int x = 60;
  if (col == 1)
    x = 480;
  if (col == 2)
    x = 900;

  int y = 40;
  if (row == 1)
    y = 380;

  // 3. Move Container
  lv_obj_set_pos(container, x, y);

  // 4. Ensure Visible
  lv_obj_clear_flag(container, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

// Definition of a Gauge Entry
typedef struct {
  lv_obj_t **obj_ptr; // Pointer to the global variable holding the object
  bool *enabled_ptr;  // Pointer to the settings bool
} gauge_entry_t;

void ui_update_global_layout(void) {
  system_settings_t *settings = system_settings_get();
  if (!settings)
    return;

  ESP_LOGI(TAG, "Reflowing global layout (Unified Flow)...");

  // Reset screen activity flags (Screens 1, 2, 4, 5 are dynamic)
  screen_active_flags[0] = false; // Screen 1
  screen_active_flags[1] = false; // Screen 2
  screen_active_flags[2] =
      false; // Screen 3 (Map - handled differently, but default to closed)
  screen_active_flags[3] = false; // Screen 4
  screen_active_flags[4] = false; // Screen 5
  screen_active_flags[6] = false; // Screen 7 (Game)
  screen_active_flags[7] = false; // Screen 8

  // Static screens (Settings/Game/Lux) might be handled elsewhere or always
  // active? User requested "flow", so we assume 1, 2, 4, 5 are the buckets.
  // Screen 3, 6, 7, 8 are special.
  screen_active_flags[5] = true; // Settings always active
  if (settings->screen3_enabled)
    screen_active_flags[2] = true;
  screen_active_flags[6] =
      true; // Game always available? Or driven by logic. Keeping true for now.
  screen_active_flags[7] = true; // Lux mode always available

  // Priority List of Gauges (The "Sequence")
  // Order: Screen 1 Defaults -> Screen 2 Defaults -> Screen 4 Defaults ->
  // Screen 5 Defaults
  gauge_entry_t gauges[] = {
      {&ui_Arc_MAP, &settings->show_map},
      {&ui_Arc_Wastegate, &settings->show_wastegate},
      {&ui_Arc_TPS, &settings->show_tps},
      {&ui_Arc_RPM, &settings->show_rpm},
      {&ui_Arc_Boost, &settings->show_boost},
      {&ui_LED_TCU,
       &settings->show_tcu}, // LED container behaves like gauge container

      {&ui_Arc_Oil_Pressure, &settings->show_oil_press},
      {&ui_Arc_Oil_Temp, &settings->show_oil_temp},
      {&ui_Arc_Water_Temp, &settings->show_water_temp},
      {&ui_Arc_Fuel_Pressure, &settings->show_fuel_press},
      {&ui_Arc_Battery_Voltage, &settings->show_battery},

      {&ui_Arc_Abs_Pedal, &settings->show_pedal},
      {&ui_Arc_WG_Pos, &settings->show_wg_pos},
      {&ui_Arc_BOV, &settings->show_bov},
      {&ui_Arc_TCU_TQ_Req, &settings->show_tcu_req},
      {&ui_Arc_TCU_TQ_Act, &settings->show_tcu_act},
      {&ui_Arc_Eng_TQ_Req, &settings->show_eng_req},

      {&ui_Arc_Eng_TQ_Act, &settings->show_eng_act},
      {&ui_Arc_Limit_TQ, &settings->show_limit_tq},
  };
  int total_gauges = sizeof(gauges) / sizeof(gauges[0]);

  // Available Screens for Flow
  lv_obj_t *screens[] = {ui_Screen1, ui_Screen2, ui_Screen4, ui_Screen5};
  int screen_indices[] = {0, 1, 3, 4}; // Map to screen_id_t
  int total_screens = 4;
  int slots_per_screen = 6;

  int current_screen_idx = 0;
  int current_slot = 0;

  for (int i = 0; i < total_gauges; i++) {
    lv_obj_t *obj = *gauges[i].obj_ptr;
    bool enabled = *gauges[i].enabled_ptr;

    if (enabled) {
      // Find a spot
      if (current_screen_idx < total_screens) {
        lv_obj_t *target_scr = screens[current_screen_idx];

        place_gauge_on_screen(obj, target_scr, current_slot);

        // Mark this screen as having content
        screen_active_flags[screen_indices[current_screen_idx]] = true;

        // Advance cursor
        current_slot++;
        if (current_slot >= slots_per_screen) {
          current_slot = 0;
          current_screen_idx++;
        }
      } else {
        // No more space on any screen? Hide it (or could dump on last screen)
        hide_gauge(obj);
      }
    } else {
      hide_gauge(obj);
    }
  }
}
