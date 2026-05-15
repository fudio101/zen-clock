# ZenClock UI component — modular LVGL interface

idf_component_register(
        SRCS "ui.c" "clock_face_text.c" "status_bar.c" "prov_screen.c"
        "nav.c" "menu_screen.c" "settings_screen.c" "device_info_screen.c" "ui_utils.c"
        INCLUDE_DIRS "."
        REQUIRES lvgl bsp settings deep_sleep wifi_manager
)
