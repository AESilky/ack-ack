# Library: Serial Bus Servo (obj only)
add_library(rover INTERFACE)

target_sources(rover INTERFACE
  rover.c
  rover_info.c
  servos.c
)

# Add the libraries required by the module
target_link_libraries(rover INTERFACE
)

