#ifndef OTA_H
#define OTA_H

#include "cJSON.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <stdio.h>

#define OTA_URL CONFIG_OTA_URL

/* Embed the server certificate in the binary */
extern const uint8_t
    ca_cert_pem_start[] asm("_binary_digicert_global_root_g2_pem_start");
extern const uint8_t
    ca_cert_pem_end[] asm("_binary_digicert_global_root_g2_pem_end");

void ota_update_task(void *pvParameter);
bool get_latest_github_tag(char *tag_buf, size_t buf_size);

#endif
