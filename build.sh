#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"

BUILD_TYPE="${1:-Debug}"
JOBS="$(nproc)"

echo "[noddle] Configuring (${BUILD_TYPE})..."
cmake -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DNODDLE_BUILD_TESTS=ON \
    -DNODDLE_WITH_OPENCV=ON \
    "$PROJECT_ROOT"

echo "[noddle] Building (${JOBS} jobs)..."
cmake --build "$BUILD_DIR" -j"$JOBS"

echo "[noddle] Running tests..."
cd "$BUILD_DIR" && ctest --output-on-failure

echo "[noddle] Build OK"
