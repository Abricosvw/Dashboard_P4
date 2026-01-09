#include "wifi_init.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <string.h>
#include "sdkconfig.h"

#define SLAVE_RESET_IO CONFIG_WIFI_SLAVE_RESET_IO

static const char *TAG = "WIFI_AP";

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

static void wifi_reset_slave(void) {
  ESP_LOGI(TAG, "Resetting ESP32-C6 (GPIO %d)...", SLAVE_RESET_IO);
  gpio_config_t reset_io_conf = {
      .intr_type = GPIO_INTR_DISABLE,
      .mode = GPIO_MODE_OUTPUT,
      .pin_bit_mask = (1ULL << SLAVE_RESET_IO),
  };
  gpio_config(&reset_io_conf);
  gpio_set_level(SLAVE_RESET_IO, 0);
  vTaskDelay(pdMS_TO_TICKS(100));
  gpio_set_level(SLAVE_RESET_IO, 1);
  ESP_LOGI(TAG, "ESP32-C6 Reset Released (GPIO %d = 1)", SLAVE_RESET_IO);
  vTaskDelay(pdMS_TO_TICKS(500)); // Wait for C6 to boot
}

esp_err_t wifi_init_ap(const char *ssid, const char *pass) {
  ESP_LOGI(TAG, "Starting WiFi AP initialization...");
  wifi_reset_slave();

  ESP_LOGI(TAG, "Initializing NVS...");
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

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
              .max_connection = 4,
              .authmode = WIFI_AUTH_WPA2_PSK,
              .channel = 1,
          },
  };
  strncpy((char *)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid));
  strncpy((char *)wifi_config.ap.password, pass,
          sizeof(wifi_config.ap.password));
  wifi_config.ap.ssid_len = strlen(ssid);

  if (strlen(pass) == 0) {
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;
  }

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  return ESP_OK;
}

esp_err_t wifi_init_sta(const char *ssid, const char *pass) {
  ESP_LOGI(TAG, "Starting WiFi STA initialization...");
  wifi_reset_slave();

  ESP_LOGI(TAG, "Initializing NVS...");
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

  wifi_config_t wifi_config = {
      .sta =
          {
              .threshold.authmode = WIFI_AUTH_WPA2_PSK,
              .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
          },
  };

  strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
  strncpy((char *)wifi_config.sta.password, pass,
          sizeof(wifi_config.sta.password));

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_connect());

  return ESP_OK;
}
