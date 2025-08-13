#ifndef OTA_H
#define OTA_H

#include "cJSON.h"
#include "esp_crt_bundle.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <stdio.h>

#include "som-http.h"

#define GITHUB_API_URL "https://api.github.com/repos/"
#define GITHUB_URL "https://github.com/"
#define LATEST_RELEASE_PATH "releases/latest"
#define LATEST_DOWNLOAD_PATH "releases/latest/download/"

#define GITHUB_USER CONFIG_GITHUB_USER
#define GITHUB_REPO CONFIG_GITHUB_REPO
#define BIN_FILENAME CONFIG_BIN_FILENAME

#define MAX_TAG_SIZE 64

void ota_update_task(void *pvParameter);
void ota_check_task(void *pvParameter);

#endif
