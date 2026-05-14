#!/usr/bin/env bash
set -euo pipefail

PLUGINS="${1:-*}"
BUILD_DIR="${2:-build}"

cmake -S . -B "${BUILD_DIR}" -DBUSINESS_PLUGIN_FILTER="${PLUGINS}"
cmake --build "${BUILD_DIR}"
