#pragma once

#include "esp_err.h"
#include "driver/twai.h"
#include "ecu_data.h"

// TWAI Pin Definitions (User provided)
#define CAN_TX_IO           (20)
#define CAN_RX_IO           (21)

// CAN Baudrate (Defaulting to 500kbps, common for automotive)
#define CAN_BAUDRATE_KBPS   (500)

/**
 * @brief Initialize the CAN (TWAI) driver and start the reception task.
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t can_init(void);

/**
 * @brief Task responsible for receiving and parsing CAN frames.
 */
void can_rx_task(void *pvParameters);
