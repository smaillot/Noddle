#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
APP_BIN="${BUILD_DIR}/app/noddle_app"

# Build if needed
if [[ ! -f "$APP_BIN" ]]; then
    echo "[noddle] Binary not found, building..."
    cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug -DNODDLE_WITH_OPENCV=ON "$PROJECT_ROOT"
    cmake --build "$BUILD_DIR" -j"$(nproc)"
fi

# Set library path for QtNodes shared lib
export LD_LIBRARY_PATH="${BUILD_DIR}/lib:${LD_LIBRARY_PATH:-}"

echo "[noddle] Starting Noddle..."
exec "$APP_BIN" "$@"
