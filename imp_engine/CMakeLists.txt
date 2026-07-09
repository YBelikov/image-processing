cmake_minimum_required(VERSION 3.15)
project(imp_engine VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ── Third-party dependencies ──────────────────
find_package(GTest REQUIRED)
find_package(stb REQUIRED)

include(CTest)

# ── Modules (dynamic libraries) ───────────────
add_subdirectory(modules/imp_geometry)
add_subdirectory(modules/imp_io)
add_subdirectory(modules/imp_algorithms)

# ── Main executable ──────────────────────────
add_executable(imp_engine
    src/main.cpp
)

target_include_directories(imp_engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)

target_link_libraries(imp_engine PRIVATE imp_geometry imp_io imp_algorithms)
