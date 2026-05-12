#!/bin/bash
echo "Running cross-platform C/C++ formatter..."
PYTHON=$(command -v python3 || command -v python) && "$PYTHON" scripts/format.py "$@"
if [ $? -ne 0 ]; then
    echo "Formatting failed."
    exit 1
fi
echo "Done!"
