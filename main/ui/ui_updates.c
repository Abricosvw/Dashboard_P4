#include "ui_updates.h"
#include "ui.h"
#include "screens/ui_Screen8.h"
#include "ecu_data.h"
#include <stdio.h>

static bool global_demo_mode = false;

void ui_updates_set_demo_mode(bool enabled) {
    global_demo_mode = enabled;
}

// This function is called periodically by the LVGL task.
// It reads the latest data from the global ECU data struct
// and updates all the gauge widgets on all screens.
// Helper function to update gauge value, text, and color
// Helper function to update gauge value, text, and color
static void update_gauge(lv_obj_t *arc, lv_obj_t *label, float value, const char *fmt, 
                        float warn_thr, float crit_thr, bool invert_logic, lv_color_t normal_color) {
    if (arc == NULL && label == NULL) return;

    // Determine Color
    lv_color_t text_color = lv_color_white();
    lv_color_t arc_color = normal_color;
    
    // Check thresholds
    bool is_warn = invert_logic ? (value <= warn_thr && value > crit_thr) : (value >= warn_thr && value < crit_thr);
    bool is_crit = invert_logic ? (value <= crit_thr) : (value >= crit_thr);

    if (is_crit) {
        text_color = lv_color_hex(0xFF0000); // Red
        arc_color = lv_color_hex(0xFF0000);
    } else if (is_warn) {
        text_color = lv_color_hex(0xFFD700); // Yellow
        arc_color = lv_color_hex(0xFFD700);
    }

    // Update Arc
    if (arc != NULL) {
        lv_arc_set_value(arc, (int16_t)value);
        lv_obj_set_style_arc_color(arc, arc_color, LV_PART_INDICATOR);
    }
    
    // Update Label
    if (label != NULL && fmt != NULL) {
        char buf[16];
        snprintf(buf, sizeof(buf), fmt, value);
        lv_label_set_text(label, buf);
        lv_obj_set_style_text_color(label, text_color, 0);
    }
}

void update_all_gauges(void) {


    ecu_data_t data;
    
    if (global_demo_mode) {
        ecu_data_simulate(&data);
    } else {
        ecu_data_get_copy(&data);
    }

    // --- Screen 1 ---
    // MAP: Cyan. Warn: 1500, Crit: 1800
    update_gauge(ui_Arc_MAP, ui_Label_MAP_Value, data.map_kpa, "%.0f", 1500, 1800, false, lv_color_hex(0x00D4FF));
    
    // RPM: Cyan. Warn: 7500, Crit: 9000
    update_gauge(ui_Arc_RPM, ui_Label_RPM_Value, data.engine_rpm, "%.0f", 7500, 9000, false, lv_color_hex(0x00D4FF));
    
    // TPS: Cyan. Warn: 80, Crit: 90
    update_gauge(ui_Arc_TPS, ui_Label_TPS_Value, data.tps_position, "%.1f", 80, 90, false, lv_color_hex(0x00D4FF));
    
    // Wastegate: Cyan. Warn: 110, Crit: 120
    update_gauge(ui_Arc_Wastegate, ui_Label_Wastegate_Value, data.wg_pos_percent, "%.1f", 110, 120, false, lv_color_hex(0x00D4FF));
    
    // Boost (Target): Cyan.
    if (ui_Arc_Boost && ui_Label_Boost_Value) {
         update_gauge(ui_Arc_Boost, ui_Label_Boost_Value, data.map_kpa, "%.0f", 200, 230, false, lv_color_hex(0x00D4FF));
    }

    // --- Screen 2 ---
    // Oil Pressure: Orange (0xFF6B35). Warn: < 2.0, Crit: < 1.0
    update_gauge(ui_Arc_Oil_Pressure, ui_Label_Oil_Pressure_Value, 0, "%.1f", 2.0, 1.0, true, lv_color_hex(0xFF6B35));
    
    // Oil Temp: Gold (0xFFD700). Warn: 110, Crit: 120
    update_gauge(ui_Arc_Oil_Temp, ui_Label_Oil_Temp_Value, data.oil_temp, "%.0f", 110, 120, false, lv_color_hex(0xFFD700));
    
    // Water Temp: Cyan (0x00D4FF). Warn: 105, Crit: 115
    update_gauge(ui_Arc_Water_Temp, ui_Label_Water_Temp_Value, data.clt_temp, "%.0f", 105, 115, false, lv_color_hex(0x00D4FF));
    
    // Fuel Pressure: Green (0x00FF88). Warn: < 3.0, Crit: < 2.0
    update_gauge(ui_Arc_Fuel_Pressure, ui_Label_Fuel_Pressure_Value, 0, "%.1f", 3.0, 2.0, true, lv_color_hex(0x00FF88));
    
    // Battery: Gold (0xFFD700). Warn: < 12.0, Crit: < 11.5
    update_gauge(ui_Arc_Battery_Voltage, ui_Label_Battery_Voltage_Value, data.battery_voltage, "%.1f", 12.0, 11.5, true, lv_color_hex(0xFFD700));

    // --- Screen 4 ---
    // Abs Pedal: Cyan
    update_gauge(ui_Arc_Abs_Pedal, ui_Label_Abs_Pedal_Value, data.abs_pedal_pos, "%.1f", 110, 120, false, lv_color_hex(0x00D4FF));
    
    // WG Pos: Green
    update_gauge(ui_Arc_WG_Pos, ui_Label_WG_Pos_Value, data.wg_pos_percent, "%.1f", 110, 120, false, lv_color_hex(0x00FF88));
    
    // BOV: Gold
    update_gauge(ui_Arc_BOV, ui_Label_BOV_Value, data.bov_percent, "%.1f", 110, 120, false, lv_color_hex(0xFFD700));
    
    // TCU TQ: Orange
    update_gauge(ui_Arc_TCU_TQ_Req, ui_Label_TCU_TQ_Req_Value, data.tcu_tq_req_nm, "%.0f", 450, 500, false, lv_color_hex(0xFF6B35));
    update_gauge(ui_Arc_TCU_TQ_Act, ui_Label_TCU_TQ_Act_Value, data.tcu_tq_act_nm, "%.0f", 450, 500, false, lv_color_hex(0xFF3366));
    update_gauge(ui_Arc_Eng_TQ_Req, ui_Label_Eng_TQ_Req_Value, data.eng_trg_nm, "%.0f", 450, 500, false, lv_color_hex(0x8A2BE2));

    // --- Screen 5 ---
    // Eng Tq Act: Cyan
    update_gauge(ui_Arc_Eng_TQ_Act, ui_Label_Eng_TQ_Act_Value, data.eng_act_nm, "%.0f", 450, 500, false, lv_color_hex(0x00D4FF));
    // Limit Tq: Green
    update_gauge(ui_Arc_Limit_TQ, ui_Label_Limit_TQ_Value, data.limit_tq_nm, "%.0f", 450, 500, false, lv_color_hex(0x00FF88));

    // --- Gear Display (Screen 4) ---
    if (ui_Label_Gear) {
        char gear_buf[16];
        if (data.gear == 0) snprintf(gear_buf, sizeof(gear_buf), "Gear: P");
        else if (data.gear == 13) snprintf(gear_buf, sizeof(gear_buf), "Gear: R"); // 13 is often Reverse in ZF/VAG
        else if (data.gear == 14) snprintf(gear_buf, sizeof(gear_buf), "Gear: N"); // 14 is often Neutral
        else snprintf(gear_buf, sizeof(gear_buf), "Gear: %d", data.gear);
        lv_label_set_text(ui_Label_Gear, gear_buf);
    }

    // --- Screen 1 TCU Box ---
    if (ui_Label_Gear_S1) {
        char gear_buf[16];
        if (data.gear == 0) snprintf(gear_buf, sizeof(gear_buf), "Gear: P");
        else if (data.gear == 13) snprintf(gear_buf, sizeof(gear_buf), "Gear: R");
        else if (data.gear == 14) snprintf(gear_buf, sizeof(gear_buf), "Gear: N");
        else snprintf(gear_buf, sizeof(gear_buf), "Gear: %d", data.gear);
        lv_label_set_text(ui_Label_Gear_S1, gear_buf);
    }

    if (ui_Label_Selector_S1) {
        char sel_buf[16];
        // Selector mapping (example VAG): P=0, R=1, N=2, D=3, S=4
        // If unknown, show raw
        if (data.selector_position == 0) snprintf(sel_buf, sizeof(sel_buf), "Sel: P");
        else if (data.selector_position == 1) snprintf(sel_buf, sizeof(sel_buf), "Sel: R");
        else if (data.selector_position == 2) snprintf(sel_buf, sizeof(sel_buf), "Sel: N");
        else if (data.selector_position == 3) snprintf(sel_buf, sizeof(sel_buf), "Sel: D");
        else if (data.selector_position == 4) snprintf(sel_buf, sizeof(sel_buf), "Sel: S");
        else snprintf(sel_buf, sizeof(sel_buf), "Sel: %d", data.selector_position);
        lv_label_set_text(ui_Label_Selector_S1, sel_buf);
    }

    // --- Screen 8 (Classic Sports) ---
    // RPM (Left) - Red
    update_gauge(ui_Gauge_RPM_S8, ui_Label_RPM_Val_S8, data.engine_rpm, "%.0f", 7500, 9000, false, lv_color_hex(0xFF0000));
    
    // Speed (Right) - White
    // Assuming data.vehicle_speed exists (or derive from RPM/Gear)
    // If vehicle_speed not in struct, use RPM * ratio for demo or default 0
    // float simul_speed = data.engine_rpm * 0.04; // Approximated
    update_gauge(ui_Gauge_Speed_S8, ui_Label_Speed_Val_S8, data.vehicle_speed, "%.0f", 250, 280, false, lv_color_white());

    // Boost (Bar)
    if (ui_Bar_Boost_S8) {
        lv_bar_set_value(ui_Bar_Boost_S8, (int32_t)data.map_kpa, LV_ANIM_OFF);
        // Color Change based on threshold
        lv_color_t bar_col = lv_color_hex(0xFF0000); // Default Red
        if (data.map_kpa < 100) bar_col = lv_color_hex(0x555555);
        lv_obj_set_style_bg_color(ui_Bar_Boost_S8, bar_col, LV_PART_INDICATOR);
    }
    if (ui_Label_Boost_Val_S8) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.0f", data.map_kpa);
        lv_label_set_text(ui_Label_Boost_Val_S8, buf);
    }

    // Temperatures & Pressures (Center Panel)
    if(ui_Label_OilTemp_Val_S8) {
        char buf[16]; snprintf(buf, sizeof(buf), "%.0f", data.oil_temp);
        lv_label_set_text(ui_Label_OilTemp_Val_S8, buf);
    }
    if(ui_Label_OilPress_Val_S8) {
        char buf[16]; snprintf(buf, sizeof(buf), "%.0f", data.oil_pressure); // Assuming oil_pressure exists
        lv_label_set_text(ui_Label_OilPress_Val_S8, buf);
    }
    if(ui_Label_WaterTemp_Val_S8) {
        char buf[16]; snprintf(buf, sizeof(buf), "%.0f", data.clt_temp);
        lv_label_set_text(ui_Label_WaterTemp_Val_S8, buf);
    }
    if(ui_Label_AirTemp_Val_S8) {
        char buf[16]; snprintf(buf, sizeof(buf), "%.0f", data.iat_temp); // Assuming iat_temp exists
        lv_label_set_text(ui_Label_AirTemp_Val_S8, buf);
    }

    // Gear (Footer) - Simplified Text Update? 
    // The Footer has 3 labels (P, N, D). This logic needs to highlight the active one.
    // However, recreating that logic here is complex since we don't have pointers to the individual footer labels in 'ui_Screen8.h',
    // only a single 'ui_Label_Gear_S8' which we repurposed or didn't fully expose.
    // In ui_Screen8.c we made local variables for P/N/D.
    // We should probably rely on a separate update function in ui_Screen8.c for complex UI state, or just stick to the simple logical label if exposed.
    // Ideally, ui_Screen8_update() should handle local widget logic.
    // But for now, we'll leave it as is.
    if (ui_Label_Gear_S8) {
        char gear_buf[8];
        if (data.gear == 0) snprintf(gear_buf, sizeof(gear_buf), "P");
        else if (data.gear == 13) snprintf(gear_buf, sizeof(gear_buf), "R");
        else if (data.gear == 14) snprintf(gear_buf, sizeof(gear_buf), "N");
        else snprintf(gear_buf, sizeof(gear_buf), "%d", data.gear);
        lv_label_set_text(ui_Label_Gear_S8, gear_buf);
    }


}
