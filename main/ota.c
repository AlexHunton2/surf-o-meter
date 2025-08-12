#include "ota.h"
#include "sdkconfig.h"

static const char *TAG = "OTA";

void ota_update_task(void *pvParameter) {
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

bool get_latest_github_tag(char *tag_buf, size_t buf_size) {
  esp_http_client_config_t http_cfg = {
      .url = OTA_URL,
      .cert_pem = (const char *)ca_cert_pem_start,
  };

  esp_http_client_handle_t client = esp_http_client_init(&http_cfg);

  // Set User-Agent header (GitHub API requires this)
  esp_http_client_set_header(client, "User-Agent", "esp32-ota-client");

  if (esp_http_client_perform(client) != ESP_OK) {
    esp_http_client_cleanup(client);
    return false;
  }

  int len = esp_http_client_get_content_length(client);
  if (len <= 0 || len >= 1024) { // Arbitrary limit to avoid big allocs
    esp_http_client_cleanup(client);
    return false;
  }

  char *buffer = malloc(len + 1);
  if (!buffer) {
    esp_http_client_cleanup(client);
    return false;
  }

  int read_len = esp_http_client_read(client, buffer, len);
  buffer[read_len] = '\0';

  esp_http_client_cleanup(client);

  // Parse JSON to extract tag_name
  cJSON *json = cJSON_Parse(buffer);
  free(buffer);
  if (!json)
    return false;

  cJSON *tag_name = cJSON_GetObjectItem(json, "tag_name");
  if (!tag_name || tag_name->type != cJSON_String) {
    cJSON_Delete(json);
    return false;
  }

  strncpy(tag_buf, tag_name->valuestring, buf_size - 1);
  tag_buf[buf_size - 1] = '\0';

  cJSON_Delete(json);
  return true;
}
