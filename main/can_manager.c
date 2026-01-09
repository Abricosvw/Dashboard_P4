#include "can_manager.h"
#include "can_parser.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sd_card_manager.h"
#include "ui/screens/ui_Screen3.h" // Include Screen3 header
#include <stdio.h>
#include <time.h>

// Extern LVGL lock functions (defined in main.c usually)
extern bool example_lvgl_lock(int timeout_ms);
extern void example_lvgl_unlock(void);

static const char *TAG = "CAN_MGR";

ecu_data_t g_ecu_data = {0};

esp_err_t can_init(void) {
  // 1. Initialize configuration structures
  // Using TWAI_MODE_NO_ACK allows the device to receive messages even if it's
  // the only node on the bus and prevents it from interfering with the
  // vehicle's bus during initial testing.
  twai_general_config_t g_config =
      TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_IO, CAN_RX_IO, TWAI_MODE_NO_ACK);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  // 2. Install TWAI driver
  ESP_RETURN_ON_ERROR(twai_driver_install(&g_config, &t_config, &f_config), TAG,
                      "TWAI driver install failed");
  ESP_LOGI(TAG, "TWAI driver installed");

  // 3. Start TWAI driver
  ESP_RETURN_ON_ERROR(twai_start(), TAG, "TWAI driver start failed");
  ESP_LOGI(TAG, "TWAI driver started");

  // 4. Create CAN RX Task
  xTaskCreatePinnedToCore(can_rx_task, "can_rx_task", 4096, NULL, 5, NULL, 0);

  return ESP_OK;
}

void can_rx_task(void *pvParameters) {
  twai_message_t message;
  while (1) {
    if (twai_receive(&message, pdMS_TO_TICKS(1000)) == ESP_OK) {
      // Process message using the powerful new parser
      parse_can_message(&message);

      // Send to UI Sniffer / Logger
      // Must take LVGL lock because we are calling UI functions
      if (example_lvgl_lock(10)) {
        ui_process_real_can_message(message.identifier, message.data,
                                    message.data_length_code);
        example_lvgl_unlock();
      }

      // Log to SD Card (Legacy direct logging - REMOVED, handled by UI/Logger
      // now)
    } else {
      // No message received in timeout
      // ESP_LOGV(TAG, "Waiting for CAN messages...");
    }
  }
}
