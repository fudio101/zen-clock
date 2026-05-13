idf_component_register(
        SRCS "src/sntp_sync.c"
        INCLUDE_DIRS "include"
        REQUIRES esp_netif lwip freertos esp_event
)
