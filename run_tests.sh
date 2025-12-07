#!/bin/bash

TIMEOUT_SECONDS=5
MAX_NUMBER=655
PASSED=0
FAILED=0

echo "Finding failing tests (timeout=${TIMEOUT_SECONDS}s, range=1-${MAX_NUMBER})..."
echo "=============================================="

for i in $(seq 1 $MAX_NUMBER); do
    output=$(timeout $TIMEOUT_SECONDS python3 commonmark-spec/test/spec_tests.py \
        --program bazel-out/aarch64-fastbuild/bin/main \
        -s commonmark-spec/spec.txt \
        -n $i 2>&1)
    exit_code=$?

    if [ $exit_code -eq 124 ]; then
        echo "TIMEOUT: Test number $i timed out after ${TIMEOUT_SECONDS}s"
        # Print input/output for timed out test
        python3 commonmark-spec/test/spec_tests.py \
            -s commonmark-spec/spec.txt \
            -n $i \
            --dump-tests 2>/dev/null
        echo "----------------------------------------------"
        ((FAILED++))
    elif [ $exit_code -ne 0 ]; then
        echo "ERROR: Test number $i exited with code ${exit_code}"
        # Print input/output for failing test
        python3 commonmark-spec/test/spec_tests.py \
            -s commonmark-spec/spec.txt \
            -n $i \
            --dump-tests 2>/dev/null
        # Print what the program actually produced
        echo "Program output:"
        timeout $TIMEOUT_SECONDS bazel-out/aarch64-fastbuild/bin/main < <(python3 -c "
import json
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
done

echo "=============================================="
echo "SUMMARY:"
echo "  Passed: $PASSED"
echo "  Failed: $FAILED"
echo "  Total:  $((PASSED + FAILED))"