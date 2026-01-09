#ifndef WIFI_CONTROLLER_H
#define WIFI_CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>


typedef struct {
  int16_t x; // -100 to 100
  int16_t y; // -100 to 100
  bool button_a;
  bool button_b;
  bool button_start;
  bool button_select;
} game_controller_state_t;

/**
 * @brief Initialize WiFi SoftAP and Web Server
 */
void wifi_controller_init(void);

/**
 * @brief Get the latest controller state
 * @param state Pointer to structure to fill
 */
void wifi_controller_get_state(game_controller_state_t *state);

#endif // WIFI_CONTROLLER_H
