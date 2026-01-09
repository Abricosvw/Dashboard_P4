#include "ai_manager.h"
#include "ai_config.h"
#include "cJSON.h"
#include "esp_log.h"
#include "esp_websocket_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "AI_MGR";
static esp_websocket_client_handle_t ws_client = NULL;
static bool is_connected = false;

static void websocket_event_handler(void *handler_args, esp_event_base_t base,
                                    int32_t event_id, void *event_data) {
  esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
  switch (event_id) {
  case WEBSOCKET_EVENT_CONNECTED:
    ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
    is_connected = true;

    // Send Authentication / Hello Handshake
    // Based on xiaozhi protocol, we might need to send a specific JSON.
    // For now, sending a simple generic Auth if required, or waiting for server
    // 'hello'.

    // Example Auth JSON construction
    cJSON *auth_json = cJSON_CreateObject();
    cJSON_AddStringToObject(auth_json, "type", "hello");
    cJSON_AddStringToObject(auth_json, "token",
                            XIAOZHI_AGENT_CODE); // Using Agent Code as token
    char *auth_str = cJSON_PrintUnformatted(auth_json);

    if (esp_websocket_client_send_text(ws_client, auth_str, strlen(auth_str),
                                       pdMS_TO_TICKS(1000))) {
      ESP_LOGI(TAG, "Sent Auth Handshake");
    }

    cJSON_free(auth_str);
    cJSON_Delete(auth_json);
    break;

  case WEBSOCKET_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
    is_connected = false;
    break;

  case WEBSOCKET_EVENT_DATA:
    ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");
    ESP_LOGI(TAG, "Received opcode=%d", data->op_code);
    if (data->op_code == WS_TRANSPORT_OPCODES_TEXT) {
      ESP_LOGI(TAG, "Received Text: %.*s", data->data_len,
               (char *)data->data_ptr);
    }
    break;

  case WEBSOCKET_EVENT_ERROR:
    ESP_LOGE(TAG, "WEBSOCKET_EVENT_ERROR");
    break;
  }
}

esp_err_t ai_manager_init(void) {
  ESP_LOGI(TAG, "Initializing AI Manager...");

  // Check if WebSocket component is available (compilation check essentially)

  esp_websocket_client_config_t websocket_cfg = {};
  websocket_cfg.uri = XIAOZHI_SERVER_URL;

  // If wss:// add cert bundle? For now assuming ws:// or skipping cert
  // verification for test websocket_cfg.cert_pem = ...;

  ws_client = esp_websocket_client_init(&websocket_cfg);
  if (!ws_client) {
    ESP_LOGE(TAG, "Failed to init WebSocket Client");
    return ESP_FAIL;
  }

  esp_websocket_register_events(ws_client, WEBSOCKET_EVENT_ANY,
                                websocket_event_handler, (void *)ws_client);

  return ESP_OK;
}

esp_err_t ai_manager_start(void) {
  if (!ws_client)
    return ESP_FAIL;

  ESP_LOGI(TAG, "Starting WebSocket Client...");
  return esp_websocket_client_start(ws_client);
}

#include "audio_manager.h" // Include audio manager

void ai_manager_trigger_listening(void) {
  ESP_LOGI(TAG, "Manual Listening Triggered");

  // For testing: Record 3 seconds of audio to SD card to verify mic
  if (audio_record_wav("/sdcard/ai_test.wav", 3000) == ESP_OK) {
    ESP_LOGI(TAG, "Recorded ai_test.wav to SD card");

    // Optional: Play it back to confirm
    // audio_play_wav("/sdcard/ai_test.wav");
  } else {
    ESP_LOGE(TAG, "Failed to record audio");
  }

  // TODO: In real implementation, stream this data to ws_client
}
