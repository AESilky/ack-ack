# Library: display (obj only)
target_sources(display INTERFACE
  display_rgb18.c
  ili_lcd_spi.c
)

# Use one of the two displays
add_subdirectory(ili9341_spi)
add_subdirectory(ili9488_spi)

target_link_libraries(display INTERFACE
  hardware_spi
  pico_stdlib
)
