#include "can_logger.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "sd_card_manager.h" // Use P4 SD manager
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static const char *TAG = "can_logger";

// Helper to get human readable name for CAN ID
static const char *get_can_id_name(uint32_t id) {
  switch (id) {
  case 0x362:
    return "ACC_1";
  case 0x050:
    return "Airbag_1";
  case 0x550:
    return "Airbag_2";
  case 0x372:
    return "BEM_1_ACAN";
  case 0x1A0:
    return "Bremse_1";
  case 0x5A0:
    return "Bremse_2";
  case 0x4A0:
    return "Bremse_3";
  case 0x4A8:
    return "Bremse_5";
  case 0x1AC:
    return "Bremse_8";
  case 0x5F4:
    return "Bremse_Codierinfo_Neu";
  case 0x5EE:
    return "CDEF_ACAN_MOST";
  case 0x5EC:
    return "CDEF_MOST_ACAN";
  case 0x7D0:
    return "Diagnose_1";
  case 0x5C0:
    return "EPB_1";
  case 0x390:
    return "Wastegate"; // Custom (was Gateway_Komfort_1)
  case 0x2AC:
    return "Geschwindigkeit_1";
  case 0x440:
    return "Getriebe_1";
  case 0x540:
    return "Getriebe_2";
  case 0x44C:
    return "Getriebe_6";
  case 0x44A:
    return "GME";
  case 0x38A:
    return "GRA_Neu";
  case 0x5D2:
    return "IDENT_D3";
  case 0x5E0:
    return "Klima1_D3_ACAN";
  case 0x320:
    return "Kombi_1";
  case 0x420:
    return "Kombi_2";
  case 0x520:
    return "Kombi_3";
  case 0x394:
    return "Blow_off"; // Custom (was LWR_Zustand)
  case 0x0C2:
    return "LWS_1";
  case 0x7C0:
    return "LWS_Calib";
  case 0x5C6:
    return "LWS_Fehler";
  case 0x7C2:
    return "LWS_Init";
  case 0x280:
    return "Motor_1";
  case 0x288:
    return "Motor_2";
  case 0x380:
    return "Motor_3";
  case 0x480:
    return "Motor_5";
  case 0x488:
    return "Motor_6";
  case 0x588:
    return "Motor_7";
  case 0x48A:
    return "Motor_8";
  case 0x580:
    return "Motor_Flexia_Neu";
  case 0x71F:
    return "Motorslave_Istverbau";
  case 0x590:
    return "Niveau_1";
  case 0x594:
    return "Niveau_2";
  case 0x59A:
    return "Niveau_3";
  case 0x7D4:
    return "PSD";
  case 0x51A:
    return "TOG";
  case 0x572:
    return "ZAS_1";
  default:
    return NULL;
  }
}

#define LOG_QUEUE_SIZE 100
#define LOG_BUFFER_SIZE 4096
#define MAX_FILE_SIZE (1024 * 1024) // 1MB

typedef struct {
  uint32_t timestamp;
  uint32_t id;
  uint8_t dlc;
  uint8_t data[8];
} can_log_msg_t;

static QueueHandle_t log_queue = NULL;
static TaskHandle_t log_task_handle = NULL;
static bool is_recording = false;
static FILE *log_file = NULL;
static size_t current_file_size = 0;
static char log_buffer[LOG_BUFFER_SIZE];
static size_t buffer_index = 0;
static void (*stop_callback)(void) = NULL;

// Helper to flush buffer to file
static void flush_buffer(void) {
  if (log_file && buffer_index > 0) {
    fwrite(log_buffer, 1, buffer_index, log_file);
    current_file_size += buffer_index;
    buffer_index = 0;
  }
}

static void can_logger_task(void *arg) {
  can_log_msg_t msg;
  while (1) {
    if (xQueueReceive(log_queue, &msg, pdMS_TO_TICKS(100)) == pdTRUE) {
      if (is_recording) {
        // Format: Timestamp(ms),ID(hex),Name,DLC,Data(hex)\n
        const char *name = get_can_id_name(msg.id);
        int len;
        if (name) {
          len = snprintf(log_buffer + buffer_index,
                         LOG_BUFFER_SIZE - buffer_index, "%lu,%03X,%s,%d,",
                         (unsigned long)msg.timestamp, (unsigned int)msg.id,
                         name, msg.dlc);
        } else {
          len = snprintf(log_buffer + buffer_index,
                         LOG_BUFFER_SIZE - buffer_index, "%lu,%03X,,%d,",
                         (unsigned long)msg.timestamp, (unsigned int)msg.id,
                         msg.dlc);
        }

        buffer_index += len;

        for (int i = 0; i < msg.dlc; i++) {
          len = snprintf(log_buffer + buffer_index,
                         LOG_BUFFER_SIZE - buffer_index, "%02X", msg.data[i]);
          buffer_index += len;
        }

        len = snprintf(log_buffer + buffer_index,
                       LOG_BUFFER_SIZE - buffer_index, "\n");
        buffer_index += len;

        // Flush if buffer is full or nearly full
        if (buffer_index >= LOG_BUFFER_SIZE - 64) {
          flush_buffer();
        }

        // Check file size limit
        if (current_file_size >= MAX_FILE_SIZE) {
          ESP_LOGI(TAG, "File limit reached. Stopping.");
          can_logger_stop();
          if (stop_callback) {
            stop_callback();
          }
        }
      }
    } else {
      // Timeout - flush any remaining data if recording
      if (is_recording && buffer_index > 0) {
        flush_buffer();
      }
    }
  }
}

void can_logger_init(void) {
  log_queue = xQueueCreate(LOG_QUEUE_SIZE, sizeof(can_log_msg_t));
  xTaskCreate(can_logger_task, "can_logger", 4096, NULL, 5, &log_task_handle);
}

void can_logger_start(void) {
  if (is_recording)
    return;

  // Check if SD card is mounted
  if (!sd_card_is_mounted()) {
    ESP_LOGE(TAG, "SD card not mounted, cannot start logging");
    return;
  }

  // Find next available filename
  char filename[32];
  int index = 1;
  struct stat st;
  do {
    snprintf(filename, sizeof(filename), "/sdcard/trace_%03d.txt", index++);
  } while (stat(filename, &st) == 0);

  ESP_LOGI(TAG, "Starting log to %s", filename);

  // Open file using standard fopen
  log_file = fopen(filename, "w");
  if (log_file) {
    is_recording = true;
    current_file_size = 0;
    buffer_index = 0;

    // Write header
    fprintf(log_file, "Timestamp,ID,Name,DLC,Data\n");
  } else {
    ESP_LOGE(TAG, "Failed to open log file");
  }
}

void can_logger_stop(void) {
  if (!is_recording)
    return;

  // Flush remaining data
  flush_buffer();

  // Close file
  if (log_file) {
    fclose(log_file);
    log_file = NULL;
  }

  is_recording = false;
  ESP_LOGI(TAG, "Logging stopped");
}

void can_logger_log(uint32_t id, uint8_t *data, uint8_t dlc) {
  if (!is_recording)
    return;

  can_log_msg_t msg;
  msg.timestamp = (uint32_t)(esp_timer_get_time() / 1000);
  msg.id = id;
  msg.dlc = dlc;
  memcpy(msg.data, data, dlc > 8 ? 8 : dlc);

  // Send to queue (non-blocking)
  xQueueSend(log_queue, &msg, 0);
}

bool can_logger_is_recording(void) { return is_recording; }

void can_logger_set_stop_callback(void (*cb)(void)) { stop_callback = cb; }
