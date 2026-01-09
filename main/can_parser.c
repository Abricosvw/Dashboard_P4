#include "can_parser.h"
#include "ecu_data.h"
#include "esp_log.h"
#include <math.h>
#include <string.h>


static const char *TAG = "CAN_PARSER";

// --- Global State ---
CanPlatform g_current_platform = PLATFORM_VW_PQ35_46; // Default
const CanPlatformConfig *g_current_platform_config = NULL;
static float g_max_torque_nm = 500.0f;

// --- Helper Functions ---
static inline uint16_t get_u16_le(const uint8_t *data, int offset) {
  return (uint16_t)(data[offset + 1] << 8) | data[offset];
}

static inline uint16_t get_u16_be(const uint8_t *data, int offset) {
  return (uint16_t)(data[offset] << 8) | data[offset + 1];
}

void can_parser_set_max_torque(float max_torque) {
  if (max_torque > 0) {
    g_max_torque_nm = max_torque;
  }
}

// --- Platform Parsers ---

// 1. VW PQ35/PQ46 (Passat B6, Golf 5/6, etc.) - The original implementation
static void parse_vw_pq35_46(const twai_message_t *message,
                             ecu_data_t *ecu_data) {
  switch (message->identifier) {
  case 0x280: // Motor_1: RPM (0.25 scaling)
    ecu_data->engine_rpm =
        ((uint16_t)message->data[3] << 8 | message->data[2]) * 0.25f;
    ecu_data->eng_act_nm = message->data[1] * 0.39f;  // Inneres_Motormoment
    ecu_data->tps_position = message->data[5] * 0.4f; // Throttle
    ecu_data->eng_trg_nm = message->data[7] * 0.39f;  // Requested Torque
    break;

  case 0x288: // Motor_2: Coolant (PQ35/46 uses this for Coolant, PQ25 might
              // differ)
    ecu_data->clt_temp = (message->data[1] * 0.75f) - 48.0f;
    ecu_data->limit_tq_nm = message->data[6] * 0.39f;
    break;

  case 0x380: // Motor_3: IAT
    ecu_data->iat_temp = (message->data[1] * 0.75f) - 48.0f;
    ecu_data->abs_pedal_pos = message->data[2] * 0.4f;
    break;

  case 0x588: // Motor_7: Oil Temp & Boost
    ecu_data->map_kpa = (message->data[4] * 0.01f) * 100.0f; // Bar -> kPa
    ecu_data->oil_temp = (message->data[7] * 1.0f) - 60.0f;
    break;

  case 0x372: // Battery
    ecu_data->battery_voltage = (message->data[5] * 0.05f) + 5.0f;
    break;

  case 0x540: // Gear (Getriebe_2)
    ecu_data->gear = (message->data[7] >> 4) & 0x0F;
    break;

  case 0x1A0: // Speed source (Bremse_1) - ABS Wheel Speed
  {
    uint16_t raw_speed = ((uint16_t)message->data[3] << 8 | message->data[2]);
    raw_speed = (raw_speed >> 1) & 0x7FFF;
    ecu_data->vehicle_speed = raw_speed * 0.01f;
  } break;

    // Custom / Other
  case 0x390: // Wastegate (Custom)
    ecu_data->wg_set_percent = message->data[1] / 2.0f;
    ecu_data->wg_pos_percent = message->data[2] / 2.0f;
    break;

  case 0x394: // BOV (Custom)
    ecu_data->bov_percent = (message->data[0] * 50.0f) / 255.0f;
    break;

  default:
    break;
  }
}

// 2. VW PQ25 (Polo 6R, Fabia 2)
static void parse_vw_pq25(const twai_message_t *message, ecu_data_t *ecu_data) {
  switch (message->identifier) {
  case 0x280: // Same as PQ35
    ecu_data->engine_rpm =
        ((uint16_t)message->data[3] << 8 | message->data[2]) * 0.25f;
    break;

  case 0x5A0: // Speed_1 (Dash Speed)
  {
    uint16_t raw_speed = get_u16_le(message->data, 1);
    ecu_data->vehicle_speed = raw_speed * 0.01f;
  } break;

  case 0x1A0: // ABS Speed (Fallback)
  {
    uint16_t raw_speed = ((uint16_t)message->data[3] << 8 | message->data[2]);
    raw_speed = (raw_speed >> 1) & 0x7FFF;
    ecu_data->vehicle_speed = raw_speed * 0.01f;
  } break;

  case 0x288:
    ecu_data->clt_temp = (message->data[1] * 0.75f) - 48.0f;
    break;

  default:
    break;
  }
}

// 3. BMW E-Series (E90/E60)
static void parse_bmw_e_series(const twai_message_t *message,
                               ecu_data_t *ecu_data) {
  switch (message->identifier) {
  case 0x0AA: // RPM (DME1)
  {
    uint16_t raw = get_u16_le(message->data, 4);
    ecu_data->engine_rpm = raw / 4.0f;
  } break;

  case 0x1D0: // Engine Temp
    ecu_data->clt_temp = message->data[0] - 48.0f;
    break;

  case 0x1A6: // Speed (Cluster Speed)
  {
    uint16_t raw = get_u16_le(message->data, 0);
    ecu_data->vehicle_speed = raw / 2.0f;
  } break;

  case 0x1D2: // Gear
    ecu_data->gear = message->data[0];
    break;

  default:
    break;
  }
}

// 4. BMW F-Series (F10/F30)
static void parse_bmw_f_series(const twai_message_t *message,
                               ecu_data_t *ecu_data) {
  switch (message->identifier) {
  // Placeholder - BN2020 needs CRC validation for production
  default:
    break;
  }
}

// 5. VW MQB (Golf 7, Octavia A7)
static void parse_vw_mqb(const twai_message_t *message, ecu_data_t *ecu_data) {
  switch (message->identifier) {
  case 0x280: // RPM
    ecu_data->engine_rpm =
        ((uint16_t)message->data[3] << 8 | message->data[2]) * 0.25f;
    break;

  case 0x0FD: // ESP_21 : Speed
  {
    uint16_t raw_speed = get_u16_le(message->data, 1);
    ecu_data->vehicle_speed = raw_speed * 0.01f;
  } break;

  case 0x288: // Coolant
    ecu_data->clt_temp = (message->data[1] * 0.75f) - 48.0f;
    break;

  default:
    break;
  }
}

// --- Main Dispatcher ---

void can_parser_set_platform(CanPlatform platform) {
  if (platform < PLATFORM_MAX) {
    g_current_platform = platform;
    ESP_LOGI(TAG, "Switched to CAN Platform: %d", platform);
  }
}

CanPlatform can_parser_get_platform(void) { return g_current_platform; }

void parse_can_message(const twai_message_t *message) {
  if (!message)
    return;

  ecu_data_t ecu_data;
  ecu_data_get_copy(&ecu_data);

  switch (g_current_platform) {
  case PLATFORM_VW_PQ35_46:
    parse_vw_pq35_46(message, &ecu_data);
    break;
  case PLATFORM_VW_PQ25:
    parse_vw_pq25(message, &ecu_data);
    break;
  case PLATFORM_BMW_E9X:
  case PLATFORM_BMW_E46:
    parse_bmw_e_series(message, &ecu_data);
    break;
  case PLATFORM_BMW_F_SERIES:
    parse_bmw_f_series(message, &ecu_data);
    break;
  case PLATFORM_VW_MQB:
    parse_vw_mqb(message, &ecu_data);
    break;
  default:
    parse_vw_pq35_46(message, &ecu_data);
    break;
  }

  ecu_data_update(&ecu_data);
}
