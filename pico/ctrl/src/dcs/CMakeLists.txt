# Library: Drive Control System (dcs processing)
add_library(dcs INTERFACE)

target_sources(dcs INTERFACE
    dcs.c
    dcs_rc.c
)

#add_subdirectory(display)

target_link_libraries(dcs INTERFACE
    pico_stdlib
    rcrx
)
