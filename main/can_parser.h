#ifndef CAN_PARSER_H
#define CAN_PARSER_H

#include "can_definitions.h"
#include "driver/twai.h"


#ifdef __cplusplus
extern "C" {
#endif

// Helper function to extract a 16-bit unsigned integer from a byte array.
// static inline uint16_t get_u16(const uint8_t* data, int offset) { ... }
// (Internal use only)

// Set the active CAN platform
void can_parser_set_platform(CanPlatform platform);

// Get the currently active CAN platform
CanPlatform can_parser_get_platform(void);

// Function to parse a received CAN message and update the ECU data structure.
void parse_can_message(const twai_message_t *message);

// Function to set the configurable maximum torque value for calculations.
void can_parser_set_max_torque(float max_torque);

#ifdef __cplusplus
}
#endif

#endif // CAN_PARSER_H
