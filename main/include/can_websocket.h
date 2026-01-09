#ifndef CAN_WEBSOCKET_H
#define CAN_WEBSOCKET_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>


// Initialize and start data server
esp_err_t start_websocket_server(void);

// CAN data structure for broadcasting
typedef struct {
  uint16_t map_pressure; // 100-250 kPa
  uint8_t wastegate_pos; // 0-100 %
  uint8_t tps_position;  // 0-100 %
  uint16_t engine_rpm;   // 0-7000 RPM
  uint16_t target_boost; // 100-250 kPa
  uint8_t tcu_status;    // 0=OK, 1=WARN, 2=ERROR
  bool data_valid;
} can_websocket_data_t;

// Game Controller (Joystick) state structure
typedef struct {
  int16_t x; // -100 to 100
  int16_t y; // -100 to 100
  bool button_a;
  bool button_b;
  bool button_start;
  bool button_select;
} game_controller_state_t;

// Stop data server
void stop_websocket_server(void);

// Update CAN data for broadcast
void update_websocket_can_data(uint16_t rpm, uint16_t map, uint8_t tps,
                               uint8_t wastegate, uint16_t target_boost,
                               uint8_t tcu_status);

// Get current joystick state
void websocket_get_joystick_state(game_controller_state_t *state);

// Data broadcast task
void websocket_broadcast_task(void *pvParameters);

// Demo data generation
typedef struct {
  float map_pressure;
  float wastegate_pos;
  float tps_position;
  float engine_rpm;
  float target_boost;
  int tcu_status;
} demo_can_data_t;

void generate_demo_can_data(demo_can_data_t *demo_data);

#endif // CAN_WEBSOCKET_H
