# ── imp_io ── dynamic library ─────────────────────────────────

set(IMP_IO_HEADERS
    imp_io/ImageData.hpp
    imp_io/ImageReader.hpp
    imp_io/ImageWriter.hpp
    imp_io/Pixel.hpp)

set(IMP_IO_SOURCES
    src/stb_impl.cpp)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${IMP_IO_HEADERS} ${IMP_IO_SOURCES})

add_library(imp_io SHARED ${IMP_IO_HEADERS} ${IMP_IO_SOURCES})

# Public include: consumers use #include "imp_io/ImageReader.hpp"
find_package(stb REQUIRED)
target_compile_features(imp_io PUBLIC cxx_std_20)
target_include_directories(imp_io PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${stb_INCLUDE_DIRS}
)

# stb is used by templated public headers, so consumers need its usage requirements.
target_link_libraries(imp_io PUBLIC stb::stb)

# ── Tests ─────────────────────────────────────

add_executable(unit_tests_imp_io tests/test_imp_io.cpp)
target_link_libraries(unit_tests_imp_io PRIVATE imp_io GTest::gtest GTest::gtest_main)

include(GoogleTest)
add_test(NAME ImpIoTests COMMAND unit_tests_imp_io)
