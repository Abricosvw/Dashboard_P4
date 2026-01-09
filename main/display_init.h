#pragma once

#include "driver/i2c_master.h"
#include "driver/ledc.h"
#include "esp_lcd_ili9881c.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_ldo_regulator.h"
#include "sdkconfig.h"

// =============================================================================
// HARDWARE VARIANTS CONFIGURATION
// Configured via Kconfig (idf.py menuconfig)
// =============================================================================

// GPIO Pin Definitions
#define LCD_I2C_SCL_IO CONFIG_LCD_I2C_SCL_IO
#define LCD_I2C_SDA_IO CONFIG_LCD_I2C_SDA_IO
#define LCD_RST_IO CONFIG_LCD_RST_IO

// Touchscreen GPIOs
#define TOUCH_I2C_ADDR CONFIG_TOUCH_I2C_ADDR
#define TOUCH_INT_IO CONFIG_TOUCH_INT_IO
#define TOUCH_RST_IO CONFIG_TOUCH_RST_IO

// Factory Backlight I2C (controlled via 0x45 on DSI bus)
#define LCD_BK_I2C_ADDR CONFIG_LCD_BK_I2C_ADDR
#define LCD_BK_I2C_REG (0x96)
#define LCD_BK_I2C_REG_STAB (0x95)

// MIPI DSI PHY LDO Configuration (Channel 3: 2.5V)
#define LCD_MIPI_DSI_PHY_PWR_LDO_CHAN (3)
#define LCD_MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV (2500)

// Peripheral Power LDO Configuration (Channel 4: 3.3V)
#define LCD_PERIPH_PWR_LDO_CHAN (4)
#define LCD_PERIPH_PWR_LDO_VOLTAGE_MV (3300)

// LCD Resolution
#define LCD_PHYS_H_RES (720) // Physical panel: portrait
#define LCD_PHYS_V_RES (1280)
#define LCD_H_RES (1280) // Logical LVGL: landscape
#define LCD_V_RES (720)
#define LCD_BIT_PER_PIXEL (16) // RGB565

esp_err_t board_init_display(esp_lcd_panel_handle_t *panel_handle);
esp_err_t board_init_backlight(i2c_master_bus_handle_t bus_handle);
esp_err_t board_set_backlight(uint32_t level_percent);
esp_err_t board_init_touch(i2c_master_bus_handle_t bus_handle);
