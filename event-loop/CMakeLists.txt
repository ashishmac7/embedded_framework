cmake_minimum_required(VERSION 3.10)

project(event_loop_lib)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/platform
)

# Source files
set(SOURCES
    source/event.c
    source/ns_timeout.c
    source/ns_timer.c
    source/system_timer.c
    platform/nsdynmemLIB_posix.c
    platform/platform_posix.c
)

add_library(event_loop_lib STATIC ${SOURCES})

target_include_directories(event_loop_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/platform
    ${CMAKE_CURRENT_SOURCE_DIR}/
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

# target_compile_definitions(event_loop_lib PUBLIC MBED_CONF_NANOSTACK_EVENTLOOP_USE_PLATFORM_TICK_TIMER)
