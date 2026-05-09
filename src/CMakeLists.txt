# ZemClock main component

idf_component_register(
    SRCS
        "main.c"
        "display/lcd_driver.c"
        "display/lcd_graphics.c"
        "display/lcd_text.c"
        "fonts/font_8x8.c"
        "ui/ui_manager.c"
    INCLUDE_DIRS "../include"
    REQUIRES driver esp_lcd
)
