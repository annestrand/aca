cmake_minimum_required(VERSION 3.10)
project(aca)

# GoogleTest
set(CMAKE_CXX_STANDARD 11)
find_package(GTest REQUIRED)

set(TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# aca tests
add_executable(aca_tests)
target_sources(aca_tests PRIVATE
    ${TESTS_DIR}/argparse/aca_argparse.cpp
    ${TESTS_DIR}/argparse/test_bad_args.cpp
    ${TESTS_DIR}/argparse/test_good_args.cpp
)
target_include_directories(aca_tests PRIVATE ${CMAKE_SOURCE_DIR})
if (MSVC)
    target_compile_options(aca_tests PRIVATE /WX)
else()
    target_compile_options(aca_tests PRIVATE -Wall)
    target_compile_options(aca_tests PRIVATE -Werror)
    # Certain tests dont use all of the stub's functions - silence this
    target_compile_options(aca_tests PRIVATE "-Wno-unused-function")
endif()
target_link_libraries(aca_tests
    GTest::GTest
    GTest::Main
)