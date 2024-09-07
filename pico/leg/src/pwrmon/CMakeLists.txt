add_library(pwrmon INTERFACE)

target_sources(pwrmon INTERFACE
  pwrmon_3221.c
)

target_link_libraries(pwrmon INTERFACE
  pico_stdlib
  hardware_i2c
)
