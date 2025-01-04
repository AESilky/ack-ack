# Library: cmt (obj only)
add_library(cmt INTERFACE)

target_sources(cmt INTERFACE
  cmt.c
)

target_link_libraries(cmt INTERFACE
  pico_float
  pico_stdlib
)
