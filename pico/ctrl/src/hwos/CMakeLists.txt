# Library: Back-End (hwos) (obj only)
add_library(hwos INTERFACE)

target_sources(hwos INTERFACE
    hwos.c
)

#add_subdirectory(display)

target_link_libraries(hwos INTERFACE
    pico_stdlib
)
