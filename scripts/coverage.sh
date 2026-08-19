#!/bin/bash
# Generates the coverage report. Run from the repo root: ./scripts/coverage.sh
#This exists as a plain shell script (rather than inline in CMakeLists.txt because CMake's add_custom_target() passes arguments through generator specific escaping that mangled the exclude regex patterns here. this exact command is the one verified working directly in a terminal.
set -e

BUILD_DIR="build-coverage"

if [ ! -d "$BUILD_DIR" ]; then
    echo "No $BUILD_DIR directory found. Run this first:"
    echo "  cmake -B $BUILD_DIR -DCMAKE_BUILD_TYPE=Coverage"
    echo "  cmake --build $BUILD_DIR"
    echo "  ctest --test-dir $BUILD_DIR"
    exit 1
fi

mkdir -p "$BUILD_DIR/coverage-report"

gcovr --root . \
      --exclude '.*/tests/.*' \
      --exclude '.*/_deps/.*' \
      --exclude 'main\.cpp' \
      --object-directory "$BUILD_DIR" \
      --print-summary \
      --html --html-details \
      -o "$BUILD_DIR/coverage-report/index.html"

echo ""
echo "HTML report: $BUILD_DIR/coverage-report/index.html"