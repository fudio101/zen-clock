idf_component_register(
    SRCS "src/ble_provisioning.c"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash esp_event bt
    PRIV_REQUIRES espressif__network_provisioning
)
