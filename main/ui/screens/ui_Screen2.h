// ECU Dashboard Screen 2 - Additional Gauges
// Second screen accessible by swiping left/right

#ifndef UI_SCREEN2_H
#define UI_SCREEN2_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdbool.h>

// SCREEN: ui_Screen2
extern void ui_Screen2_screen_init(void);
extern void ui_Screen2_screen_destroy(void);
void ui_Screen2_screen_destroy(void);
void ui_Screen2_update_animations(bool demo_enabled);
void ui_Screen2_update_layout(void);
extern lv_obj_t *ui_Screen2;

// Additional ECU Gauge Objects (5 датчиков - одинаковый размер с Screen1)
extern lv_obj_t *ui_Arc_Oil_Pressure;
extern lv_obj_t *ui_Arc_Oil_Temp;
extern lv_obj_t *ui_Arc_Water_Temp;
extern lv_obj_t *ui_Arc_Fuel_Pressure;
extern lv_obj_t *ui_Arc_Battery_Voltage; // Вернули Battery датчик

// Additional Label Objects
extern lv_obj_t *ui_Label_Oil_Pressure_Value;
extern lv_obj_t *ui_Label_Oil_Temp_Value;
extern lv_obj_t *ui_Label_Water_Temp_Value;
extern lv_obj_t *ui_Label_Fuel_Pressure_Value;
extern lv_obj_t *ui_Label_Battery_Voltage_Value; // Вернули Battery label

// Arc visibility control
void ui_Screen2_update_arc_visibility(int arc_index, bool visible);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
