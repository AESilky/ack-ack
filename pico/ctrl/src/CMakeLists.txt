# hwctrl primary project file

cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_PLATFORM "rp2350-arm-s")
set(PICO_BOARD "pico2")
set(PICO_USE_MALLOC_MUTEX 1)
set(PICO_QUEUE_MAX_LEVEL 1)
# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)
if (PICO_SDK_VERSION_STRING VERSION_LESS "2.1.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 2.1.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()
# Use fully-deoptimized debug build for true single-step and data viewing. Set to '0' for some optimizations
set(PICO_DEOPTIMIZED_DEBUG 1)

project(hwctrl C CXX ASM)

add_compile_options(
  -O0                       # No optimizations for now, for debugging
  -Wall
  -Wno-format               # int != int32_t as far as the compiler is concerned
  -Wno-unused-function
  -Wno-maybe-uninitialized
  #-H                       # list include header paths
)
pico_sdk_init()

# Local libraries (additional sources)
add_subdirectory(cmt)
add_subdirectory(curswitch)
add_subdirectory(dcs)
add_subdirectory(display)
add_subdirectory(expio)
add_subdirectory(gfx)
add_subdirectory(hwos)
add_subdirectory(rotary_encoder)
add_subdirectory(rover)
add_subdirectory(servo)
add_subdirectory(term)
add_subdirectory(touch_panel)
add_subdirectory(util)

# Add executable.
add_executable(hwctrl
  main.c
  board.c
  debug_support.c
  multicore.c
  spi_ops.c
)

pico_set_program_name(hwctrl "HWControl")
pico_set_program_version(hwctrl "0.1")

# Use the USB for stdio. UART is used to communicate with host for commands.
pico_enable_stdio_uart(hwctrl 0)
pico_enable_stdio_usb(hwctrl 1)

# Add the required include file paths to the build
include_directories(
  ${CMAKE_CURRENT_LIST_DIR}
  ${PICO_SDK_PATH}/src/rp2_common/hardware_adc/include
  ${PICO_SDK_PATH}/src/rp2_common/hardware_dma/include
  ${PICO_SDK_PATH}/src/rp2_common/hardware_spi/include
  ${PICO_SDK_PATH}/src/rp2_common/hardware_i2c/include
  ${PICO_SDK_PATH}/src/rp2_common/hardware_pio/include
  ${PICO_SDK_PATH}/src/rp2_common/hardware_pwm/include
  ${PICO_SDK_PATH}/src/rp2_common/pico_cyw43_arch/include
  ${PICO_SDK_PATH}/src/rp2_common/pico_multicore/include
)

# ZZZ Figure out how to set `PICO_DEBUG_MALLOC` only if 'debug build'
add_compile_definitions(
  PICO_MALLOC_PANIC
  PICO_USE_MALLOC_MUTEX
  PICO_QUEUE_MAX_LEVEL
  PICO_USE_STACK_GUARDS
  PICO_STACK_SIZE=4096
  PICO_CORE1_STACK_SIZE=4096

  # PICO_DEBUG_MALLOC
  PICO_MAX_SHARED_IRQ_HANDLERS=6u
  PICO_STDIO_USB_CONNECTION_WITHOUT_DTR
  PICO_STDIO_USB_SUPPORT_CHARS_AVAILABLE_CALLBACK
)

# Add the libraries required by the system to the build
target_link_libraries(hwctrl
  cmt
  curswitch
  dcs
  display
  expio
  fonts
  gfx
  hwos
  rotary_encoder
  rover
  servo
  terminal
  touch_panel
  util
  hardware_adc
  hardware_clocks
  hardware_exception
  hardware_i2c
  hardware_pio
  hardware_spi
  hardware_timer
  pico_float
  pico_multicore
  pico_stdlib
)
if(${PICO_BOARD} STREQUAL "pico_w")
target_link_libraries(hwctrl
  pico_cyw43_arch_lwip_threadsafe_background
)
endif()

# pico_set_linker_script(hwctrl ${CMAKE_SOURCE_DIR}/memmap_custom.ld)

pico_add_extra_outputs(hwctrl)
