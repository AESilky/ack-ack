# Library: neopix (obj only)
add_library(neopix INTERFACE)

pico_generate_pio_header(neopix ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)

target_sources(neopix INTERFACE
  neopix.c
  ws2312.c
)
