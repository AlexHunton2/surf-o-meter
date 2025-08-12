idf_component_register(
  SRCS "main.c" "ota.c"
  INCLUDE_DIRS "."
  EMBED_FILES "../certs/digicert_global_root_g2.pem"
  PRIV_REQUIRES "nvs_flash" "esp_wifi" "esp_http_client" "esp_https_ota" "json"
)
