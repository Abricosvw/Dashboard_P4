#ifndef _UI_SCREEN7_H
#define _UI_SCREEN7_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../ui.h"

extern lv_obj_t * ui_Screen7;

void ui_Screen7_screen_init(void);
void ui_Screen7_screen_destroy(void);
void ui_Screen7_update_layout(void); // For consistency, even if empty

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
