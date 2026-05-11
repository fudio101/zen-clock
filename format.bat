@echo off
echo Running cross-platform C/C++ formatter...
python scripts\format.py %*
if %errorlevel% neq 0 (
    echo Formatting failed.
    exit /b %errorlevel%
)
echo Done!
