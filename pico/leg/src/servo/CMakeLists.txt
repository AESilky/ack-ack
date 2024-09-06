# Library: servo (obj only)
add_library(servo INTERFACE)

pico_generate_pio_header(servo ${CMAKE_CURRENT_LIST_DIR}/pwm.pio)
pico_generate_pio_header(servo ${CMAKE_CURRENT_LIST_DIR}/pwm_rcv.pio)

target_sources(servo INTERFACE
  servo.c
  receiver.c
)

target_link_libraries(servo INTERFACE
  pico_stdlib
  hardware_pio
)
