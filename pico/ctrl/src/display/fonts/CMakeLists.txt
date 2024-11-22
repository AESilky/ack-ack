add_library(fonts INTERFACE)

target_sources(fonts INTERFACE
  font_10_16.c
)

target_link_libraries(fonts INTERFACE
  pico_stdlib
)
