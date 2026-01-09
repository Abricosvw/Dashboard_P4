#ifndef UI_LAYOUT_MANAGER_H
#define UI_LAYOUT_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdbool.h>

void ui_layout_manager_init(void);
void ui_update_global_layout(void);
bool ui_layout_is_screen_active(int screen_index);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // UI_LAYOUT_MANAGER_H
