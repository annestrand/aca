cmake_minimum_required(VERSION 4.0)

project(aca)
set(CMAKE_CXX_STANDARD 11)

find_package(Git REQUIRED)
find_package(Python COMPONENTS Interpreter Development REQUIRED)

option(BUILD_EXAMPLES "Build the aca examples" ON)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/log/aca_log.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/log/test_log.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/ds/test_ring_ds.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/ds/aca_ring_ds.cpp
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
target_link_libraries(aca_tests GTest::gtest_main)

if (BUILD_EXAMPLES)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples)
endif()

# clang-tidy & clang-format integration
find_program(RUN_CLANG_TIDY_BIN NAMES run-clang-tidy run-clang-tidy.py)
if (RUN_CLANG_TIDY_BIN)
    message(STATUS "Found run-clang-tidy: ${RUN_CLANG_TIDY_BIN}")
    set(THIRD_PARTY_REGEX "^^(?!.*third_party)")
    add_custom_target(tidy
        COMMAND ${Python_EXECUTABLE} "${RUN_CLANG_TIDY_BIN}"
            "-p=${CMAKE_BINARY_DIR}"
            -fix -quiet -format
            "${THIRD_PARTY_REGEX}"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-tidy (with fix) on aca sources..."
    )
endif()
find_program(CLANG_FORMAT_EXE NAMES "clang-format")
if (CLANG_FORMAT_EXE)
    message(STATUS "Found clang-format: ${CLANG_FORMAT_EXE}")
    add_custom_target(format
        COMMAND ${GIT_EXECUTABLE} clang-format || ${CMAKE_COMMAND} -E true
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-format on aca sources..."
    )
endif()
