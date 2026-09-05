set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(aca_async_logger
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/aca_log_impl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/aca_ring_ds_impl.c
)

target_include_directories(aca_async_logger PRIVATE ${CMAKE_SOURCE_DIR})

find_package(Threads REQUIRED)
target_link_libraries(aca_async_logger PRIVATE Threads::Threads)

if (MSVC)
    # MSVC's C compiler has no <stdatomic.h>/_Atomic support, and aca_ring_ds.h needs
    # atomics for its own type definitions - so any unit that includes it must be built
    # as C++ on MSVC. The sources stay pure C (they build as C11 on Clang/GCC); MSVC just
    # compiles them with the C++ compiler. aca_ring_ds.h exposes extern "C" linkage so the
    # C-compiled aca_log_impl.c unit still links against everything.
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/main.c
        ${CMAKE_CURRENT_SOURCE_DIR}/aca_ring_ds_impl.c
        PROPERTIES LANGUAGE CXX)
endif()
