#!/bin/bash

# Script to find the first test number (1-655) that times out
# Timeout is set to 10 seconds

TIMEOUT_SECONDS=10
MAX_NUMBER=655

echo "Searching for first timeout (timeout=${TIMEOUT_SECONDS}s, range=1-${MAX_NUMBER})..."
echo "=============================================="

for i in $(seq 1 $MAX_NUMBER); do
    # Run the test with timeout
    timeout $TIMEOUT_SECONDS python3 commonmark-spec/test/spec_tests.py \
        --program bazel-out/aarch64-fastbuild/bin/main \
        -s commonmark-spec/spec.txt \
        -n $i > /dev/null 2>&1
    
    exit_code=$?
    
    # timeout returns 124 when the command times out
    if [ $exit_code -eq 124 ]; then
        echo "TIMEOUT FOUND: Test number $i timed out after ${TIMEOUT_SECONDS} seconds"
        exit 0
    fi
    
    # Print progress every 50 tests
    if [ $((i % 50)) -eq 0 ]; then
        echo "Progress: Checked $i tests so far (no timeout yet)..."
    fi
done

echo "=============================================="
echo "No timeouts found in tests 1-${MAX_NUMBER}"