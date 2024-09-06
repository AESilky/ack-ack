# Library: Functional-Level (obj only)
add_library(fl INTERFACE)

target_sources(fl INTERFACE
    fl.c
    fl_disp.c
    core1_main.c
)

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(fl INTERFACE
    pico_stdlib
    pico_multicore
)
