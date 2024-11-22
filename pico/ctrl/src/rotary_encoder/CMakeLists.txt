# Library: rotary_encoder (Includes Programable I/O)
add_library(rotary_encoder INTERFACE)

pico_generate_pio_header(
    rotary_encoder ${CMAKE_CURRENT_LIST_DIR}/quadrature_encoder.pio
)

target_sources(rotary_encoder INTERFACE
    rotary_encoder.c
    re_pbsw.c
)

target_link_libraries(rotary_encoder INTERFACE
    hardware_pio
    pico_stdlib
)
