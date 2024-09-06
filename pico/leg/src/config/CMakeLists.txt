# Library: config (obj only)
add_library(config INTERFACE)

target_sources(config INTERFACE
  config.c
  config_hndlr.c
)

include_directories(
  ${PICO_SDK_PATH}/src/common/pico_stdlib/include
)

target_link_libraries(config INTERFACE
  pico_stdlib
)
