add_executable(pio_ws28xx)

# generate the PIO header file
pico_generate_pio_header(pio_ws28xx ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

target_sources(pio_ws28xx PRIVATE ws28xx.c)

target_link_libraries(pio_ws28xx PRIVATE pico_stdlib hardware_pio)
pico_add_extra_outputs(pio_ws28xx)
