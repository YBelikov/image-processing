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
target_compile_features(imp_geometry PUBLIC cxx_std_20)
target_include_directories(imp_geometry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# ── Tests ─────────────────────────────────────

add_executable(unit_tests_imp_geometry tests/test_imp_geometry.cpp)
target_link_libraries(unit_tests_imp_geometry PRIVATE imp_geometry GTest::gtest GTest::gtest_main)

include(GoogleTest)
add_test(NAME ImpGeometryTests COMMAND unit_tests_imp_geometry)
