# Library: Hardware Runtime (hwrt) (obj only)
add_library(hwrt INTERFACE)

target_sources(hwrt INTERFACE
    hwrt.c
)

#add_subdirectory(display)

target_link_libraries(hwrt INTERFACE
    pico_stdlib
)
