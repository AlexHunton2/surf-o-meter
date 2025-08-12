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

#include "ota.h"

#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PASS CONFIG_WIFI_PASS

static const char *TAG = "MAIN";

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

static TimerHandle_t ota_timer;
static char current_tag[64] = "";

static void ota_timer_callback(TimerHandle_t xTimer) {
  // Already updating...
  if (xTaskGetHandle("ota_update_task") != NULL) {
    return;
  }

  char latest_tag[64];
  if (get_latest_github_tag(latest_tag, sizeof(latest_tag))) {
    if (strcmp(current_tag, latest_tag) != 0) {
      strcpy(current_tag, latest_tag);
      xTaskCreate(&ota_update_task, "ota_update_task", 8192, NULL, 5, NULL);
    } else {
      ESP_LOGI(TAG, "Software up to date.");
    }
  } else {
    ESP_LOGE(TAG, "Failed to get latest GitHub tag");
  }
}

void app_main(void) {
  ESP_ERROR_CHECK(nvs_flash_init());
  wifi_init();

  /* Wait for WiFi connection before starting OTA */
  vTaskDelay(pdMS_TO_TICKS(5000));

  ota_timer = xTimerCreate("ota_timer",
                           pdMS_TO_TICKS(120000), // 2 min
                           pdTRUE,                // auto-reload
                           NULL, ota_timer_callback);

  xTimerStart(ota_timer, 0);
}
