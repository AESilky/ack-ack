# Library: expio (Expansion I/O - obj only)
add_library(expio INTERFACE)

target_sources(expio INTERFACE
    expio.c
)

target_link_libraries(expio INTERFACE
    hardware_spi
    pico_stdlib
)
