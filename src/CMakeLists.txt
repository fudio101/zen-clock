# ZenClock main component

idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "." "../include"
    REQUIRES bsp lvgl esp_lvgl_port ui settings
)
