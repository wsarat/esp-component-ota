idf_component_register(SRCS "ota.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "app_update" "esp_driver_gpio" "mbedtls" "esp_partition" "esp_https_ota")
