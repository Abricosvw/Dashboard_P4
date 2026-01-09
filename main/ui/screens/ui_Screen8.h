#ifndef UI_SCREEN8_H
#define UI_SCREEN8_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "../ui.h"

extern lv_obj_t *ui_Screen8;

// Gauge objects
extern lv_obj_t *ui_Gauge_RPM_S8;
extern lv_obj_t *ui_Gauge_Boost_S8;
extern lv_obj_t *ui_Gauge_Temp_S8;
extern lv_obj_t *ui_Gauge_Fuel_S8;

// Label objects
extern lv_obj_t *ui_Label_RPM_Val_S8;
extern lv_obj_t *ui_Label_Speed_Val_S8;
extern lv_obj_t *ui_Label_Gear_S8;

// New indicators for Classic Sports layout
extern lv_obj_t *ui_Gauge_Speed_S8;
extern lv_obj_t *ui_Label_Boost_Val_S8;
extern lv_obj_t *ui_Bar_Boost_S8;
extern lv_obj_t *ui_Label_OilTemp_Val_S8;
extern lv_obj_t *ui_Label_OilPress_Val_S8;
extern lv_obj_t *ui_Label_WaterTemp_Val_S8;
extern lv_obj_t *ui_Label_AirTemp_Val_S8;

void ui_Screen8_screen_init(void);
void ui_Screen8_screen_destroy(void);

// Update function
void ui_Screen8_update(void);
void ui_Screen8_update_animations(bool enabled);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
