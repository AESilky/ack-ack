# Library: Hardware Runtime for board address 0 (hwrt0) (obj only)
add_library(hwrt0 INTERFACE)

target_sources(hwrt0 INTERFACE
    hwrt0.c
)

target_link_libraries(hwrt0 INTERFACE
    pico_stdlib
)

# Library: Hardware Runtime for board address 1 (hwrt1) (obj only)
add_library(hwrt1 INTERFACE)

target_sources(hwrt1 INTERFACE
    hwrt1.c
)

target_link_libraries(hwrt1 INTERFACE
    pico_stdlib
)
