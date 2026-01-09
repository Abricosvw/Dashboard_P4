#ifndef CAN_DEFINITIONS_H
#define CAN_DEFINITIONS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Platform Selection Enum
typedef enum {
  PLATFORM_VW_PQ35_46 =
      0,            // Default (Passat B6/CC, Golf 5/6, Octavia A5, Superb 2)
  PLATFORM_VW_PQ25, // Polo 6R, Fabia 2
  PLATFORM_VW_MQB,  // Golf 7, Octavia A7, Superb 3 (Placeholders)
  PLATFORM_BMW_E9X, // BMW E90/E60 (Exx CAN)
  PLATFORM_BMW_E46, // BMW E46 (Limited support)
  PLATFORM_BMW_F_SERIES, // BMW F-Series (F30, etc.)
  PLATFORM_MAX
} CanPlatform;

// Platform Configuration Structure
// Defines the core IDs used for common dashboard features.
// Note: Parsing logic often varies beyond just IDs (scaling, bit-shifting),
// so this struct is primarily fo generic ID filtering. Detailed parsing
// is handled by platform-specific functions in can_parser.c
typedef struct {
  const char *name;

  // Core Engine IDs
  uint32_t id_rpm;     // Engine RPM
  uint32_t id_speed;   // Vehicle Speed
  uint32_t id_coolant; // Coolant Temperature
  uint32_t id_tps;     // Throttle Position

  // Status IDs
  uint32_t id_gear;      // Transmission Gear
  uint32_t id_lights;    // Lighting Status
  uint32_t id_handbrake; // Parking Brake
  uint32_t id_doors;     // Door Status

  // Scaling Factors (if simple multiplication is enough)
  float rpm_factor;
  float speed_factor;

} CanPlatformConfig;

// Common VW IDs (PQ35/46 usually)
#define VW_PQ_RPM_ID 0x280
#define VW_PQ_SPEED_ID 0x5A0 // ABS Speed
#define VW_PQ_COOLANT_ID 0x288

// BMW E-Series IDs
#define BMW_E_RPM_ID 0x0AA
#define BMW_E_SPEED_ID 0x1A6
#define BMW_E_COOLANT_ID 0x1D0

// BMW F-Series IDs (Preliminary)
#define BMW_F_RPM_ID 0x0A5 // Needs 0-CRC check?
#define BMW_F_SPEED_ID 0x1A1
#define BMW_F_COOLANT_ID 0x ? // TBD

// VW MQB IDs (Derived from Opendbc/r00li research)
#define VW_MQB_RPM_ID 0x280     // Often legacy compatible? TBD
#define VW_MQB_SPEED_ID 0xFD    // ESP_21 per OpenDBC
#define VW_MQB_COOLANT_ID 0x288 // Often legacy compatible? TBD

// External reference to the current configuration
extern CanPlatform g_current_platform;
extern const CanPlatformConfig *g_current_platform_config;

#ifdef __cplusplus
}
#endif

#endif // CAN_DEFINITIONS_H
