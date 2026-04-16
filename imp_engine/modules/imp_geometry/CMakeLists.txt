# ── imp_geometry ── dynamic library ───────────────────────────

set(IMP_GEOMETRY_HEADERS
    imp_geometry/Point.hpp
    imp_geometry/Transformation.hpp
    imp_geometry/AffineTransformation.hpp)

set(IMP_GEOMETRY_SOURCES
    src/AffineTransformation.cpp)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${IMP_GEOMETRY_HEADERS} ${IMP_GEOMETRY_SOURCES})

add_library(imp_geometry SHARED ${IMP_GEOMETRY_HEADERS} ${IMP_GEOMETRY_SOURCES})

# Public include: consumers use #include "imp_geometry/Point.hpp"
target_include_directories(imp_geometry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# OpenCV dependency
target_link_libraries(imp_geometry PRIVATE opencv::opencv)
target_include_directories(imp_geometry PRIVATE ${OpenCV_INCLUDE_DIRS})

set_target_properties(imp_geometry PROPERTIES
    XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++20"
    XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++"
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
    XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "YES"
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "YES"
)

# ── Tests ─────────────────────────────────────

add_executable(unit_tests_imp_geometry tests/test_imp_geometry.cpp)
target_link_libraries(unit_tests_imp_geometry PRIVATE imp_geometry GTest::gtest GTest::gtest_main)

set_target_properties(unit_tests_imp_geometry PROPERTIES
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
    XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "YES"
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "YES"
)

include(GoogleTest)
add_test(NAME ImpGeometryTests COMMAND unit_tests_imp_geometry)
