/**
 * Board Initialization and General Functions.
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 *
 * This sets up the Pico-W for use for leg.
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
#ifndef _ScoreBOARD_H_
#define _ScoreBOARD_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "system_defs.h" // Main system/board/application definitions

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
 * @brief Beep for a 'normal' duration
 * @ingroup board
 */
extern void beep();

/**
 * @brief Beep for an 'extended' duration
 * @ingroup board
 */
extern void beep_long();

/**
 * @brief Flash the LED on/off
 * @ingroup board
 *
 * @param ms Milliseconds to turn the LED on.
*/
extern void led_flash(int ms);

/**
 * @brief Turn the LED on/off
 * @ingroup board
 *
 * @param on True to turn LED on, False to turn it off.
*/
extern void led_on(bool on);

/**
 * @brief Turn the LED on off on off...
 * @ingroup board
 *
 * This flashes the LED for times specified by the `pattern` in milliseconds.
 *
 * @param pattern Array of millisend values to turn the LED on, off, on, etc.
 *      The last element of the array must be 0.
*/
extern void led_on_off(const int32_t* pattern);

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
 * @brief Get the temperature from the on-chip temp sensor if Celsius.
 *
 * @return float Celsius temperature
 */
extern float onboard_temp_c();

/**
 * @brief Get the temperature from the on-chip temp sensor in Farenheit.
 *
 * @return float Farenheit temperature
 */
extern float onboard_temp_f();

/**
 * @brief Beep the Buzzer on/off
 * @ingroup board
 *
 * @param ms Milliseconds to turn the buzzer on.
*/
extern void tone_sound_duration(int ms);

/**
 * @brief Turn the buzzer on/off
 * @ingroup board
 *
 * @param on True to turn buzzer on, False to turn it off.
*/
extern void tone_on(bool on);

/**
 * @brief Beep the buzzer on/off/on/off...
 * @ingroup board
 *
 * This beeps the buzzer for times specified by the `pattern` in milliseconds.
 *
 * @param pattern Array of millisend values to beep the buzzer on, off, on, etc.
 *      The last element of the array must be 0.
*/
extern void tone_on_off(const int32_t* pattern);

/**
 * @brief The current state of the User Input Switch
 * @ingroup board
 *
 * @return true The switch is pressed
 * @return false The switch isn't pressed
 */
extern bool user_switch_pressed();

/** @brief Printf like function that only prints in debug mode, includes a 'D:' prefix and appends NL */
extern void debug_printf(const char* format, ...) __attribute__((format(_printf_, 1, 2)));
/** @brief Printf like function that includes a 'E:' prefix and appends NL */
extern void error_printf(const char* format, ...) __attribute__((format(_printf_, 1, 2)));
/** @brief Printf like function that includes a 'I:' prefix and appends NL */
extern void info_printf(const char* format, ...) __attribute__((format(_printf_, 1, 2)));
/** @brief Printf like function that includes a 'W:' prefix and appends NL */
extern void warn_printf(const char* format, ...) __attribute__((format(_printf_, 1, 2)));

#ifdef __cplusplus
}
#endif
#endif // _ScoreBOARD_H_
