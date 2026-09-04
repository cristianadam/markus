#!/bin/bash -eu
# Build-and-run helper for the markus fuzz targets.
#
# Recommended (local): the standalone driver + AFL++. The standalone driver
# builds with any compiler (use the fast default one) and AFL's per-exec
# timeout keeps the quadratic worst-cases from stalling the run:
#   cmake -S . -B build -DBUILD_TESTING=OFF -DMARKUS_BUILD_FUZZER=ON
#   cmake --build build --target fuzz_markus_standalone
#   ./fuzz/run_fuzz.sh afl
#
# libFuzzer (best in CI / Linux, where the per-unit -timeout works and a
# normal clang ships the fuzzer runtime): configure the whole tree with a
# fuzzer-capable compiler, then build the fuzz_markus target:
#   cmake -S . -B build-fuzz -DBUILD_TESTING=OFF \
#         -DCMAKE_CXX_COMPILER=/path/to/fuzzer-capable/clang++ \
#         -DMARKUS_BUILD_FUZZER=ON -DMARKUS_BUILD_FUZZER_LIBFUZZER=ON
#   cmake --build build-fuzz --target fuzz_markus
#   BUILDDIR=build-fuzz ./fuzz/run_fuzz.sh libfuzzer
#
# Usage:
#   ./fuzz/run_fuzz.sh replay [corpus_dir]
#       Replay the seed corpus through the standalone driver. A quick smoke
#       test that catches crashes on known-good inputs before fuzzing.
#   ./fuzz/run_fuzz.sh libfuzzer [corpus_dir] [libFuzzer args...]
#       Run the libFuzzer harness (needs the fuzzer-capable build).
#   ./fuzz/run_fuzz.sh afl [corpus_dir]
#       Run the standalone driver under AFL++ (recommended locally).

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILDDIR="${BUILDDIR:-${SCRIPT_DIR}/../build}"
CORPUS="${SCRIPT_DIR}/corpus"
DICT="${SCRIPT_DIR}/fuzzing_dictionary"

# LeakSanitizer is only available on Linux ASan; forcing it on macOS makes
# Apple's ASan abort, so enable it there only.
case "$(uname -s)" in
  Linux) DEFAULT_ASAN_OPTS="detect_leaks=1:quarantine_size_mb=10" ;;
  *)     DEFAULT_ASAN_OPTS="quarantine_size_mb=10" ;;
esac

cmd="${1:-replay}"
shift || true

case "$cmd" in
  replay)
    corpus="${1:-$CORPUS}"
    bin="$BUILDDIR/fuzz_markus_standalone"
    [ -x "$bin" ] || { echo "build first: cmake -S . -B build -DMARKUS_BUILD_FUZZER=ON && cmake --build build --target fuzz_markus_standalone" >&2; exit 1; }
    echo "Replaying '$corpus' with $bin"
    ASAN_OPTIONS="${ASAN_OPTIONS:-$DEFAULT_ASAN_OPTS}" "$bin" "$corpus"
    echo "OK: no crashes replaying '$corpus'"
    ;;
  libfuzzer)
    corpus="${1:-$CORPUS}"
    shift || true
    bin="$BUILDDIR/fuzz_markus"
    [ -x "$bin" ] || { echo "build first: configure with a fuzzer-capable CMAKE_CXX_COMPILER and -DMARKUS_BUILD_FUZZER_LIBFUZZER=ON, then 'cmake --build $BUILDDIR --target fuzz_markus'" >&2; exit 1; }
    ASAN_OPTIONS="${ASAN_OPTIONS:-$DEFAULT_ASAN_OPTS}" \
      "$bin" "$corpus" -max_len=1024 -timeout=10 -dict="$DICT" "$@"
    ;;
  afl)
    corpus="${1:-$CORPUS}"
    shift || true
    bin="$BUILDDIR/fuzz_markus_standalone"
    [ -x "$bin" ] || { echo "build first: cmake -S . -B build -DMARKUS_BUILD_FUZZER=ON && cmake --build build --target fuzz_markus_standalone" >&2; exit 1; }
    command -v afl-fuzz >/dev/null 2>&1 || { echo "afl-fuzz not found on PATH" >&2; exit 1; }
    out="${AFL_OUT:-${SCRIPT_DIR}/afl_out}"
    # AFL++ needs symbolize=0 when the target is ASan-instrumented. For best
    # coverage build the standalone target with afl-clang-fast instead (see
    # fuzz/CMakeLists.txt header); a plain ASan build runs in dumb mode.
    ASAN_OPTIONS="detect_leaks=0:abort_on_error=1:symbolize=0" \
      AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1 \
      # -n = dumb mode: the standalone target is ASan (not AFL) instrumented, so
      # AFL falls back to timeout/crash-only discovery. For edge-guided fuzzing
      # build it with afl-clang-fast and drop the -n.
      afl-fuzz -n -i "$corpus" -o "$out" "$bin" @@ "$@"
    ;;
  *)
    echo "unknown command: $cmd (use: replay | libfuzzer | afl)" >&2
    exit 2
    ;;
esac
