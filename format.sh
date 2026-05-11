#!/bin/bash
echo "Running cross-platform C/C++ formatter..."
python scripts/format.py "$@"
if [ $? -ne 0 ]; then
    echo "Formatting failed."
    exit 1
fi
echo "Done!"
