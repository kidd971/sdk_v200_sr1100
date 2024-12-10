#!/bin/bash

# Usage: ./check_venv.sh [base_dir]
# If base_dir is not provided, it defaults to an empty string (no directory prefix)

# Set the base directory (empty if not provided)
BASE_DIR=${1:-}

# Define the target directory
TARGET_DIR="${BASE_DIR}script"

echo "Comparing environment.yml files..."
if ! diff /environment.yml "${TARGET_DIR}/environment.yml"; then
  echo "environment.yml files differ"
  exit 1
fi

echo "Comparing bootstrap.sh files..."
if ! diff /bootstrap.sh "script/bootstrap.sh"; then
  echo "bootstrap.sh files differ"
  exit 1
fi

echo "All files are identical."
