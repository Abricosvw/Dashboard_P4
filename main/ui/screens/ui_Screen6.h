// ECU Dashboard Screen 6 - Device Parameters Settings
// Sixth screen dedicated to device configuration and settings

#ifndef UI_SCREEN6_H
#define UI_SCREEN6_H

#include "lvgl.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// SCREEN: ui_Screen6
extern void ui_Screen6_screen_init(void);
extern void ui_Screen6_screen_destroy(void);
extern lv_obj_t *ui_Screen6;

// Device Parameters Settings Objects
extern void *ui_Label_Device_Title;
extern void *ui_Button_Demo_Mode;
extern void *ui_Button_Enable_Screen3;
extern void *ui_Button_Save_Settings;
extern void *ui_Button_Reset_Settings;

// Functions for device parameters settings
extern void ui_update_device_settings_display(void);
extern void ui_save_device_settings(void);
extern void ui_reset_device_settings(void);

// Screen6 utility functions
extern void ui_Screen6_load_settings(void);
extern void ui_Screen6_save_settings(void);
extern void ui_Screen6_update_button_states(void);

typedef struct {
  float cpu_freq_mhz;
  float cpu_load_percent;
  uint32_t task_count;
  size_t heap_total_bytes;
  size_t heap_free_bytes;
  size_t heap_min_free_bytes;
  size_t internal_total_bytes;
  size_t internal_free_bytes;
  size_t spiram_total_bytes;
  size_t spiram_free_bytes;
} ui_system_stats_t;

void ui_screen6_set_system_stats(const ui_system_stats_t *stats);
void ui_screen6_mark_stats_unavailable(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
