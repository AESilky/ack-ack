# Library: Sensor Bank (obj only)
add_library(sensbank INTERFACE)

pico_generate_pio_header(sensbank ${CMAKE_CURRENT_LIST_DIR}/sensbank.pio)

target_sources(sensbank INTERFACE
  sensbank.c
)
