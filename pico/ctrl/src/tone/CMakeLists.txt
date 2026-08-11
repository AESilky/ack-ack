# Library: tone (obj only)
add_library(tone INTERFACE)

target_sources(tone INTERFACE
  melody.c
  pitch_freq.c
  tone.c
)

# Add the libraries required by the system to the build
target_link_libraries(tone INTERFACE
  hardware_pwm
)

