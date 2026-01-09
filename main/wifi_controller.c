#include "wifi_controller.h"
#include <stdlib.h>
#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "wifi_fallbacks.h"
#include <nvs_flash.h>
#include <sys/param.h>

#if !CONFIG_IDF_TARGET_ESP32P4
#include "esp_netif.h"
#endif
#include "cJSON.h"
#include "esp_http_server.h"

static const char *TAG = "WIFI_CONTROLLER";

// Global state protected by mutex (or simple atomic for now since it's one
// writer)
static game_controller_state_t current_state = {0};
static SemaphoreHandle_t state_mutex = NULL;

// Embedded HTML (we could also mount SPIFFS, but embedding is easier for single
// file) We will use the file we created: main/web/joystick.html To embed it, we
// need to add it to CMakeLists.txt as EMBED_TXTFILES For now, let's assume we
// can read it or hardcode it. Actually, standard ESP-IDF way is embedding
// binary. Let's declare the external symbols.
extern const uint8_t joystick_html_start[] asm("_binary_joystick_html_start");
extern const uint8_t joystick_html_end[] asm("_binary_joystick_html_end");

#if !CONFIG_IDF_TARGET_ESP32P4
/* WiFi Event Handler */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  if (event_id == WIFI_EVENT_AP_STACONNECTED) {
    wifi_event_ap_staconnected_t *event =
        (wifi_event_ap_staconnected_t *)event_data;
    ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac),
             event->aid);
  } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
    wifi_event_ap_stadisconnected_t *event =
        (wifi_event_ap_stadisconnected_t *)event_data;
    ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac),
             event->aid);
  }
}
#endif

#if !CONFIG_IDF_TARGET_ESP32P4
/* HTTP GET Handler for / */
static esp_err_t root_get_handler(httpd_req_t *req) {
  // Send the embedded HTML
  // Calculate size
  size_t html_len = joystick_html_end - joystick_html_start;
  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, (const char *)joystick_html_start, html_len);
  return ESP_OK;
}

static const httpd_uri_t root = {.uri = "/",
                                 .method = HTTP_GET,
                                 .handler = root_get_handler,
                                 .user_ctx = NULL};

/* WebSocket Handler for /ws */
static esp_err_t ws_handler(httpd_req_t *req) {
  if (req->method == HTTP_GET) {
    ESP_LOGI(TAG, "Handshake done, the new connection was opened");
    return ESP_OK;
  }

  httpd_ws_frame_t ws_pkt;
  uint8_t *buf = NULL;
  memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
  ws_pkt.type = HTTPD_WS_TYPE_TEXT;

  // Set max length
  esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
    return ret;
  }

  if (ws_pkt.len) {
    buf = calloc(1, ws_pkt.len + 1);
    if (buf == NULL) {
      ESP_LOGE(TAG, "Failed to calloc memory for buf");
      return ESP_ERR_NO_MEM;
    }
    ws_pkt.payload = buf;
    ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
      free(buf);
      return ret;
    }

    // Parse JSON
    // Expected: {"x":0,"y":0,"a":0,"b":0,"s":0,"l":0}
    cJSON *root = cJSON_Parse((char *)ws_pkt.payload);
    if (root) {
      if (state_mutex &&
          xSemaphoreTake(state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        cJSON *x = cJSON_GetObjectItem(root, "x");
        cJSON *y = cJSON_GetObjectItem(root, "y");
        cJSON *a = cJSON_GetObjectItem(root, "a");
        cJSON *b = cJSON_GetObjectItem(root, "b");
        cJSON *s = cJSON_GetObjectItem(root, "s");
        cJSON *l = cJSON_GetObjectItem(root, "l");

        if (x)
          current_state.x = x->valueint;
        if (y)
          current_state.y = y->valueint;
        if (a)
          current_state.button_a = a->valueint;
        if (b)
          current_state.button_b = b->valueint;
        if (s)
          current_state.button_start = s->valueint;
        if (l)
          current_state.button_select = l->valueint;

        xSemaphoreGive(state_mutex);
      }
      cJSON_Delete(root);
    }
    free(buf);
  }
  return ESP_OK;
}

static const httpd_uri_t ws = {.uri = "/ws",
                               .method = HTTP_GET,
                               .handler = ws_handler,
                               .user_ctx = NULL,
                               .is_websocket = true};

static httpd_handle_t start_webserver(void) {
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_open_sockets = 3; // Limit sockets to save RAM

  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
  if (httpd_start(&server, &config) == ESP_OK) {
    ESP_LOGI(TAG, "Registering URI handlers");
    httpd_register_uri_handler(server, &root);
    httpd_register_uri_handler(server, &ws);
    return server;
  }

  ESP_LOGI(TAG, "Error starting server!");
  return NULL;
}
#endif

void wifi_controller_init(void) {
  state_mutex = xSemaphoreCreateMutex();

  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

#if !CONFIG_IDF_TARGET_ESP32P4
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

  wifi_config_t wifi_config = {
      .ap =
          {
              .ssid = "ESP32_GAME_CONTROLLER",
              .ssid_len = strlen("ESP32_GAME_CONTROLLER"),
              .channel = 1,
              .password = "",
              .max_connection = 4,
              .authmode = WIFI_AUTH_OPEN,
              .pmf_cfg =
                  {
                      .required = false,
                  },
          },
  };

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
           "ESP32_GAME_CONTROLLER", "OPEN", 1);

  start_webserver();
#else
  ESP_LOGW(TAG, "WiFi Controller is NOT supported on ESP32-P4 target (No "
                "integrated WiFi)");
#endif
}

void wifi_controller_get_state(game_controller_state_t *state) {
  if (state_mutex && xSemaphoreTake(state_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
    *state = current_state;
    xSemaphoreGive(state_mutex);
  }
}
