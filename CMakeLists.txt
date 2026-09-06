cmake_minimum_required(VERSION 3.10)

project(aca)
set(CMAKE_CXX_STANDARD 11)

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
get_target_property(ACA_TEST_SOURCES aca_tests SOURCES)
get_target_property(ACA_ASYNC_LOGGER_SOURCES aca_async_logger SOURCES)
find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
if (CLANG_TIDY_EXE)
    if (ENABLE_LINT OR BUILD_TYPE_LOWER MATCHES "release")
        message(STATUS "Clang-Tidy: Enabled")
        set_target_properties(aca_tests PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
    endif()
    add_custom_target(tidy
        COMMAND ${CLANG_TIDY_EXE} 
            "-p=${CMAKE_BINARY_DIR}" 
            ${ACA_TEST_SOURCES} 
            ${ACA_ASYNC_LOGGER_SOURCES} 
            -fix -quiet
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-tidy (with fix) on aca sources..."
    )
endif()
find_program(CLANG_FORMAT_EXE NAMES "clang-format")
if (CLANG_FORMAT_EXE)
    message(STATUS "Clang-Format: Enabled")
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} 
            -i 
            ${ACA_TEST_SOURCES}
            ${ACA_ASYNC_LOGGER_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-format on aca sources..."
    )
endif()
