# Library: gfx (obj only)
add_library(curswitch INTERFACE)

target_sources(curswitch INTERFACE
curswitch.c
)

target_link_libraries(curswitch INTERFACE
  pico_stdlib
  hardware_adc
)
