# Library: expio (Expansion I/O - obj only)
add_library(eeprom INTERFACE)

target_sources(eeprom INTERFACE
    eeprom.c
)

target_link_libraries(eeprom INTERFACE
    hardware_i2c
    pico_stdlib
)
