#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <stdio.h>

#include "ota.h"
#include "som-ble.h"

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

static void ota_timer_callback(TimerHandle_t xTimer) {
  BaseType_t res =
      xTaskCreate(&ota_check_and_update_task, "ota_check_and_update_task", 8192, NULL, 5, NULL);
  if (res != pdPASS) {
    ESP_LOGE(TAG, "Failed to create OTA check task");
  }
}

void app_main(void) {
  ESP_ERROR_CHECK(nvs_flash_init());
  wifi_init();

  /* Wait for WiFi connection before starting OTA */
  vTaskDelay(pdMS_TO_TICKS(5000));

  // Update internal clock
  esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
  esp_sntp_setservername(0, "pool.ntp.org");
  esp_sntp_init();

  // OTA:
    #ifdef CONFIG_DEBUG_MODE
        ESP_LOGI(TAG, "Debug mode is enabled!");
        char tag_version[MAX_TAG_SIZE];
        read_curr_tag(tag_version, MAX_TAG_SIZE);

        if (strcmp(tag_version, "DEBUG") != 0) {
            save_curr_tag("DEBUG");
        }
    #else
        // Attempt OTA

        // Boot up OTA Attempt
        xTaskCreate(&ota_check_and_update_task, "ota_check_and_update_task", 8192, NULL, 5, NULL);

        // Callback OTA every so often
        ota_timer = xTimerCreate("ota_timer",
                               pdMS_TO_TICKS(3 * 60000), // 3 min
                               pdTRUE,               // auto-reload
                               NULL, ota_timer_callback);

        xTimerStart(ota_timer, 0);
    #endif

    ble_init_and_scan();
}
