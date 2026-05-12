# ZenClock UI component — modular LVGL interface

idf_component_register(
    SRCS "ui.c" "clock_face_text.c" "status_bar.c" "prov_screen.c"
    INCLUDE_DIRS "."
    REQUIRES lvgl bsp
)
