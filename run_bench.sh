#!/bin/bash
set -euo pipefail

# Build cmark if not already built
CMARK_BUILD="bazel-cmark"
if [ ! -f "$CMARK_BUILD/src/cmark" ]; then
  echo "Building cmark..."
  cmake -S commonmark-spec -B "$CMARK_BUILD" -DCMAKE_BUILD_TYPE=Release
  cmake --build "$CMARK_BUILD" -j$(sysctl -n hw.ncpu 2>/dev/null || nproc)
  echo ""
fi

# Build benchmark
echo "Building benchmark..."
CXX=${CXX:-clang++}
$CXX -std=c++20 -O2 -I. -Icommonmark-spec/src -I"$CMARK_BUILD/src" \
  bench.cc "$CMARK_BUILD/src/libcmark.a" \
  -o bazel-bin/bench

echo "Running benchmark..."
echo ""
bazel-bin/bench "$@"
