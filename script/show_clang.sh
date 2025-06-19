#!/bin/bash

# Check if an argument is provided
if [ "$#" -ne 1 ]; then
  echo "Usage: $0 <path-to-CMakePresets.json>"
  exit 1
fi

# Path to the CMakePresets.json file
CMAKE_PRESETS_FILE="$1"

# Check if the file exists
if [ ! -f "$CMAKE_PRESETS_FILE" ]; then
  echo "Error: File '$CMAKE_PRESETS_FILE' not found!"
  exit 1
fi

# Update 'hidden' to false for presets starting with "CLANG"
jq --indent 4 '(.configurePresets[] | select(.name | startswith("CLANG"))) |= . + {"hidden": false}' "$CMAKE_PRESETS_FILE" > tmp.json && mv tmp.json "$CMAKE_PRESETS_FILE"
rm -rf tmp.json

echo "Updated 'hidden' to false for all presets whose name starts with 'CLANG' in '$CMAKE_PRESETS_FILE'."
