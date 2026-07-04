#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

# Build if needed
if [ ! -f "${BUILD_DIR}/main" ] || [ ! -f "${BUILD_DIR}/test_markus" ]; then
  cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
  cmake --build "${BUILD_DIR}" -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)
fi

# If specific test numbers are provided, convert to ctest regex filter
if [ "$#" -gt 0 ]; then
  # Convert test numbers like "1 2 3" to regex "Example_(1|2|3)$"
  regex="Example_($(echo "$@" | tr ' ' '|'))$"
  echo "Running specific tests matching: ${regex}"
  ctest --test-dir "${BUILD_DIR}" -R "${regex}" --output-on-failure
else
  echo "Running all CommonMark spec tests..."
  ctest --test-dir "${BUILD_DIR}" --output-on-failure
fi
