# Library: Touch Panel (obj only)
add_library(termx INTERFACE)

target_sources(termx INTERFACE
)

target_link_libraries(termx INTERFACE
    pico_stdlib
)
