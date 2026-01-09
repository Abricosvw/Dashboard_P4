#include "can_websocket.h"
#include "cJSON.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "settings_config.h"
#include <string.h>

static const char *TAG = "CAN_WS";

// Global data
static can_websocket_data_t g_can_data = {0};
static game_controller_state_t g_joystick_state = {0};
static SemaphoreHandle_t data_mutex = NULL;
static httpd_handle_t ws_server = NULL;
static int g_demo_counter = 0;

void generate_demo_can_data(demo_can_data_t *demo_data) {
  if (!demo_data)
    return;
  g_demo_counter++;
  int cycle = g_demo_counter % 100;
  float phase = cycle / 100.0f;

  demo_data->map_pressure =
      120.0f + 30.0f * (cycle > 50 ? (100 - cycle) : cycle) / 50.0f;
  demo_data->wastegate_pos = 45.0f + 25.0f * phase;
  demo_data->tps_position = 35.0f + 30.0f * phase;
  demo_data->engine_rpm = 2500.0f + 500.0f * phase;
  demo_data->target_boost = 180.0f + 20.0f * phase;
  demo_data->tcu_status = (g_demo_counter % 100 > 95) ? 1 : 0;
}

static esp_err_t ws_handler(httpd_req_t *req) {
  if (req->method == HTTP_GET) {
    ESP_LOGI(TAG, "WebSocket handshake done");
    return ESP_OK;
  }

  httpd_ws_frame_t ws_pkt;
  uint8_t *buf = NULL;
  memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
  ws_pkt.type = HTTPD_WS_TYPE_TEXT;

  esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
  if (ret != ESP_OK)
    return ret;

  if (ws_pkt.len) {
    buf = calloc(1, ws_pkt.len + 1);
    if (buf == NULL)
      return ESP_ERR_NO_MEM;
    ws_pkt.payload = buf;
    ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
    if (ret != ESP_OK) {
      free(buf);
      return ret;
    }

    // Handle incoming joystick data
    cJSON *root = cJSON_Parse((char *)ws_pkt.payload);
    if (root) {
      if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        cJSON *x = cJSON_GetObjectItem(root, "x");
        cJSON *y = cJSON_GetObjectItem(root, "y");
        cJSON *a = cJSON_GetObjectItem(root, "a");
        cJSON *b = cJSON_GetObjectItem(root, "b");
        cJSON *s = cJSON_GetObjectItem(root, "s");
        cJSON *l = cJSON_GetObjectItem(root, "l");

        if (x)
          g_joystick_state.x = x->valueint;
        if (y)
          g_joystick_state.y = y->valueint;
        if (a)
          g_joystick_state.button_a = a->valueint;
        if (b)
          g_joystick_state.button_b = b->valueint;
        if (s)
          g_joystick_state.button_start = s->valueint;
        if (l)
          g_joystick_state.button_select = l->valueint;

        xSemaphoreGive(data_mutex);
      }
      cJSON_Delete(root);
    }
    free(buf);
  }
  return ESP_OK;
}

static const httpd_uri_t ws_uri = {.uri = "/ws",
                                   .method = HTTP_GET,
                                   .handler = ws_handler,
                                   .user_ctx = NULL,
                                   .is_websocket = true};

static esp_err_t data_handler(httpd_req_t *req) {
  bool demo_enabled = demo_mode_get_enabled();
  char json_data[256];

  if (demo_enabled && !g_can_data.data_valid) {
    demo_can_data_t demo;
    generate_demo_can_data(&demo);
    snprintf(
        json_data, sizeof(json_data),
        "{\"map_pressure\":%.1f,\"wastegate_pos\":%.1f,\"tps_position\":%.1f,"
        "\"engine_rpm\":%.0f,\"target_boost\":%.1f,\"tcu_status\":%d}",
        demo.map_pressure, demo.wastegate_pos, demo.tps_position,
        demo.engine_rpm, demo.target_boost, demo.tcu_status);
  } else {
    snprintf(json_data, sizeof(json_data),
             "{\"map_pressure\":%d,\"wastegate_pos\":%d,\"tps_position\":%d,"
             "\"engine_rpm\":%d,\"target_boost\":%d,\"tcu_status\":%d}",
             g_can_data.map_pressure, g_can_data.wastegate_pos,
             g_can_data.tps_position, g_can_data.engine_rpm,
             g_can_data.target_boost, g_can_data.tcu_status);
  }

  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_send(req, json_data, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

static const httpd_uri_t data_api_uri = {.uri = "/data",
                                         .method = HTTP_GET,
                                         .handler = data_handler,
                                         .user_ctx = NULL};

esp_err_t start_websocket_server(void) {
  if (data_mutex == NULL) {
    data_mutex = xSemaphoreCreateMutex();
  }

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 8081;
  config.max_open_sockets = 10;

  ESP_LOGI(TAG, "Starting WebSocket/API server on port %d", config.server_port);
  if (httpd_start(&ws_server, &config) == ESP_OK) {
    httpd_register_uri_handler(ws_server, &ws_uri);
    httpd_register_uri_handler(ws_server, &data_api_uri);
    return ESP_OK;
  }
  return ESP_FAIL;
}

void stop_websocket_server(void) {
  if (ws_server) {
    httpd_stop(ws_server);
    ws_server = NULL;
  }
}

void update_websocket_can_data(uint16_t rpm, uint16_t map, uint8_t tps,
                               uint8_t wastegate, uint16_t target_boost,
                               uint8_t tcu_status) {
  if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    g_can_data.engine_rpm = rpm;
    g_can_data.map_pressure = map;
    g_can_data.tps_position = tps;
    g_can_data.wastegate_pos = wastegate;
    g_can_data.target_boost = target_boost;
    g_can_data.tcu_status = tcu_status;
    g_can_data.data_valid = true;
    xSemaphoreGive(data_mutex);
  }
}

void websocket_get_joystick_state(game_controller_state_t *state) {
  if (state && xSemaphoreTake(data_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
    *state = g_joystick_state;
    xSemaphoreGive(data_mutex);
  }
}

void websocket_broadcast_task(void *pvParameters) {
  while (1) {
    // Here we could add logic to push data via WebSocket to all clients
    // For now, clients pull via /data or we can implement httpd_ws_send_frame
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
