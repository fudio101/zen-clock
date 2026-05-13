idf_component_register(
        SRCS "src/wifi_manager.c"
        "src/wifi_credentials.c"
        INCLUDE_DIRS "include" "../../include"
        PRIV_INCLUDE_DIRS "priv_include"
        REQUIRES esp_wifi esp_netif esp_event nvs_flash freertos lwip
)
