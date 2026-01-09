// ECU Dashboard Screen 1 - Main Gauges
// First screen with 6 main ECU gauges
#ifndef UI_SCREEN1_H
#define UI_SCREEN1_H

#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// SCREEN: ui_Screen1
extern void ui_Screen1_screen_init(void);
extern void ui_Screen1_screen_destroy(void);
void ui_Screen1_screen_destroy(void);
void ui_Screen1_update_animations(bool demo_enabled);
void ui_Screen1_update_layout(void);
extern lv_obj_t *ui_Screen1;

extern lv_obj_t *ui_Arc_MAP;
extern lv_obj_t *ui_Arc_Wastegate;
extern lv_obj_t *ui_Arc_TPS;
extern lv_obj_t *ui_Label_RPM_Value;
extern lv_obj_t *ui_Label_Boost_Value;
extern lv_obj_t *ui_Label_Gear_S1;
extern lv_obj_t *ui_Label_Selector_S1;
extern lv_obj_t *ui_Arc_RPM;
extern lv_obj_t *ui_Arc_Boost;
// Intake Air Temp убран, возвращен TCU
extern lv_obj_t *ui_LED_TCU;
extern lv_obj_t *ui_Label_TCU_Status;

// Arc visibility control
void ui_Screen1_update_arc_visibility(int arc_index, bool visible);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
