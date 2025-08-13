#include "ota.h"
#include "esp_http_client.h"

static const char *TAG = "OTA";

void ota_update_task(void *pvParameter) {
#define OTA_URL                                                                \
  GITHUB_URL GITHUB_USER "/" GITHUB_REPO "/" LATEST_DOWNLOAD_PATH BIN_FILENAME

  ESP_LOGI(TAG, "Starting OTA update from: %s", OTA_URL);

  esp_http_client_config_t http_cfg = {
      .url = OTA_URL,
      .crt_bundle_attach = esp_crt_bundle_attach,
      .buffer_size_tx = 2048,              // Increase TX buffer
      .buffer_size = 2048,                 // Increase RX buffer
      .skip_cert_common_name_check = true, // if cert issues occur
      .timeout_ms = 10000,                 // avoid timeout
      .keep_alive_enable = true};

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

esp_err_t _save_curr_tag(const char *tag) {
  nvs_handle_t handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);
  if (err != ESP_OK)
    return err;

  err = nvs_set_str(handle, "curr_tag", tag);
  if (err == ESP_OK)
    err = nvs_commit(handle);

  nvs_close(handle);
  return err;
}

esp_err_t _read_curr_tag(char *tag, size_t max_len) {
  nvs_handle_t handle;
  esp_err_t err = nvs_open("storage", NVS_READONLY, &handle);
  if (err != ESP_OK)
    return err;

  size_t required_size = max_len;
  err = nvs_get_str(handle, "curr_tag", tag, &required_size);

  nvs_close(handle);
  return err;
}

bool _get_latest_github_tag(char *tag_buf, size_t buf_size) {
  char response_buf[MAX_HTTP_OUTPUT_BUFFER + 1] = {0};

#define TAG_URL                                                                \
  GITHUB_API_URL GITHUB_USER "/" GITHUB_REPO "/" LATEST_RELEASE_PATH
  esp_http_client_config_t http_cfg = {
      .url = TAG_URL,
      .crt_bundle_attach = esp_crt_bundle_attach,
      .event_handler = http_event_handler,
      .user_data = response_buf,
  };

  esp_http_client_handle_t client = esp_http_client_init(&http_cfg);
  esp_http_client_set_header(client, "User-Agent", "esp32-ota-client");

  if (esp_http_client_perform(client) != ESP_OK) {
    ESP_LOGE(TAG, "HTTP perform failed");
    esp_http_client_cleanup(client);
    return false;
  }

  int status = esp_http_client_get_status_code(client);
  if (status != 200) {
    ESP_LOGE(TAG, "HTTP status code %d", status);
    esp_http_client_cleanup(client);
    return false;
  }

  // Done with request, clean up
  esp_http_client_cleanup(client);

  cJSON *json = cJSON_Parse(response_buf);
  if (!json) {
    ESP_LOGE(TAG, "Failed to parse JSON");
    return false;
  }

  cJSON *tag_name = cJSON_GetObjectItem(json, "tag_name");
  if (!tag_name || tag_name->type != cJSON_String) {
    ESP_LOGE(TAG, "No tag_name in JSON");
    cJSON_Delete(json);
    return false;
  }
  ESP_LOGI(TAG, "Version Tag Found: %s", tag_name->valuestring);

  strncpy(tag_buf, tag_name->valuestring, buf_size - 1);
  tag_buf[buf_size - 1] = '\0';

  cJSON_Delete(json);
  return true;
}

void ota_check_task(void *pvParameter) {
  char latest_tag[MAX_TAG_SIZE];
  char curr_tag[MAX_TAG_SIZE];

  _read_curr_tag(curr_tag, MAX_TAG_SIZE);

  ESP_LOGI(TAG, "Current version: %s", curr_tag);
  ESP_LOGI(TAG, "Checking for firmware update...");

  if (xTaskGetHandle("ota_update_task") != NULL) {
    ESP_LOGI(TAG, "OTA update already running, skipping check.");
    vTaskDelete(NULL);
    return;
  }

  if (_get_latest_github_tag(latest_tag, sizeof(latest_tag))) {
    if (strcmp(curr_tag, latest_tag) != 0) {
      _save_curr_tag(latest_tag);
      xTaskCreate(&ota_update_task, "ota_update_task", 8192, latest_tag, 5,
                  NULL);
    } else {
      ESP_LOGI(TAG, "Software up to date.");
    }
  } else {
    ESP_LOGE(TAG, "Failed to get latest GitHub tag");
  }

  vTaskDelete(NULL);
}
