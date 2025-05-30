# Library: pio_uart (obj only)
add_library(pio_uart INTERFACE)

pico_generate_pio_header(pio_uart ${CMAKE_CURRENT_LIST_DIR}/serial_rd.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)

target_sources(pio_uart INTERFACE
  rcrx.c
)

# Add the libraries required by the system to the build
target_link_libraries(pio_uart INTERFACE
  hardware_dma
  hardware_pio
)

