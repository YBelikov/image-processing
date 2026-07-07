cmake_minimum_required(VERSION 3.15)
project(imp_engine VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Third-party dependencies ──────────────────
find_package(OpenCV REQUIRED)
find_package(GTest REQUIRED)
find_package(stb REQUIRED)

enable_testing()

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

set_target_properties(imp_engine PROPERTIES
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
    XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "YES"
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "YES"
)
