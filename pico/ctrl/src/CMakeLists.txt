# hwctrl primary project file

cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
#set(PICO_PLATFORM "rp2350-arm-s")
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


add_subdirectory(curswitch)
add_subdirectory(dcs)
add_subdirectory(display)
add_subdirectory(eeprom)
add_subdirectory(gfx)
add_subdirectory(hid_nav)
add_subdirectory(hwrt)
add_subdirectory(neopix)
add_subdirectory(rcrx)
add_subdirectory(rotary_encoder)
add_subdirectory(rover)
add_subdirectory(sensbank)
add_subdirectory(servo)
add_subdirectory(term)
add_subdirectory(termx)
add_subdirectory(touch_panel)

#
# Common top-level sources
#
set(CMN_TL_SRC
  main.c
  board.c
  debug_support.c
  multicore.c
  pio_sm.c
  spi_ops.c
  tests.c
  p2bugs.c
)
#
# Common libraries (built the same for both boards 0 & 1)
#
add_subdirectory(cmt)
add_subdirectory(expio)
add_subdirectory(util)
#
# External libraries
add_subdirectory(lib)
#
set(CMN_LIBS
  cmt
  eeprom
  expio
  sensbank
  util
  hardware_adc
  hardware_clocks
  hardware_dma
  hardware_exception
  hardware_i2c
  hardware_pio
  hardware_spi
  hardware_timer
  pico_double
  pico_float
  pico_multicore
  pico_stdlib
)

#
# Add executable for Board-0
#
add_executable(hwctrl_0
  ${CMN_TL_SRC}
)
pico_set_program_name(hwctrl_0 "HWControl-0")
pico_set_program_version(hwctrl_0 "1.0")
# Use the VFP instructions for float and double (can use DCP)
pico_set_float_implementation(hwctrl_0 pico)
pico_set_double_implementation(hwctrl_0 pico)
# Use the USB for stdio. UART is used to communicate with host for commands.
pico_enable_stdio_uart(hwctrl_0 1)
pico_enable_stdio_usb(hwctrl_0 0)
target_compile_definitions(hwctrl_0 PRIVATE
	BOARD_ADDR=0
)
target_include_directories(hwctrl_0 PRIVATE
	${CMAKE_CURRENT_LIST_DIR}
)
# Add the libraries required by the system to the build
target_link_libraries(hwctrl_0
  ${CMN_LIBS}
)
target_link_libraries(hwctrl_0
  dcs
  eeprom
  hwrt0
  rcrx
  rover
  servo
  termx
)
if(${PICO_BOARD} STREQUAL "pico")
target_link_libraries(hwctrl_0
  hardware_rtc
)
endif()
if(${PICO_BOARD} STREQUAL "pico_w")
target_link_libraries(hwctrl_0
  hardware_rtc
  pico_cyw43_arch_lwip_threadsafe_background
)
endif()
#
pico_add_extra_outputs(hwctrl_0)


#
# Add executable for Board-1
#
add_executable(hwctrl_1
  ${CMN_TL_SRC}
)
pico_set_program_name(hwctrl_1 "HWControl-1")
pico_set_program_version(hwctrl_1 "1.0")
# Use the VFP instructions for float and double (can use DCP)
pico_set_float_implementation(hwctrl_1 pico)
pico_set_double_implementation(hwctrl_1 pico)
# Use the USB for stdio. UART is used to communicate with host for commands.
pico_enable_stdio_uart(hwctrl_1 0)
pico_enable_stdio_usb(hwctrl_1 1)
target_compile_definitions(hwctrl_1 PRIVATE
	BOARD_ADDR=1
)
target_include_directories(hwctrl_1 PRIVATE
	${CMAKE_CURRENT_LIST_DIR}
)
# Add the libraries required by the system to the build
target_link_libraries(hwctrl_1
  ${CMN_LIBS}
)
target_link_libraries(hwctrl_1
  curswitch
  display
  fonts
  gfx
  hid_nav
  hwrt1
  neopix
  rotary_encoder
  terminal
  termx
  touch_panel
)
if(${PICO_BOARD} STREQUAL "pico")
target_link_libraries(hwctrl_1
  hardware_rtc
)
endif()
if(${PICO_BOARD} STREQUAL "pico_w")
target_link_libraries(hwctrl_1
  hardware_rtc
  pico_cyw43_arch_lwip_threadsafe_background
)
endif()
#
pico_add_extra_outputs(hwctrl_1)

# Add the required include file paths to the build
include_directories(
  ${CMAKE_CURRENT_LIST_DIR}
  ${PICO_SDK_PATH}/src/rp2_common/hardware_adc/include
  ${PICO_SDK_PATH}/src/rp2_common/hardware_dma/include
  ${PICO_SDK_PATH}/src/rp2_common/hardware_spi/include
  ${PICO_SDK_PATH}/src/rp2_common/hardware_i2c/include
  ${PICO_SDK_PATH}/src/rp2_common/hardware_pio/include
  ${PICO_SDK_PATH}/src/rp2_common/hardware_pwm/include
  ${PICO_SDK_PATH}/src/rp2_common/pico_multicore/include
)

if(${PICO_BOARD} STREQUAL "pico")
include_directories(
  ${PICO_SDK_PATH}/src/rp2_common/hardware_rtc/include
)
endif()
if(${PICO_BOARD} STREQUAL "pico_w")
include_directories(
  ${PICO_SDK_PATH}/src/rp2_common/hardware_rtc/include
  ${PICO_SDK_PATH}/src/rp2_common/pico_cyw43_arch/include
)
endif()


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


# pico_set_linker_script(hwctrl ${CMAKE_SOURCE_DIR}/memmap_custom.ld)
