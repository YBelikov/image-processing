#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_TYPE="${1:-Debug}"
TARGET="${2:-}"
BUILD_TYPE_LOWER="$(printf '%s' "${BUILD_TYPE}" | tr '[:upper:]' '[:lower:]')"
UNIVERSAL_DIR="${SCRIPT_DIR}/build_ninja/universal-${BUILD_TYPE_LOWER}"
ARCHS=(arm64 x86_64)
ARTIFACTS=(
    "bin/imp_engine"
    "bin/unit_tests_imp_algorithms"
    "bin/unit_tests_imp_geometry"
    "bin/unit_tests_imp_io"
    "lib/libimp_algorithms.dylib"
    "lib/libimp_geometry.dylib"
    "lib/libimp_io.dylib"
)

artifact_for_target() {
    case "$1" in
        imp_engine) echo "bin/imp_engine" ;;
        imp_algorithms) echo "lib/libimp_algorithms.dylib" ;;
        imp_geometry) echo "lib/libimp_geometry.dylib" ;;
        imp_io) echo "lib/libimp_io.dylib" ;;
        unit_tests_imp_algorithms) echo "bin/unit_tests_imp_algorithms" ;;
        unit_tests_imp_geometry) echo "bin/unit_tests_imp_geometry" ;;
        unit_tests_imp_io) echo "bin/unit_tests_imp_io" ;;
        *)
            echo "Unknown target: $1" >&2
            echo "Known targets: imp_engine imp_algorithms imp_geometry imp_io unit_tests_imp_algorithms unit_tests_imp_geometry unit_tests_imp_io" >&2
            exit 2
            ;;
    esac
}

case "${BUILD_TYPE}" in
    Debug|Release) ;;
    *)
        echo "Usage: $0 [Debug|Release] [target]" >&2
        exit 2
        ;;
esac

if [[ -n "${TARGET}" ]]; then
    ARTIFACTS=("$(artifact_for_target "${TARGET}")")
fi

"${SCRIPT_DIR}/configure_universal_ninja.sh" "${BUILD_TYPE}"

for ARCH in "${ARCHS[@]}"; do
    BUILD_ARGS=(--build --preset "ninja-${ARCH}-${BUILD_TYPE_LOWER}")
    if [[ -n "${TARGET}" ]]; then
        BUILD_ARGS+=(--target "${TARGET}")
    fi

    cmake "${BUILD_ARGS[@]}"
done

mkdir -p "${UNIVERSAL_DIR}/bin" "${UNIVERSAL_DIR}/lib"

for ARTIFACT in "${ARTIFACTS[@]}"; do
    ARM64_PATH="${SCRIPT_DIR}/build_ninja/arm64-${BUILD_TYPE_LOWER}/${ARTIFACT}"
    X86_64_PATH="${SCRIPT_DIR}/build_ninja/x86_64-${BUILD_TYPE_LOWER}/${ARTIFACT}"
    OUTPUT_PATH="${UNIVERSAL_DIR}/${ARTIFACT}"

    if [[ ! -f "${ARM64_PATH}" || ! -f "${X86_64_PATH}" ]]; then
        echo "Missing artifact for lipo merge: ${ARTIFACT}" >&2
        exit 1
    fi

    mkdir -p "$(dirname "${OUTPUT_PATH}")"
    lipo -create "${ARM64_PATH}" "${X86_64_PATH}" -output "${OUTPUT_PATH}"
done

echo "Universal ${BUILD_TYPE} artifacts are in ${UNIVERSAL_DIR}"
