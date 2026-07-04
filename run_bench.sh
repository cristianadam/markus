#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

# Build if needed
if [ ! -f "${BUILD_DIR}/bench" ]; then
  echo "Building benchmark..."
  cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
  cmake --build "${BUILD_DIR}" --target bench -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)
  echo ""
fi

# Run benchmark
echo "Running benchmark..."
echo ""
"${BUILD_DIR}/bench" "$@"
