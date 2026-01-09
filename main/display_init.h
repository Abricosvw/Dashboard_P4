#pragma once

#include "driver/i2c_master.h"
#include "driver/ledc.h"
#include "esp_lcd_ili9881c.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_ldo_regulator.h"

// =============================================================================
// HARDWARE VARIANTS CONFIGURATION
// VARIANT 1: ESP32-P4-WIFI6-DEV-KIT (Current Focus)
// VARIANT 2: ESP32-P4-Pico (Has CH422G IO Expander)
// =============================================================================

// GPIO Pin Definitions for VARIANT 1 (ESP32-P4-Module-DEV-KIT + 7-DSI-TOUCH-A)
#define LCD_I2C_SCL_IO (8)
#define LCD_I2C_SDA_IO (7)
#define LCD_RST_IO (33)      // Correct for Waveshare P4 DSI panels
#define LCD_BK_LIGHT_IO (26) // PWM pin for backlight on Dev-Kit
#define LCD_BK_LIGHT_ON_LEVEL (1)

// Touchscreen GPIOs (VARIANT 1)
#define TOUCH_INT_IO (4)
#define TOUCH_RST_IO (5)

// Factoy Backlight I2C (controlled via 0x45 on DSI bus)
#define LCD_BK_I2C_ADDR (0x45)
#define LCD_BK_I2C_REG (0x96)
#define LCD_BK_I2C_REG_STAB (0x95)

// MIPI DSI PHY LDO Configuration (Channel 3: 2.5V)
#define LCD_MIPI_DSI_PHY_PWR_LDO_CHAN (3)
#define LCD_MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV (2500)

// Peripheral Power LDO Configuration (Channel 4: 3.3V) - Required for I2C
// Expander
#define LCD_PERIPH_PWR_LDO_CHAN (4)
#define LCD_PERIPH_PWR_LDO_VOLTAGE_MV (3300)

// LCD Resolution - Landscape mode with PPA hardware rotation
#define LCD_PHYS_H_RES (720) // Physical panel: portrait
#define LCD_PHYS_V_RES (1280)
#define LCD_H_RES (1280) // Logical LVGL: landscape (PPA rotates to physical)
#define LCD_V_RES (720)  // Hardware rotation via PPA
#define LCD_BIT_PER_PIXEL (16) // RGB565
// #define LCD_BK_LIGHT_IO (32) // Already defined above

// Audio pins (for later)
#define AUDIO_I2C_ADDR (0x18)

esp_err_t board_init_display(esp_lcd_panel_handle_t *panel_handle);
esp_err_t board_init_backlight(i2c_port_t port);
esp_err_t board_set_backlight(uint32_t level_percent);
