set(IMP_ALGORITHMS_HEADERS
    imp_algorithms/Composition.hpp)

set(IMP_ALGORITHMS_SOURCES
    src/Composition.cpp)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${IMP_ALGORITHMS_HEADERS}  ${IMP_ALGORITHMS_SOURCES})

add_library(imp_algorithms SHARED ${IMP_ALGORITHMS_HEADERS} ${IMP_ALGORITHMS_SOURCES})

target_include_directories(imp_algorithms PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(imp_algorithms PUBLIC imp_io)

add_executable(unit_tests_imp_algorithms tests/test_composition.cpp)
target_link_libraries(unit_tests_imp_algorithms PRIVATE
    imp_algorithms
    GTest::gtest
    GTest::gtest_main)

set_target_properties(unit_tests_imp_algorithms PROPERTIES
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
    XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "YES"
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "YES"
)

include(GoogleTest)
add_test(NAME ImpAlgorithmsTests COMMAND unit_tests_imp_algorithms)
