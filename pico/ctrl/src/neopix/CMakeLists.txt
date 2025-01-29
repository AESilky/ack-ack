# Library: neopix (obj only)
add_library(neopix INTERFACE)

pico_generate_pio_header(neopix ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)

target_sources(neopix INTERFACE
  neopix.c
)

# Add the libraries required by the system to the build
target_link_libraries(neopix INTERFACE
  hardware_dma
)

