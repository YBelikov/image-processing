#!/bin/bash
set -euo pipefail

# ─────────────────────────────────────────────
# Configuration
# ─────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_TYPE="${1:-Debug}"
BUILD_TYPE_LOWER="$(printf '%s' "${BUILD_TYPE}" | tr '[:upper:]' '[:lower:]')"
BUILD_DIR="${SCRIPT_DIR}/build_ninja/arm64-${BUILD_TYPE_LOWER}"
PRESET="ninja-arm64-${BUILD_TYPE_LOWER}"

case "${BUILD_TYPE}" in
    Debug|Release) ;;
    *)
        echo "Usage: $0 [Debug|Release]" >&2
        exit 2
        ;;
esac

# ─────────────────────────────────────────────
# Prepare build directory
# ─────────────────────────────────────────────
mkdir -p "${BUILD_DIR}"

# ─────────────────────────────────────────────
# Conan install
# ─────────────────────────────────────────────
conan install "${SCRIPT_DIR}" \
    --output-folder="${BUILD_DIR}" \
    --build=missing \
    -s arch=armv8 \
    -s build_type="${BUILD_TYPE}" \
    -s compiler.libcxx=libc++ \
    -c tools.cmake.cmaketoolchain:generator=Ninja

# ─────────────────────────────────────────────
# Generate Ninja build files
# ─────────────────────────────────────────────
cmake --preset "${PRESET}"
