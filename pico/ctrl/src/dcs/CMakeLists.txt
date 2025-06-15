# Library: Drive Control System (dcs processing)
add_library(dcs INTERFACE)

target_sources(dcs INTERFACE
    dcs.c
    core1_main.c
    dcs_rc.c
)

#add_subdirectory(display)

target_link_libraries(dcs INTERFACE
    pico_multicore
    pico_stdlib
    rcrx
)
