# leg primary project file

cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_BOARD "pico")
set(PICO_USE_MALLOC_MUTEX 1)
# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)
message("Raspberry Pi Pico SDK version is: ${PICO_SDK_VERSION_STRING}")
if (PICO_SDK_VERSION_STRING VERSION_LESS "1.5.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.5.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()
# Use fully-deoptimized debug build for true single-step and data viewing. Set to '0' for some optimizations
set(PICO_DEOPTIMIZED_DEBUG 1)

project(leg C CXX ASM)

add_compile_options(
  -O0                       # No optimizations for now, for debugging
  -Wall
  -Wno-format               # int != int32_t as far as the compiler is concerned
  -Wno-unused-function
  -Wno-maybe-uninitialized
  #-H                       # list include header paths
)
pico_sdk_init()

# External libraries
# <none>
# Local libraries
add_subdirectory(cg)
add_subdirectory(cmt)
add_subdirectory(config)
add_subdirectory(display)
add_subdirectory(fl)
add_subdirectory(os)
add_subdirectory(pwrmon)
add_subdirectory(servo)
add_subdirectory(test)
add_subdirectory(util)
add_subdirectory(lib)

# Add executable.
add_executable(leg
  main.c
  board.c
  debug_support.c
)

pico_set_program_name(leg "Leg")
pico_set_program_version(leg "0.1")

# Use the UART for communication
pico_enable_stdio_uart(leg 1)
pico_enable_stdio_usb(leg 0)

# Add the required include file paths to the build
include_directories(
  ${CMAKE_CURRENT_LIST_DIR}
  ${PICO_SDK_PATH}/src/common/pico_stdlib/include
  ${PICO_SDK_PATH}/src/rp2_common/hardware_adc/include
  ${PICO_SDK_PATH}/src/rp2_common/hardware_dma/include
  ${PICO_SDK_PATH}/src/rp2_common/hardware_spi/include
  ${PICO_SDK_PATH}/src/rp2_common/hardware_i2c/include
  ${PICO_SDK_PATH}/src/rp2_common/hardware_pio/include
  ${PICO_SDK_PATH}/src/rp2_common/hardware_pwm/include
  ${PICO_SDK_PATH}/src/rp2_common/hardware_rtc/include
  ${PICO_SDK_PATH}/src/rp2_common/pico_multicore/include
  ${PICO_SDK_PATH}/src/rp2040/hardware_structs/include
)

# ZZZ Figure out how to set `PICO_DEBUG_MALLOC` only if 'debug build'
add_compile_definitions(
  PICO_MALLOC_PANIC
  PICO_USE_MALLOC_MUTEX
  PICO_USE_STACK_GUARDS
  PICO_STACK_SIZE=4096
  PICO_CORE1_STACK_SIZE=4096

  # PICO_DEBUG_MALLOC
  PICO_MAX_SHARED_IRQ_HANDLERS=6u
  PICO_STDIO_USB_CONNECTION_WITHOUT_DTR
  PICO_STDIO_USB_SUPPORT_CHARS_AVAILABLE_CALLBACK
)

# Add the libraries required by the system to the build
target_link_libraries(leg
  cg
  cmt
  config
  display
  fl
  fonts
  os
  pwrmon
  servo
  util
  hardware_adc
  hardware_clocks
  hardware_exception
  hardware_i2c
  hardware_pio
  hardware_timer
  pico_stdlib
)

pico_set_linker_script(leg ${CMAKE_SOURCE_DIR}/memmap_custom.ld)

pico_add_extra_outputs(leg)
