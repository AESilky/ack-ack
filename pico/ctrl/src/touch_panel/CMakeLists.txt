# Library: Touch Panel (obj only)
add_library(touch_panel INTERFACE)

target_sources(touch_panel INTERFACE
    touch.c
)

target_link_libraries(display INTERFACE
    pico_stdlib
)
