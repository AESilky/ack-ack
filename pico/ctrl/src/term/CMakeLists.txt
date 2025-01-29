# Library: Touch Panel (obj only)
add_library(terminal INTERFACE)

target_sources(terminal INTERFACE
    term.c
    tkbd.c
)

target_link_libraries(terminal INTERFACE
    pico_stdlib
)
