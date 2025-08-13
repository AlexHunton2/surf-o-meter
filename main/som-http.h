#ifndef SOM_HTTP_H
#define SOM_HTTP_H

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "esp_system.h"
#include "esp_tls.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_http_client.h"

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 4096

esp_err_t http_event_handler(esp_http_client_event_t *evt);

#endif
