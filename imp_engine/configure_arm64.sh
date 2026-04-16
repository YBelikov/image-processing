#!/bin/bash
set -e  # Exit on any error

# ─────────────────────────────────────────────
# Configuration
# ─────────────────────────────────────────────
BUILD_DIR="build_xcode"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# ─────────────────────────────────────────────
# Clean & prepare build directory
# ─────────────────────────────────────────────
echo "🧹 Cleaning build directory..."
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# ─────────────────────────────────────────────
# Conan install — Release & Debug
# ─────────────────────────────────────────────
CONAN_COMMON_ARGS=(
    "$SCRIPT_DIR"
    --output-folder=.
    --build=missing
    -s compiler.libcxx=libc++
)

echo "Installing Conan dependencies (Release)..."
conan install "${CONAN_COMMON_ARGS[@]}" -s build_type=Release

echo "Installing Conan dependencies (Debug)..."
conan install "${CONAN_COMMON_ARGS[@]}" -s build_type=Debug

# ─────────────────────────────────────────────
# Generate Xcode project
# ─────────────────────────────────────────────
echo "Generating Xcode project..."
cmake "$SCRIPT_DIR" \
    -G Xcode \
    -DCMAKE_TOOLCHAIN_FILE="$(pwd)/conan_toolchain.cmake"
