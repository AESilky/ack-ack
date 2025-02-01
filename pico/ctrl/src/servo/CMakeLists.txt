# Library: Serial Bus Servo (obj only)
add_library(servo INTERFACE)

target_sources(servo INTERFACE
  servo.c
  servos.c
)
