file(GLOB_RECURSE UI_SOURCES "ui/*.c")

idf_component_register(SRCS "main.c" "board_init.c" "main_gui.c" "can_manager.c" "can_websocket.c" "wifi_init.c" "wifi_controller.c" "sd_card_manager.c" "web_server.c" "settings_manager.c" "audio_manager.c" "ecu_data.c" "can_parser.c" "can_logger.c" "background_task.c" "ai_manager.c" ${UI_SOURCES}
                       INCLUDE_DIRS "." "ui" "include"
                       REQUIRES esp_lcd esp_lcd_ili9881c lvgl esp_lvgl_port esp_hw_support esp_driver_ledc driver esp_wifi nvs_flash esp_event esp_netif fatfs esp_http_server esp_driver_sdmmc json esp_websocket_client i2c_bus esp_driver_ppa
                       EMBED_TXTFILES "web/joystick.html")
