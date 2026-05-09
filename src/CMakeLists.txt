# ZenClock main component

idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "../include"
    REQUIRES tdisplays3 lvgl esp_lvgl_port ui
)
