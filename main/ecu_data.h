#ifndef ECU_DATA_H
#define ECU_DATA_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdbool.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

// ECU Data structure based on VW CAN bus spec
typedef struct {
  // Engine Parameters
  float engine_rpm;
  float tps_position;
  float abs_pedal_pos;
  float map_kpa;
  float clt_temp;        // Coolant Temperature
  float iat_temp;        // Intake Air Temperature
  float oil_temp;        // Oil Temperature
  float oil_pressure;    // Oil Pressure (kPa)
  float vehicle_speed;   // Vehicle Speed (km/h)
  float battery_voltage; // Battery Voltage (V)

  // Boost Control
  float wg_set_percent;
  float wg_pos_percent;
  float bov_percent;

  // Torque Values (Nm)
  float tcu_tq_req_nm;
  float tcu_tq_act_nm;
  float eng_trg_nm;
  float eng_act_nm;
  float limit_tq_nm;
  int8_t gear;              // Gear Position
  int8_t selector_position; // Selector Lever Position

  // System
  uint64_t timestamp;
} ecu_data_t;

// System settings
typedef struct {
  float max_boost_limit; // Maximum boost limit
  float max_rpm_limit;   // Maximum RPM limit

  bool audio_alerts_enabled; // Audio alerts enabled
  char ecu_address[32];      // ECU address

  // Gauge Visibility Settings (Screen 1)
  bool show_map;
  bool show_wastegate;
  bool show_tps;
  bool show_rpm;
  bool show_boost;
  bool show_tcu;

  // Screen 2
  bool show_oil_press;
  bool show_oil_temp;
  bool show_water_temp;
  bool show_fuel_press;
  bool show_battery;

  // Screen 4
  bool show_pedal;
  bool show_wg_pos;
  bool show_bov;
  bool show_tcu_req;
  bool show_tcu_act;
  bool show_eng_req;

  // Screen 5
  bool show_eng_act;
  bool show_limit_tq;

  // Dynamic Layout Settings
  int active_gauge_ids[24]; // Ordered list of enabled gauge IDs
  int active_gauge_count;   // Number of active gauges
  bool screen3_enabled;
  uint32_t screen_brightness; // Added for P4 compatibility
} system_settings_t;

// Gauge IDs for dynamic layout
typedef enum {
  GAUGE_NONE = -1,

  // Screen 1 Gauges
  GAUGE_MAP = 0,
  GAUGE_WASTEGATE,
  GAUGE_TPS,
  GAUGE_RPM,
  GAUGE_BOOST,
  GAUGE_TCU,

  // Screen 2 Gauges
  GAUGE_OIL_PRESS,
  GAUGE_OIL_TEMP,
  GAUGE_WATER_TEMP,
  GAUGE_FUEL_PRESS,
  GAUGE_BATTERY,

  // Screen 4 Gauges
  GAUGE_PEDAL,
  GAUGE_WG_POS,
  GAUGE_BOV,
  GAUGE_TCU_REQ,
  GAUGE_TCU_ACT,
  GAUGE_ENG_REQ,

  // Screen 5 Gauges
  GAUGE_ENG_ACT,
  GAUGE_LIMIT_TQ,

  GAUGE_MAX
} gauge_id_t;

// Connection status
typedef struct {
  bool connected;
  char message[128];
} connection_status_t;

// Data stream entry
typedef enum { LOG_INFO, LOG_WARNING, LOG_SUCCESS, LOG_ERROR } log_type_t;

typedef struct {
  uint64_t timestamp;
  char message[256];
  log_type_t type;
} data_stream_entry_t;

// Function prototypes
void ecu_data_init(void);
void ecu_data_update(ecu_data_t *data);
ecu_data_t *ecu_data_get(void);                // Unsafe, for internal use
void ecu_data_get_copy(ecu_data_t *data_copy); // Thread-safe getter
char *ecu_data_to_json(const ecu_data_t *data);
bool ecu_data_from_json(const char *json_str, ecu_data_t *data);
void ecu_data_simulate(ecu_data_t *data);

// System settings functions
void system_settings_init(void);
system_settings_t *system_settings_get(void);
void system_settings_save(const system_settings_t *settings);

// Logging functions
void data_stream_add_entry(const char *message, log_type_t type);
void data_stream_clear(void);
char *data_stream_to_json(void);

// Simple data functions for WiFi server
char *ecu_data_to_string(const ecu_data_t *data);
char *data_stream_to_string(void);

#ifdef __cplusplus
}
#endif

#endif // ECU_DATA_H
