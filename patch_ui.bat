@echo off
REM ============================================================
REM  ZenClock — Patch SquareLine UI CMakeLists.txt
REM  Run this after every SquareLine Studio export.
REM  It replaces the generated add_library() with
REM  idf_component_register() for ESP-IDF compatibility.
REM ============================================================

set UI_DIR=%~dp0components\ui
set CMAKE_FILE=%UI_DIR%\CMakeLists.txt

echo Patching %CMAKE_FILE% ...

(
echo file(GLOB_RECURSE UI_SRCS
echo     "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
echo ^)
echo.
echo idf_component_register(
echo     SRCS ${UI_SRCS}
echo     INCLUDE_DIRS "." "components"
echo     REQUIRES lvgl
echo ^)
) > "%CMAKE_FILE%"

echo Done! CMakeLists.txt patched for ESP-IDF.
