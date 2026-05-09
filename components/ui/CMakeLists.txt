file(GLOB_RECURSE UI_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

idf_component_register(
    SRCS ${UI_SRCS}
    INCLUDE_DIRS "." "components"
    REQUIRES lvgl
)
