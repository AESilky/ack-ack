# Library: Sensor Bank (obj only)
add_library(sensbank INTERFACE)

pico_generate_pio_header(sensbank ${CMAKE_CURRENT_LIST_DIR}/sensbank.pio)

target_sources(sensbank INTERFACE
  sensbank.c
  adc1015.c
)

target_link_libraries(sensbank INTERFACE
    hardware_i2c
    hardware_pio
    pico_stdlib
)

