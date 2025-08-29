cmake_minimum_required(VERSION 3.10)


project(event_loop VERSION 1.3.40)

add_executable(event_loop main.c ns_list.c
                nanostack-event-loop/source/event.c
                nanostack-event-loop/source/system_timer.c
                nanostack-event-loop/platform/platform_posix.c
                nsdynmemLIB_posix.c
)

target_include_directories(event_loop PRIVATE . 
                            nanostack-event-loop/include
                            nanostack-event-loop)