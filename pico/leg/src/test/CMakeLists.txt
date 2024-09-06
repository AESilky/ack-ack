# Library: leg test (obj only)
add_library(leg_test INTERFACE)

target_sources(leg_test INTERFACE
    test.c
)

target_link_libraries(leg_test INTERFACE
    pico_stdlib
)
