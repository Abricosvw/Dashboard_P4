// ECU Dashboard Screen 3 - CAN Bus Terminal
// Third screen dedicated to CAN Bus monitoring

#ifndef UI_SCREEN3_H
#define UI_SCREEN3_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdint.h>

// SCREEN: ui_Screen3
extern void ui_Screen3_screen_init(void);
extern void ui_Screen3_screen_destroy(void);
extern lv_obj_t *ui_Screen3;

// CAN Bus Terminal Objects
extern void *ui_Table_CAN_List;
extern void *ui_Label_CAN_Status;
extern void *ui_Label_CAN_Count;

// Functions for updating CAN terminal
extern void ui_add_can_message(uint32_t id, uint8_t *data, uint8_t dlc);
extern void ui_update_can_status(int connected, int message_count);
extern void ui_clear_can_terminal(void);
extern void ui_set_search_text(const char *search_text);
extern void ui_set_update_speed(int speed_ms);

// CAN Sniffer functions
extern void ui_set_can_sniffer_active(int active);
extern int ui_get_can_sniffer_active(void);
extern void ui_get_last_can_message(uint32_t *id, uint8_t *data, uint8_t *dlc);
extern void ui_process_real_can_message(uint32_t id, uint8_t *data,
                                        uint8_t dlc);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
