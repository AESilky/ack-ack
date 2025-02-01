# Library: display (obj only)
add_library(display INTERFACE)

target_sources(display INTERFACE
    display.c
)

# Use one of the two displays
add_subdirectory(fonts)
add_subdirectory(display_rgb18)

target_link_libraries(display INTERFACE
    pico_stdlib
)
