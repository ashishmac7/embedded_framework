cmake_minimum_required(VERSION 3.10)



set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -Og -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -Og -fno-omit-frame-pointer")

project(event_loop VERSION 1.3.40)


# Add subdirectory for event-loop library
add_subdirectory(event-loop)

# Examples
# Each example is built as a separate executable
# add_executable(basic_timer examples/basic_timer.c common/ns_list.c)
# target_include_directories(basic_timer PRIVATE . 
#     event-loop/include
# )
# target_link_libraries(basic_timer PRIVATE event_loop_lib)

add_executable(event_scheduler examples/event_scheduler.c common/ns_list.c)
target_include_directories(event_scheduler PRIVATE . 
    event-loop/include
)
target_link_libraries(event_scheduler PRIVATE event_loop_lib)