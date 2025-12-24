#!/bin/bash
set -u

TIMEOUT_SECONDS=5
MAX_NUMBER=655
PASSED=0
FAILED=0

# If args are provided, run those test numbers; otherwise run 1..MAX_NUMBER
if [ "$#" -gt 0 ]; then
  TESTS=("$@")
  echo "Finding failing tests (timeout=${TIMEOUT_SECONDS}s, tests=${TESTS[*]})..."
else
  echo "Finding failing tests (timeout=${TIMEOUT_SECONDS}s, range=1-${MAX_NUMBER})..."
fi
echo "=============================================="

run_one_test() {
  local i="$1"

  output=$(timeout "$TIMEOUT_SECONDS" python3 commonmark-spec/test/spec_tests.py \
    --program bazel-bin/main \
    -s commonmark-spec/spec.txt \
    -n "$i" 2>&1)
  exit_code=$?

  if [ "$exit_code" -eq 124 ]; then
    echo "TIMEOUT: Test number $i timed out after ${TIMEOUT_SECONDS}s"
    python3 commonmark-spec/test/spec_tests.py \
      -s commonmark-spec/spec.txt \
      -n "$i" \
      --dump-tests 2>/dev/null
    echo "----------------------------------------------"
    ((FAILED++))
  elif [ "$exit_code" -ne 0 ]; then
    echo "ERROR: Test number $i exited with code ${exit_code}"
    python3 commonmark-spec/test/spec_tests.py \
      -s commonmark-spec/spec.txt \
      -n "$i" \
      --dump-tests 2>/dev/null

    echo "Program output:"
    timeout "$TIMEOUT_SECONDS" bazel-bin/main < <(python3 -c "
import sys
sys.path.insert(0, 'commonmark-spec/test')
from spec_tests import get_tests
tests = get_tests('commonmark-spec/spec.txt')
for t in tests:
    if t['example'] == $i:
        print(t['markdown'], end='')
        break
") 2>&1
    echo "----------------------------------------------"
    ((FAILED++))
  else
    ((PASSED++))
  fi
}

if [ "$#" -gt 0 ]; then
  for i in "${TESTS[@]}"; do
    run_one_test "$i"
  done
else
  for i in $(seq 1 "$MAX_NUMBER"); do
    run_one_test "$i"
  done
fi

echo "=============================================="
echo "SUMMARY:"
echo "  Passed: $PASSED"
echo "  Failed: $FAILED"
echo "  Total:  $((PASSED + FAILED))"
