# Library: pio_uart (obj only)
add_library(rcrx INTERFACE)

pico_generate_pio_header(rcrx ${CMAKE_CURRENT_LIST_DIR}/rx_sbus.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)
pico_generate_pio_header(rcrx ${CMAKE_CURRENT_LIST_DIR}/serial_rd.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)

target_sources(rcrx INTERFACE
  rcrx.c
  rx_sbus.c
  serial_rd.c
)

# Add the libraries required by the system to the build
target_link_libraries(rcrx INTERFACE
  hardware_dma
  hardware_pio
)

