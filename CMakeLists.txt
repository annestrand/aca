cmake_minimum_required(VERSION 3.10)

project(aca)
set(CMAKE_CXX_STANDARD 11)

# aca tests
add_executable(aca_tests)
target_sources(aca_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/argparse/aca_argparse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/argparse/test_bad_args.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/argparse/test_good_args.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/gdbstub/aca_gdbstub.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/gdbstub/test_common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/gdbstub/test_breakpoint.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/gdbstub/test_mem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/gdbstub/test_recv.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/gdbstub/test_regs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/gdbstub/test_send.cpp
)
target_include_directories(aca_tests PRIVATE ${CMAKE_SOURCE_DIR})
target_include_directories(aca_tests PRIVATE ${CMAKE_SOURCE_DIR}/tests/gdbstub)
if (MSVC)
    target_compile_options(aca_tests PRIVATE /WX)
else()
    target_compile_options(aca_tests PRIVATE -Wall)
    target_compile_options(aca_tests PRIVATE -Werror)
    # Certain tests dont use all of the stub's functions - silence this
    target_compile_options(aca_tests PRIVATE "-Wno-unused-function")
endif()

# GoogleTest
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/googletest)

target_link_libraries(aca_tests
    GTest::gtest
    GTest::gtest_main
)