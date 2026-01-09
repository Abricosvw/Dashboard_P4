#ifndef UI_UPDATES_H
#define UI_UPDATES_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// This function is called periodically by the LVGL task.
// It reads the latest data from the global ECU data struct
// and updates all the gauge widgets on all screens.
void update_all_gauges(void);
void ui_updates_set_demo_mode(bool enabled);

#ifdef __cplusplus
}
#endif

#endif // UI_UPDATES_H
