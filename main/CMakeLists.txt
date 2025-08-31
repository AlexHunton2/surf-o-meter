idf_component_register(
  SRCS "main.c" "ota.c" "som-http.c"
  INCLUDE_DIRS "."
  PRIV_REQUIRES "nvs_flash" "esp_wifi" "esp_http_client" "esp_https_ota" "json" "esp-tls"
)