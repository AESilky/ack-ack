# Library: OS (obj only)
add_library(os INTERFACE)

target_sources(os INTERFACE
    os.c
)

target_link_libraries(os INTERFACE
    pico_stdlib
)
