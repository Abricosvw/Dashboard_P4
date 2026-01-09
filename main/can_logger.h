#ifndef CAN_LOGGER_H
#define CAN_LOGGER_H

#include <stdbool.h>
#include <stdint.h>


// Initialize the CAN logger (create task, queue, etc.)
void can_logger_init(void);

// Start recording to a new file
void can_logger_start(void);

// Stop recording
void can_logger_stop(void);

// Queue a CAN message for logging
void can_logger_log(uint32_t id, uint8_t *data, uint8_t dlc);

// Check if currently recording
bool can_logger_is_recording(void);

// Set a callback function to be called when recording stops automatically (e.g.
// limit reached)
void can_logger_set_stop_callback(void (*cb)(void));

#endif // CAN_LOGGER_H
