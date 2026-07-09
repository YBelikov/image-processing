#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_TYPE="${1:-Debug}"
BUILD_TYPE_LOWER="$(printf '%s' "${BUILD_TYPE}" | tr '[:upper:]' '[:lower:]')"
UNIVERSAL_DIR="${SCRIPT_DIR}/build_ninja/universal-${BUILD_TYPE_LOWER}"
ARCHS=(arm64 x86_64)

case "${BUILD_TYPE}" in
    Debug|Release) ;;
    *)
        echo "Usage: $0 [Debug|Release]" >&2
        exit 2
        ;;
esac

mkdir -p "${UNIVERSAL_DIR}/bin" "${UNIVERSAL_DIR}/lib"

for ARCH in "${ARCHS[@]}"; do
    BUILD_DIR="${SCRIPT_DIR}/build_ninja/${ARCH}-${BUILD_TYPE_LOWER}"
    PRESET="ninja-${ARCH}-${BUILD_TYPE_LOWER}"
    CONAN_ARCH="${ARCH}"

    if [[ "${ARCH}" == "arm64" ]]; then
        CONAN_ARCH="armv8"
    fi

    mkdir -p "${BUILD_DIR}"

    conan install "${SCRIPT_DIR}" \
        --output-folder="${BUILD_DIR}" \
        --build=missing \
        -s arch="${CONAN_ARCH}" \
        -s build_type="${BUILD_TYPE}" \
        -s compiler.libcxx=libc++ \
        -c tools.cmake.cmaketoolchain:generator=Ninja

    cmake --preset "${PRESET}"
done

echo "Configured universal Ninja builds:"
echo "  cmake --build --preset ninja-arm64-${BUILD_TYPE_LOWER}"
echo "  cmake --build --preset ninja-x86_64-${BUILD_TYPE_LOWER}"
echo
echo "Build and merge universal artifacts with:"
echo "  ./build_universal_ninja.sh ${BUILD_TYPE}"
