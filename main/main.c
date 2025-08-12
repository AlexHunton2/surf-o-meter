#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <stdio.h>

#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PASS CONFIG_WIFI_PASS
#define OTA_URL CONFIG_OTA_URL

static const char *TAG = "MAIN";

/* Embed the server certificate in the binary */
extern const uint8_t
    ca_cert_pem_start[] asm("_binary_digicert_global_root_g2_pem_start");
extern const uint8_t
    ca_cert_pem_end[] asm("_binary_digicert_global_root_g2_pem_end");

static void wifi_init(void) {
  esp_netif_init();
  esp_event_loop_create_default();
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);

  wifi_config_t wifi_config = {
      .sta =
          {
              .ssid = WIFI_SSID,
              .password = WIFI_PASS,
          },
  };

  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
  esp_wifi_start();

  ESP_LOGI(TAG, "Connecting to WiFi...");
  if (esp_wifi_connect() != ESP_OK) {
    ESP_LOGI(TAG, "Failed to connect to WiFi, restarting...");
    esp_restart();
  }
}

static void ota_update_task(void *pvParameter) {
  ESP_LOGI(TAG, "Starting OTA update from: %s", OTA_URL);

  esp_http_client_config_t http_cfg = {
      .url = OTA_URL,
      .cert_pem = (const char *)ca_cert_pem_start,
  };

  esp_https_ota_config_t ota_cfg = {
      .http_config = &http_cfg,
  };

  esp_err_t ret = esp_https_ota(&ota_cfg);
  if (ret == ESP_OK) {
    ESP_LOGI(TAG, "OTA successful, restarting...");
    esp_restart();
  } else {
    ESP_LOGE(TAG, "OTA failed: %s", esp_err_to_name(ret));
  }

  vTaskDelete(NULL);
}

void app_main(void) {
  ESP_ERROR_CHECK(nvs_flash_init());
  wifi_init();

  /* Wait for WiFi connection before starting OTA */
  vTaskDelay(pdMS_TO_TICKS(5000));

  xTaskCreate(&ota_update_task, "ota_update_task", 8192, NULL, 5, NULL);
}
