#!/usr/bin/env bash
# Simple test runner: executes all test binaries in build/tests

set -euo pipefail

BUILD_TEST_DIR="${BUILD_TEST_DIR:-$(pwd)/../build/tests}" # default relative path if not provided

if [ -d "${BUILD_TEST_DIR}" ]; then
  echo "Running tests in ${BUILD_TEST_DIR}"
  for t in "${BUILD_TEST_DIR}"/*; do
    if [ -x "${t}" ] && [ -f "${t}" ]; then
      echo "\n=== Running: ${t} ==="
      "${t}" || { echo "Test failed: ${t}"; exit 1; }
    fi
  done
  echo "\nAll tests executed."
else
  echo "Test build directory not found: ${BUILD_TEST_DIR}"
  echo "Please run cmake/make and ensure test binaries are built to \\${CMAKE_BINARY_DIR}/tests"
  exit 2
fi
