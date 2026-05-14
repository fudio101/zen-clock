idf_component_register(
        SRCS "src/deep_sleep.c"
        INCLUDE_DIRS "include" "../../include"
        REQUIRES bsp esp_timer freertos esp_hw_support driver
)
