# ── imp_io ── dynamic library ─────────────────────────────────

set(IMP_IO_HEADERS
    imp_io/CodecResult.hpp
    imp_io/DecodedImage.hpp
    imp_io/ImageCodec.hpp
    imp_io/ImageData.hpp
    imp_io/ImageMetadata.hpp
    imp_io/ImageReader.hpp
    imp_io/ImageWriter.hpp
    imp_io/JpegCodec.hpp
    imp_io/PngCodec.hpp
    imp_io/Pixel.hpp)

set(IMP_IO_SOURCES
    src/DecodedImage.cpp
    src/ExifOrientation.hpp
    src/ExifOrientation.cpp
    src/ImageCodec.cpp
    src/JpegCodec.cpp
    src/PngCodec.cpp
    src/jpeg/JpegDriver.hpp
    src/jpeg/JpegDriver.cpp
    src/jpeg/JpegBridge.h
    src/jpeg/JpegBridge.c
    src/png/PngDriver.hpp
    src/png/PngDriver.cpp
    src/png/PngBridge.h
    src/png/PngBridge.c
    src/stb_impl.cpp)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${IMP_IO_HEADERS} ${IMP_IO_SOURCES})

add_library(imp_io SHARED ${IMP_IO_HEADERS} ${IMP_IO_SOURCES})

# Public include: consumers use #include "imp_io/ImageReader.hpp"
find_package(stb REQUIRED)
find_package(PNG 1.6.58 EXACT REQUIRED)
find_package(libjpeg-turbo 3.1.4.1 EXACT REQUIRED CONFIG)
target_compile_features(imp_io PUBLIC cxx_std_20)
target_compile_features(imp_io PRIVATE c_std_99)
target_include_directories(imp_io PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${stb_INCLUDE_DIRS}
)

# stb is used by templated public headers, so consumers need its usage requirements.
target_link_libraries(imp_io PUBLIC stb::stb)
target_link_libraries(imp_io PRIVATE PNG::PNG)
target_link_libraries(imp_io PRIVATE libjpeg-turbo::jpeg-static)

# ── Tests ─────────────────────────────────────

add_executable(unit_tests_imp_io
    tests/test_imp_io.cpp
    tests/test_decoded_image.cpp
    tests/test_image_codec.cpp
    tests/test_jpeg_codec.cpp
    tests/test_png_codec.cpp)
target_link_libraries(unit_tests_imp_io PRIVATE imp_io GTest::gtest GTest::gtest_main)
target_compile_definitions(unit_tests_imp_io PRIVATE
    IMP_IO_JPEG_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/jpeg"
    IMP_IO_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/png")

include(GoogleTest)
add_test(NAME ImpIoTests COMMAND unit_tests_imp_io)
