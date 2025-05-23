/**
 * HWControl Board Initialization and General Functions.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
 * This sets up the Pico-W for use for hwctrl.
 * It:
 * 1 Configures the GPIO Pins for the proper IN/OUT
 *
 * It provides some utility methods to:
 * 1. Turn the On-Board LED ON/OFF
 * 2. Flash the On-Board LED a number of times
 * 3. Turn the buzzer ON/OFF
 * 4. Beep the buzzer a number of times
 * 5. Read the state of the input switch
 *
*/
#ifndef HWCBoard_H_
#define HWCBoard_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * @brief Initialize the board
 * @ingroup board
 *
 * This sets up the GPIO for the proper direction (IN/OUT), pull-ups, etc.
 * This calls the init for each of the devices/subsystems.
 * If all is okay, it returns true, else false.
*/
extern int board_init(void);

/**
 * @brief Get the board address (0=Main, 1=Secondary)
 * @ingroup board
 *
 * @return uint8_t 0=Main 1=Secondary
 */
extern uint8_t board_addr(void);

/**
 * @brief Turn the display backlight ON/OFF.
 * @ingroup board
 *
 * @param on True=On False=OFF
 */
extern void display_backlight_on(bool on);

/**
 * @brief Flash the Pico LED on/off
 * @ingroup board
 *
 * @param ms Milliseconds to turn the LED on.
*/
extern void led_flash(int ms);

/**
 * @brief Turn the Pico LED on/off
 * @ingroup board
 *
 * @param on True to turn LED on, False to turn it off.
*/
extern void led_on(bool on);

/**
 * @brief Turn the Pico LED on off on off...
 * @ingroup board
 *
 * This flashes the LED for times specified by the `pattern` in milliseconds.
 *
 * @param pattern Array of millisend values to turn the LED on, off, on, etc.
 *      The last element of the array must be 0.
*/
extern void led_on_off(const int32_t* pattern);

/**
 * @brief Turn the Board LED-A (green) on/off
 * @ingroup board
 *
 * @param on True to turn LED on, False to turn it off.
*/
extern void ledA_on(bool on);

/**
 * @brief Turn the Board LED-B (yellow) on/off
 * @ingroup board
 *
 * @param on True to turn LED on, False to turn it off.
*/
extern void ledB_on(bool on);

/**
 * @brief Get a millisecond value of the time since the board was booted.
 * @ingroup board
 *
 * @return uint32_t Time in milliseconds
 */
extern uint32_t now_ms();

/**
 * @brief Get a microsecond value of the time since the board was booted.
 * @ingroup board
 *
 * @return uint64_t Time in microseconds
 */
extern uint64_t now_us();

/**
 * @brief Get the temperature from the on-chip temp sensor in Celsius.
 *
 * @return float Celsius temperature
 */
extern float onboard_temp_c();

/**
 * @brief Get the temperature from the on-chip temp sensor in Fahrenheit.
 *
 * @return float Fahrenheit temperature
 */
extern float onboard_temp_f();

/**
 * @brief The current state of the User Input Switch
 * @ingroup board
 *
 * @return true The switch is pressed
 * @return false The switch isn't pressed
 */
extern bool user_switch_pressed();

/** @brief Printf like function that includes the datetime and type prefix */
extern void debug_printf(const char* format, ...) __attribute__((format(_printf_, 1, 2)));
/** @brief Printf like function that includes the datetime and type prefix */
extern void error_printf(const char* format, ...) __attribute__((format(_printf_, 1, 2)));
/** @brief Printf like function that includes the datetime and type prefix */
extern void info_printf(const char* format, ...) __attribute__((format(_printf_, 1, 2)));
/** @brief Printf like function that includes the datetime and type prefix */
extern void warn_printf(const char* format, ...) __attribute__((format(_printf_, 1, 2)));

/**
 * @brief Board level (common) panic location.
 * @ingroup board
 *
 * This should be used in preference to directly using the Pico `panic` to make
 * it better for debugging and common fatal error handling.
 *
 * @param fmt format string (printf-like)
 * @param ...  printf-like arguments
 */
extern void board_panic(const char* fmt, ...);

#ifdef __cplusplus
}
#endif
#endif // HWCBoard_H_
