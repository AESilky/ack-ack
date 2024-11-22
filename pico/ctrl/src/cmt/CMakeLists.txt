# Library: cmt (obj only)
add_library(cmt INTERFACE)

target_sources(cmt INTERFACE
  cmt.c
  core1_main.c
  multicore.c
)

target_link_libraries(cmt INTERFACE
  pico_stdlib
)
