// ECU Dashboard Screen 3 - Advanced CAN Bus Terminal
// Third screen dedicated to CAN Bus monitoring with advanced features

#include "ui_Screen3.h"
#include "../ui.h"
#include "can_logger.h" // Include logger
#include "can_manager.h"
#include "ui_Screen1.h"
#include "ui_Screen2.h"
#include "ui_events.h"
#include "ui_helpers.h"
#include "ui_screen_manager.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LV_FONT_DECLARE(lv_font_montserrat_10);
LV_FONT_DECLARE(lv_font_montserrat_12);
LV_FONT_DECLARE(lv_font_montserrat_20);

// Screen object
lv_obj_t *ui_Screen3;

// CAN Bus Terminal Objects
void *ui_Table_CAN_List;
void *ui_Label_CAN_Status;
void *ui_Label_CAN_Count;

// Advanced CAN Terminal Objects
void *ui_Button_Clear;
void *ui_Button_Sniffer;
void *ui_Button_Record; // New Record Button
void *ui_TextArea_Search;
void *ui_Slider_UpdateSpeed;
void *ui_TextArea_Search;
// void * ui_Slider_UpdateSpeed; // Removed
// void * ui_Label_UpdateSpeed; // Removed
void *ui_Panel_FilterList; // Container for ID checkboxes

// CAN Terminal state
static int can_message_count = 0;
static int can_sniffer_active = 1; // Sniffer mode active
static char search_text[64] = "";
// static int update_speed_ms = 100;    // Removed global update speed

// Sniffer state
static int last_can_id = 0;
static uint8_t last_can_data[8] = {0};
static uint8_t last_can_dlc = 0;

// Table state
#define MAX_CAN_IDS 100 // Increased limit

typedef struct {
  uint32_t id;
  int row_index;
  bool visible;
  lv_obj_t *checkbox;
  uint32_t last_update_time;
} can_row_t;

static can_row_t can_rows[MAX_CAN_IDS];
static int can_row_count = 0;

// ... (structs)

// ...

void ui_clear_can_terminal(void) {
  if (ui_Table_CAN_List) {
    // 1. Reset table to header only
    lv_table_set_row_cnt((lv_obj_t *)ui_Table_CAN_List, 1);

    // 2. Re-populate with visible rows
    int new_table_row = 1;
    for (int i = 0; i < can_row_count; i++) {
      if (can_rows[i].visible) {
        // Update index
        can_rows[i].row_index = new_table_row;

        // Set ID cell (Data cells will be empty until next update)
        char id_str[16];
        snprintf(id_str, sizeof(id_str), "%03X", (unsigned int)can_rows[i].id);
        lv_table_set_cell_value((lv_obj_t *)ui_Table_CAN_List, new_table_row, 0,
                                id_str);

        new_table_row++;
      } else {
        can_rows[i].row_index = -1;
      }
    }

    // 3. Do NOT reset message count (per user request)
    // can_message_count = 0;
    // lv_label_set_text((lv_obj_t*)ui_Label_CAN_Count, "Messages: 0");
  }
}

// Function prototypes
// Removed unused function declaration to fix compiler warning
// static void screen3_touch_handler(lv_event_t * e);

static void clear_button_event_cb(lv_event_t *e);
static void sniffer_button_event_cb(lv_event_t *e);
static void record_button_event_cb(lv_event_t *e); // New callback
static void search_text_event_cb(lv_event_t *e);
// static void update_speed_slider_event_cb(lv_event_t * e); // Removed
static void filter_checkbox_event_cb(lv_event_t *e);
// static int is_message_matches_search(const char* message);

// CAN Sniffer functions
// static void can_sniffer_add_message(uint32_t id, uint8_t *data, uint8_t dlc);
// // Removed unused declaration
// // Removed static declaration static void can_sniffer_format_message(char
// *buffer, size_t size, uint32_t id, uint8_t *data, uint8_t dlc);
static int can_sniffer_is_id_filtered(uint32_t id);
static int can_sniffer_search_in_data(uint8_t *data, uint8_t dlc,
                                      const char *search_term);
static void can_sniffer_update_statistics(uint32_t id);

// Clear button event callback
static void clear_button_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED) {
    ui_clear_can_terminal();
  }
}

// Auto-stop callback from logger
static void logger_stop_callback(void) {
  // Update button style back to idle (Green)
  extern bool example_lvgl_lock(int timeout_ms);
  extern void example_lvgl_unlock(void);

  if (example_lvgl_lock(100)) {
    if (ui_Button_Record) {
      lv_obj_set_style_bg_color((lv_obj_t *)ui_Button_Record,
                                lv_color_hex(0x00FF88), 0); // Green
      lv_obj_t *label = lv_obj_get_child((lv_obj_t *)ui_Button_Record, 0);
      if (label)
        lv_label_set_text(label, "REC");
    }
    example_lvgl_unlock();
  }
}

// Record button event callback
static void record_button_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED) {
    if (can_logger_is_recording()) {
      can_logger_stop();
      lv_obj_set_style_bg_color((lv_obj_t *)ui_Button_Record,
                                lv_color_hex(0x00FF88), 0); // Green (Idle)
    } else {
      can_logger_set_stop_callback(logger_stop_callback);
      can_logger_start();
      lv_obj_set_style_bg_color((lv_obj_t *)ui_Button_Record,
                                lv_color_hex(0xFF3366), 0); // Red (Recording)
    }
  }
}

// Sniffer button event callback
static void sniffer_button_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED) {
    lv_obj_t *btn = (lv_obj_t *)e->target;

    // Toggle sniffer state
    can_sniffer_active = !can_sniffer_active;
    ui_set_can_sniffer_active(can_sniffer_active);

    // Update button appearance
    if (can_sniffer_active) {
      lv_obj_set_style_bg_color(btn, lv_color_hex(0x00FF88),
                                0); // Green when active
      lv_obj_t *label = lv_obj_get_child(btn, 0);
      if (label) {
        lv_label_set_text(label, "SNIFFER: ON");
      }
    } else {
      lv_obj_set_style_bg_color(btn, lv_color_hex(0xFF3366),
                                0); // Red when inactive
      lv_obj_t *label = lv_obj_get_child(btn, 0);
      if (label) {
        lv_label_set_text(label, "SNIFFER: OFF");
      }
    }
  }
}

// Search text event callback
static void search_text_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_VALUE_CHANGED) {
    const char *text = lv_textarea_get_text((lv_obj_t *)ui_TextArea_Search);
    if (text) {
      snprintf(search_text, sizeof(search_text), "%s", text);
    }
  }
}

// Update speed slider event callback
/*
// Update speed slider event callback (Removed)
static void update_speed_slider_event_cb(lv_event_t * e) {
    // Removed
}
*/

// Filter checkbox event callback
static void filter_checkbox_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_VALUE_CHANGED) {
    lv_obj_t *cb = (lv_obj_t *)e->target;
    bool checked = lv_obj_has_state(cb, LV_STATE_CHECKED);
    uint32_t id = (uint32_t)(uintptr_t)lv_obj_get_user_data(cb);

    // Update visibility in tracker
    for (int i = 0; i < can_row_count; i++) {
      if (can_rows[i].id == id) {
        can_rows[i].visible = checked;
        break;
      }
    }
  }
}

/*
// Check if message matches search text (Search temporarily disabled for static
table) static int is_message_matches_search(const char* message) { if
(strlen(search_text) == 0) return 1; // No search text, show all

    // Case-insensitive search
    char message_lower[256];
    char search_lower[64];
    snprintf(message_lower, sizeof(message_lower), "%s", message);
    snprintf(search_lower, sizeof(search_lower), "%s", search_text);

    // Convert to lowercase
    for (int i = 0; message_lower[i]; i++) {
        if (message_lower[i] >= 'A' && message_lower[i] <= 'Z') {
            message_lower[i] = message_lower[i] + 32;
        }
    }
    for (int i = 0; search_lower[i]; i++) {
        if (search_lower[i] >= 'A' && search_lower[i] <= 'Z') {
            search_lower[i] = search_lower[i] + 32;
        }
    }

    return strstr(message_lower, search_lower) != NULL;
}
*/

// Initialize Screen3
void ui_Screen3_screen_init(void) {
  ui_Screen3 = lv_obj_create(NULL);
  lv_obj_set_size(ui_Screen3, 1280, 720);
  lv_obj_clear_flag(ui_Screen3, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(ui_Screen3, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(ui_Screen3, LV_OPA_COVER, 0);

  // Main terminal container (LEFT SIDE)
  lv_obj_t *terminal_cont = lv_obj_create(ui_Screen3);
  lv_obj_set_width(terminal_cont, 400); // reduced from 450
  lv_obj_set_height(terminal_cont, 410);
  lv_obj_set_x(terminal_cont, 5); // move slightly left
  lv_obj_set_y(terminal_cont, 60);
  lv_obj_set_align(terminal_cont, LV_ALIGN_TOP_LEFT);
  // lv_obj_clear_flag(terminal_cont, LV_OBJ_FLAG_SCROLLABLE); // Enable
  // scrolling
  lv_obj_set_style_bg_color(terminal_cont, lv_color_hex(0x00000000), 0);
  lv_obj_set_style_border_width(terminal_cont, 0, 0);
  lv_obj_set_style_pad_all(terminal_cont, 0, 0);

  // Terminal title (stays in place)
  lv_obj_t *terminal_title = lv_label_create(ui_Screen3);
  lv_label_set_text(terminal_title, "Advanced CAN Bus Terminal");
  lv_obj_set_style_text_color(terminal_title, lv_color_hex(0x00D4FF), 0);
  lv_obj_set_style_text_font(terminal_title, &lv_font_montserrat_20, 0);
  lv_obj_set_style_bg_color(terminal_title, lv_color_hex(0x2a2a2a), 0);
  lv_obj_align(terminal_title, LV_ALIGN_TOP_MID, 0, 10);

  // CAN Terminal Table (main display) - LEFT SIDE
  ui_Table_CAN_List = lv_table_create(terminal_cont);
  lv_obj_set_width((lv_obj_t *)ui_Table_CAN_List, 400); // match container
  lv_obj_set_height((lv_obj_t *)ui_Table_CAN_List,
                    LV_SIZE_CONTENT); // Allow scrolling
  lv_obj_set_pos((lv_obj_t *)ui_Table_CAN_List, 0, 0);

  // Style for the main table background (transparent)
  lv_obj_set_style_bg_color((lv_obj_t *)ui_Table_CAN_List,
                            lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa((lv_obj_t *)ui_Table_CAN_List, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width((lv_obj_t *)ui_Table_CAN_List, 0, 0);
  lv_obj_set_style_radius((lv_obj_t *)ui_Table_CAN_List, 0, 0);

  // Style for the cells (ITEMS)
  lv_obj_set_style_bg_color((lv_obj_t *)ui_Table_CAN_List,
                            lv_color_hex(0x000000), LV_PART_ITEMS);
  lv_obj_set_style_bg_opa((lv_obj_t *)ui_Table_CAN_List, LV_OPA_COVER,
                          LV_PART_ITEMS);
  lv_obj_set_style_text_color((lv_obj_t *)ui_Table_CAN_List,
                              lv_color_hex(0x00FF88), LV_PART_ITEMS);
  lv_obj_set_style_border_width((lv_obj_t *)ui_Table_CAN_List, 1,
                                LV_PART_ITEMS);
  lv_obj_set_style_border_color((lv_obj_t *)ui_Table_CAN_List,
                                lv_color_hex(0x333333), LV_PART_ITEMS);
  lv_obj_set_style_border_side((lv_obj_t *)ui_Table_CAN_List,
                               LV_BORDER_SIDE_BOTTOM,
                               LV_PART_ITEMS); // Only bottom border for rows

  lv_obj_set_style_text_font((lv_obj_t *)ui_Table_CAN_List,
                             &lv_font_montserrat_12, 0);

  // Configure table columns
  lv_table_set_col_cnt((lv_obj_t *)ui_Table_CAN_List, 4);
  lv_table_set_col_width((lv_obj_t *)ui_Table_CAN_List, 0, 60);  // ID
  lv_table_set_col_width((lv_obj_t *)ui_Table_CAN_List, 1, 40);  // DLC
  lv_table_set_col_width((lv_obj_t *)ui_Table_CAN_List, 2, 200); // DATA
  lv_table_set_col_width((lv_obj_t *)ui_Table_CAN_List, 3, 100); // ASCII

  // Set headers
  lv_table_set_cell_value((lv_obj_t *)ui_Table_CAN_List, 0, 0, "ID");
  lv_table_set_cell_value((lv_obj_t *)ui_Table_CAN_List, 0, 1, "DLC");
  lv_table_set_cell_value((lv_obj_t *)ui_Table_CAN_List, 0, 2, "DATA");
  lv_table_set_cell_value((lv_obj_t *)ui_Table_CAN_List, 0, 3, "ASCII");

  can_row_count = 0;

  // RIGHT SIDE PANEL - Controls and Status
  lv_obj_t *right_panel = lv_obj_create(ui_Screen3);
  lv_obj_set_width(right_panel, 300); // reduced from 320
  lv_obj_set_height(right_panel, 410);
  lv_obj_align(right_panel, LV_ALIGN_TOP_RIGHT, -10, 60);
  lv_obj_clear_flag(right_panel, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(right_panel, lv_color_hex(0x00000000), 0);
  lv_obj_set_style_border_width(right_panel, 0, 0);
  lv_obj_set_style_pad_all(right_panel, 5, 0);
  lv_obj_set_flex_flow(right_panel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(right_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(right_panel, 8, 0);

  // --- Top row: Status, Count, and Clear button ---
  lv_obj_t *top_row = lv_obj_create(right_panel);
  lv_obj_remove_style_all(top_row); // Remove default styles
  lv_obj_set_width(top_row, LV_PCT(100));
  lv_obj_set_height(top_row, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(top_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(top_row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  // CAN Status indicator
  ui_Label_CAN_Status = lv_label_create(top_row);
  lv_label_set_text((lv_obj_t *)ui_Label_CAN_Status, "● CAN: DISCONNECTED");
  lv_obj_set_style_text_color((lv_obj_t *)ui_Label_CAN_Status,
                              lv_color_hex(0xFF3366), 0);
  lv_obj_set_style_text_font((lv_obj_t *)ui_Label_CAN_Status,
                             &lv_font_montserrat_10, 0);

  // CAN Message counter
  ui_Label_CAN_Count = lv_label_create(top_row);
  lv_label_set_text((lv_obj_t *)ui_Label_CAN_Count, "Messages: 0");
  lv_obj_set_style_text_color((lv_obj_t *)ui_Label_CAN_Count,
                              lv_color_hex(0x00FF88), 0);
  lv_obj_set_style_text_font((lv_obj_t *)ui_Label_CAN_Count,
                             &lv_font_montserrat_14, 0);

  // Clear button (Moved to control row)

  // --- Control buttons row ---
  lv_obj_t *control_cont = lv_obj_create(right_panel);
  lv_obj_remove_style_all(control_cont);
  lv_obj_set_width(control_cont, LV_PCT(100));
  lv_obj_set_height(control_cont, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(control_cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_gap(control_cont, 5, 5);
  lv_obj_set_flex_align(control_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);

  // Sniffer button
  ui_Button_Sniffer = lv_btn_create(control_cont);
  lv_obj_set_size((lv_obj_t *)ui_Button_Sniffer, 120, 30);
  lv_obj_set_style_bg_color((lv_obj_t *)ui_Button_Sniffer,
                            lv_color_hex(0x00FF88),
                            0); // Start with green (active)
  lv_obj_set_style_radius((lv_obj_t *)ui_Button_Sniffer, 15, 0);
  lv_obj_add_event_cb((lv_obj_t *)ui_Button_Sniffer, sniffer_button_event_cb,
                      LV_EVENT_CLICKED, NULL);

  lv_obj_t *sniffer_label = lv_label_create((lv_obj_t *)ui_Button_Sniffer);
  lv_label_set_text(sniffer_label, "SNIFFER: ON");
  lv_obj_set_style_text_color(sniffer_label, lv_color_black(), 0);
  lv_obj_set_style_text_font(sniffer_label, &lv_font_montserrat_10, 0);
  lv_obj_center(sniffer_label);

  // Clear button (Moved here)
  ui_Button_Clear = lv_btn_create(control_cont);
  lv_obj_set_size((lv_obj_t *)ui_Button_Clear, 80, 30);
  lv_obj_set_style_bg_color((lv_obj_t *)ui_Button_Clear, lv_color_hex(0xFF3366),
                            0);
  lv_obj_set_style_radius((lv_obj_t *)ui_Button_Clear, 15, 0);
  lv_obj_add_event_cb((lv_obj_t *)ui_Button_Clear, clear_button_event_cb,
                      LV_EVENT_CLICKED, NULL);

  lv_obj_t *clear_label = lv_label_create((lv_obj_t *)ui_Button_Clear);
  lv_label_set_text(clear_label, "CLEAR");
  lv_obj_set_style_text_color(clear_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(clear_label, &lv_font_montserrat_12, 0);
  lv_obj_center(clear_label);

  // Record button
  ui_Button_Record = lv_btn_create(control_cont);
  lv_obj_set_size((lv_obj_t *)ui_Button_Record, 60, 30);
  lv_obj_set_style_bg_color((lv_obj_t *)ui_Button_Record,
                            lv_color_hex(0x00FF88), 0); // Green (Idle)
  lv_obj_set_style_radius((lv_obj_t *)ui_Button_Record, 15, 0);
  lv_obj_add_event_cb((lv_obj_t *)ui_Button_Record, record_button_event_cb,
                      LV_EVENT_CLICKED, NULL);

  lv_obj_t *rec_label = lv_label_create((lv_obj_t *)ui_Button_Record);
  lv_label_set_text(rec_label, "REC");
  lv_obj_set_style_text_color(rec_label, lv_color_black(), 0);
  lv_obj_set_style_text_font(rec_label, &lv_font_montserrat_12, 0);
  lv_obj_center(rec_label);

  // --- Search row ---
  lv_obj_t *search_cont = lv_obj_create(right_panel);
  lv_obj_remove_style_all(search_cont);
  lv_obj_set_width(search_cont, LV_PCT(100));
  lv_obj_set_height(search_cont, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(search_cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(search_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(search_cont, 10, 0);
  lv_obj_set_style_bg_color(search_cont, lv_color_hex(0x1a1a1a), 0);
  lv_obj_set_style_pad_all(search_cont, 5, 0);
  lv_obj_set_style_radius(search_cont, 8, 0);

  lv_obj_t *search_label = lv_label_create(search_cont);
  lv_label_set_text(search_label, "Search:");
  lv_obj_set_style_text_color(search_label, lv_color_hex(0x00D4FF), 0);
  lv_obj_set_style_text_font(search_label, &lv_font_montserrat_14, 0);

  ui_TextArea_Search = lv_textarea_create(search_cont);
  lv_obj_set_flex_grow((lv_obj_t *)ui_TextArea_Search, 1);
  lv_obj_set_height((lv_obj_t *)ui_TextArea_Search, 35);
  lv_textarea_set_one_line((lv_obj_t *)ui_TextArea_Search, true);
  lv_obj_set_style_bg_color((lv_obj_t *)ui_TextArea_Search,
                            lv_color_hex(0x333333), 0);
  lv_obj_set_style_text_color((lv_obj_t *)ui_TextArea_Search, lv_color_white(),
                              0);
  lv_obj_set_style_border_width((lv_obj_t *)ui_TextArea_Search, 0, 0);
  lv_obj_set_style_radius((lv_obj_t *)ui_TextArea_Search, 5, 0);
  lv_obj_set_style_text_font((lv_obj_t *)ui_TextArea_Search,
                             &lv_font_montserrat_12, 0);
  lv_textarea_set_placeholder_text((lv_obj_t *)ui_TextArea_Search,
                                   "Enter search text...");
  lv_obj_add_event_cb((lv_obj_t *)ui_TextArea_Search, search_text_event_cb,
                      LV_EVENT_VALUE_CHANGED, NULL);

  /*
  // --- Update speed slider (Removed) ---
  // ... code removed ...
  */

  // --- Filter List Container (Bottom Right) ---
  lv_obj_t *filter_label = lv_label_create(right_panel);
  lv_label_set_text(filter_label, "Filter IDs:");
  lv_obj_set_style_text_color(filter_label, lv_color_hex(0x00D4FF), 0);
  lv_obj_set_style_text_font(filter_label, &lv_font_montserrat_12, 0);
  lv_obj_align(filter_label, LV_ALIGN_LEFT_MID, 0, 0);

  ui_Panel_FilterList = lv_obj_create(right_panel);
  lv_obj_set_width((lv_obj_t *)ui_Panel_FilterList, LV_PCT(100));
  lv_obj_set_flex_grow((lv_obj_t *)ui_Panel_FilterList,
                       1); // Fill remaining space
  lv_obj_set_style_bg_color((lv_obj_t *)ui_Panel_FilterList,
                            lv_color_hex(0x1a1a1a), 0);
  lv_obj_set_style_border_width((lv_obj_t *)ui_Panel_FilterList, 0, 0);
  lv_obj_set_style_pad_all((lv_obj_t *)ui_Panel_FilterList, 5, 0);
  lv_obj_set_flex_flow((lv_obj_t *)ui_Panel_FilterList, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_gap((lv_obj_t *)ui_Panel_FilterList, 5, 0);
  lv_obj_set_style_radius((lv_obj_t *)ui_Panel_FilterList, 5, 0);
}

// Destroy Screen3
void ui_Screen3_screen_destroy(void) { lv_obj_del(ui_Screen3); }

// Add/Update CAN message in table
void ui_add_can_message(uint32_t id, uint8_t *data, uint8_t dlc) {
  if (!ui_Table_CAN_List)
    return;

  // Check search text (if needed, but user wants static rows, so maybe just
  // highlight?) For now, we'll filter out if it doesn't match search Note: We
  // need to convert data to string to check search, which is expensive. Let's
  // skip search for now or implement it later if requested.

  int row_idx = -1;

  // Find existing row
  for (int i = 0; i < can_row_count; i++) {
    if (can_rows[i].id == id) {
      // Check if ID is filtered out
      if (!can_rows[i].visible)
        return;

      // Check if ID is in the table (row_index != -1)
      if (can_rows[i].row_index != -1) {
        row_idx = can_rows[i].row_index;

        // Per-row rate limiting (50ms)
        uint32_t now = lv_tick_get();
        if (now - can_rows[i].last_update_time < 50) {
          return; // Too fast, skip update
        }
        can_rows[i].last_update_time = now;
      } else {
        // ID exists but was cleared from table. Re-add it.
        uint32_t table_row_count =
            lv_table_get_row_cnt((lv_obj_t *)ui_Table_CAN_List);
        row_idx = table_row_count;
        can_rows[i].row_index = row_idx;
        can_rows[i].last_update_time = lv_tick_get();

        // Set ID cell
        char id_str[16];
        snprintf(id_str, sizeof(id_str), "%03X",
                 (unsigned int)id); // Format: 101
        lv_table_set_cell_value((lv_obj_t *)ui_Table_CAN_List, row_idx, 0,
                                id_str);
      }
      break;
    }
  }

  // If not found (New ID), add new row
  if (row_idx == -1) {
    // Only add if we haven't reached max tracked IDs
    if (can_row_count >= MAX_CAN_IDS) {
      return; // Tracker full
    }

    // Add to tracker
    can_rows[can_row_count].id = id;
    can_rows[can_row_count].visible = true; // Visible by default
    can_rows[can_row_count].last_update_time = lv_tick_get();

    // Add to table
    uint32_t table_row_count =
        lv_table_get_row_cnt((lv_obj_t *)ui_Table_CAN_List);
    row_idx = table_row_count;
    can_rows[can_row_count].row_index = row_idx;

    // Create checkbox in filter list
    if (ui_Panel_FilterList) {
      lv_obj_t *cb = lv_checkbox_create((lv_obj_t *)ui_Panel_FilterList);
      char cb_text[16];
      snprintf(cb_text, sizeof(cb_text), "%03X",
               (unsigned int)id); // Format: 101
      lv_checkbox_set_text(cb, cb_text);
      lv_obj_add_state(cb, LV_STATE_CHECKED);
      lv_obj_set_style_text_color(cb, lv_color_white(), 0);
      lv_obj_set_style_text_font(cb, &lv_font_montserrat_12, 0);
      lv_obj_add_event_cb(cb, filter_checkbox_event_cb, LV_EVENT_VALUE_CHANGED,
                          NULL);
      lv_obj_set_user_data(cb, (void *)(uintptr_t)id); // Store ID in user data
      can_rows[can_row_count].checkbox = cb;
    }

    can_row_count++;

    // Set ID cell once
    char id_str[16];
    snprintf(id_str, sizeof(id_str), "%03X", (unsigned int)id); // Format: 101
    lv_table_set_cell_value((lv_obj_t *)ui_Table_CAN_List, row_idx, 0, id_str);
  }

  // Check visibility
  bool is_visible = true;
  for (int i = 0; i < can_row_count; i++) {
    if (can_rows[i].id == id) {
      is_visible = can_rows[i].visible;
      break;
    }
  }

  if (!is_visible)
    return; // Skip update if filtered out

  // Update DLC
  char dlc_str[4];
  snprintf(dlc_str, sizeof(dlc_str), "%d", dlc);
  lv_table_set_cell_value((lv_obj_t *)ui_Table_CAN_List, row_idx, 1, dlc_str);

  // Update Data
  char data_hex_str[32] = {0};
  for (int i = 0; i < dlc && i < 8; i++) {
    char byte_str[4];
    snprintf(byte_str, sizeof(byte_str), "%02X ", data[i]);
    strncat(data_hex_str, byte_str,
            sizeof(data_hex_str) - strlen(data_hex_str) - 1);
  }
  lv_table_set_cell_value((lv_obj_t *)ui_Table_CAN_List, row_idx, 2,
                          data_hex_str);

  // Update ASCII
  char data_ascii_str[9] = {0};
  for (int i = 0; i < dlc && i < 8; i++) {
    if (data[i] >= 32 && data[i] <= 126) {
      data_ascii_str[i] = data[i];
    } else {
      data_ascii_str[i] = '.';
    }
  }
  data_ascii_str[dlc] = '\0';
  lv_table_set_cell_value((lv_obj_t *)ui_Table_CAN_List, row_idx, 3,
                          data_ascii_str);

  // Update message count (total)
  can_message_count++;
  char count_text[32];
  snprintf(count_text, sizeof(count_text), "Messages: %d", can_message_count);
  lv_label_set_text((lv_obj_t *)ui_Label_CAN_Count, count_text);
}

// Update CAN status and message count
void ui_update_can_status(int connected, int message_count) {
  if (ui_Label_CAN_Status) {
    if (connected) {
      lv_label_set_text((lv_obj_t *)ui_Label_CAN_Status, "● CAN: CONNECTED");
      lv_obj_set_style_text_color((lv_obj_t *)ui_Label_CAN_Status,
                                  lv_color_hex(0x00FF88), 0);
    } else {
      lv_label_set_text((lv_obj_t *)ui_Label_CAN_Status, "● CAN: DISCONNECTED");
      lv_obj_set_style_text_color((lv_obj_t *)ui_Label_CAN_Status,
                                  lv_color_hex(0xFF3366), 0);
    }
  }

  if (ui_Label_CAN_Count) {
    char count_text[32];
    snprintf(count_text, sizeof(count_text), "Messages: %d", message_count);
    lv_label_set_text((lv_obj_t *)ui_Label_CAN_Count, count_text);
  }
}

// Old ui_clear_can_terminal removed

// Set search text
void ui_set_search_text(const char *search_text_input) {
  if (search_text_input) {
    snprintf(search_text, sizeof(search_text), "%s", search_text_input);

    if (ui_TextArea_Search) {
      lv_textarea_set_text((lv_obj_t *)ui_TextArea_Search, search_text);
    }
  }
}

// Set update speed (Removed)
void ui_set_update_speed(int speed_ms) {
  // Removed
}

// Update CAN statistics display (statistics no longer displayed)
void ui_update_can_statistics(void) {
  // Statistics display removed - function kept for compatibility
}

// Reset CAN statistics (statistics no longer displayed)
void ui_reset_can_statistics(void) {
  // Statistics display removed - function kept for compatibility
}

// ============================================================================
// CAN SNIFFER FUNCTIONS
// ============================================================================

// Add CAN message to terminal with filtering and search
// Process CAN message for terminal and logger (Called from CAN task)
// Add CAN message to terminal with filtering and search
// Process CAN message for terminal and logger (Called from CAN task)
void ui_process_real_can_message(uint32_t id, uint8_t *data, uint8_t dlc) {
  if (!can_sniffer_active)
    return;

  // Check if ID should be filtered
  if (!can_sniffer_is_id_filtered(id))
    return;

  // Log to SD card if recording
  // Cast const away since can_logger_log might not take const yet (check logger
  // header) Assuming can_logger_log takes (uint32_t, uint8_t*, uint8_t)
  can_logger_log(id, (uint8_t *)data, dlc);

  // Check if search term matches
  if (!can_sniffer_search_in_data((uint8_t *)data, dlc, search_text))
    return;

  // Add to terminal (Table) - ui_add_can_message takes uint8_t*
  ui_add_can_message(id, (uint8_t *)data, dlc);

  // Update statistics
  can_sniffer_update_statistics(id);

  // Store last message for debugging
  last_can_id = id;
  last_can_dlc = dlc;
  memcpy(last_can_data, data, dlc > 8 ? 8 : dlc);
}

/*
// Format CAN message for display (Unused in table view)
static void can_sniffer_format_message(char *buffer, size_t size, uint32_t id,
uint8_t *data, uint8_t dlc)
{
    // Timestamp
    char timestamp_str[16];
    uint32_t tick = lv_tick_get();
    snprintf(timestamp_str, sizeof(timestamp_str), "%lu.%03lu", tick / 1000,
(tick % 1000));

    // HEX Data
    char data_hex_str[3 * 8 + 1] = {0};
    for (int i = 0; i < dlc && i < 8; i++) {
        char byte_str[4];
        snprintf(byte_str, sizeof(byte_str), "%02X ", data[i]);
        strncat(data_hex_str, byte_str, sizeof(data_hex_str) -
strlen(data_hex_str) - 1);
    }

    // ASCII Data
    char data_ascii_str[9] = {0};
    for (int i = 0; i < dlc && i < 8; i++) {
        if (data[i] >= 32 && data[i] <= 126) { // Printable ASCII
            data_ascii_str[i] = data[i];
        } else {
            data_ascii_str[i] = '.'; // Non-printable
        }
    }
    data_ascii_str[dlc] = '\0';

    // DBC Comments (placeholder)
    const char* dbc_comment = ""; // Placeholder for future DBC implementation

    // Final formatted string
    snprintf(buffer, size, "%-12s | %-3X | %-1d | %-24s | %-8s | %s",
             timestamp_str,
             (unsigned int)id,
             dlc,
             data_hex_str,
             data_ascii_str,
             dbc_comment);
}
*/

// Check if CAN ID should be shown (always show all messages now)
static int can_sniffer_is_id_filtered(uint32_t id) {
  return 1; // Show all messages
}

// Search for text in CAN data (supports hex values and ASCII)
static int can_sniffer_search_in_data(uint8_t *data, uint8_t dlc,
                                      const char *search_term) {
  if (!search_term || strlen(search_term) == 0)
    return 1; // No search term, show all

  char search_lower[64];
  snprintf(search_lower, sizeof(search_lower), "%s", search_term);

  // Convert search term to lowercase
  for (int i = 0; search_lower[i]; i++) {
    if (search_lower[i] >= 'A' && search_lower[i] <= 'Z') {
      search_lower[i] = search_lower[i] + 32;
    }
  }

  // Search for hex values (e.g., "AA", "FF")
  if (strlen(search_lower) == 2) {
    uint8_t search_byte;
    if (sscanf(search_lower, "%2hhx", &search_byte) == 1) {
      for (int i = 0; i < dlc && i < 8; i++) {
        if (data[i] == search_byte)
          return 1;
      }
    }
  }

  // Search for longer hex sequences (e.g., "AABB", "FF0011")
  if (strlen(search_lower) > 2 && strlen(search_lower) % 2 == 0) {
    uint8_t search_bytes[8];
    int num_bytes = strlen(search_lower) / 2;
    if (num_bytes <= 8) {
      for (int i = 0; i < num_bytes; i++) {
        if (sscanf(&search_lower[i * 2], "%2hhx", &search_bytes[i]) != 1) {
          break; // Invalid hex, try text search
        }
      }
      // Check if data contains this sequence
      for (int i = 0; i <= dlc - num_bytes; i++) {
        if (memcmp(&data[i], search_bytes, num_bytes) == 0)
          return 1;
      }
    }
  }

  // Search for ASCII text in data (simple approach)
  char data_ascii[9] = "";
  for (int i = 0; i < dlc && i < 8; i++) {
    if (data[i] >= 32 && data[i] <= 126) { // Printable ASCII
      data_ascii[i] = data[i];
    } else {
      data_ascii[i] = '.'; // Non-printable
    }
  }

  // Convert to lowercase for comparison
  for (int i = 0; data_ascii[i]; i++) {
    if (data_ascii[i] >= 'A' && data_ascii[i] <= 'Z') {
      data_ascii[i] = data_ascii[i] + 32;
    }
  }

  return strstr(data_ascii, search_lower) != NULL;
}

// Update statistics counters (statistics removed)
static void can_sniffer_update_statistics(uint32_t id) {
  // Statistics display removed - function simplified
}

// Enable/disable CAN sniffer
void ui_set_can_sniffer_active(int active) {
  can_sniffer_active = active;
  if (active) {
    // Clear terminal when enabling
    ui_clear_can_terminal();
  }
}

// Get CAN sniffer active state
int ui_get_can_sniffer_active(void) { return can_sniffer_active; }

// Get last CAN message for debugging
void ui_get_last_can_message(uint32_t *id, uint8_t *data, uint8_t *dlc) {
  if (id)
    *id = last_can_id;
  if (data)
    memcpy(data, last_can_data, 8);
  if (dlc)
    *dlc = last_can_dlc;
}

// Duplicate function removed
