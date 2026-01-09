// ECU Dashboard Screen 5 - ECU Data Gauges (Page 2)
#ifndef UI_SCREEN5_H
#define UI_SCREEN5_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

void ui_Screen5_screen_init(void);
void ui_Screen5_screen_destroy(void);
void ui_Screen5_screen_destroy(void);
void ui_Screen5_update_animations(bool demo_enabled);
void ui_Screen5_update_layout(void);
extern lv_obj_t *ui_Screen5;

extern lv_obj_t *ui_Arc_Eng_TQ_Act;
extern lv_obj_t *ui_Label_Eng_TQ_Act_Value;

extern lv_obj_t *ui_Arc_Limit_TQ;
extern lv_obj_t *ui_Label_Limit_TQ_Value;

// Arc visibility control
void ui_Screen5_update_arc_visibility(int arc_index, bool visible);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
