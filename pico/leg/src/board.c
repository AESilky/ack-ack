/**
 * Board Initialization and General Functions.
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 *
 * This sets up the Pico for use for leg.
 * It:
 * 1. Configures the GPIO Pins for the proper IN/OUT, pull-ups, etc.
 * 2. Calls the init routines for Config, Functional-Level (Display, Touch, rotary)
 *
 * It provides some utility methods to:
 * 1. Turn the On-Board LED ON/OFF
 * 2. Flash the On-Board LED a number of times
 * 3. Turn the buzzer ON/OFF
 * 4. Beep the buzzer a number of times
 *
*/
#include "system_defs.h"

#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "pico/printf.h"
#include "pico/time.h"
#include "pico/types.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/rtc.h"
#include "hardware/spi.h"
#include "hardware/timer.h"
#include "pico/bootrom.h"
#ifdef BOARD_IS_PICOW
    #include "pico/cyw43_arch.h"
#endif
#include "config/config.h"
#include "display/display.h"
#include "board.h"
#include "debug_support.h"
#include "cmt/multicore.h"
#include "pwrmon/pwrmon_3221.h"
#include "servo/receiver.h"
#include "servo/servo.h"
#include "util/util.h"


/**
 * @brief Initialize the board
 *
 * This sets up the GPIO for the proper direction (IN/OUT), pull-ups, etc.
 * This calls the init for each of the devices/subsystems.
 * If all is okay, it returns 0, else non-zero.
 *
 * Although each subsystem could (some might argue should) configure their own Pico
 * pins, having everything here makes the overall system easier to understand
 * and helps assure that there are no conflicts.
*/
int board_init() {
    int retval = 0;

    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    stdio_init_all();

    sleep_ms(80); // Ok to `sleep` as msg system not started

    // SPI NOT USED.

    // I2C USED for OLED Display and Current Sense.
    i2c_init(I2C_PORT, SSD1306_I2C_CLK * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    gpio_set_drive_strength(I2C_SDA_PIN, GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_drive_strength(I2C_SCL_PIN, GPIO_DRIVE_STRENGTH_2MA);

    // GPIO Outputs (other than chip-selects)
    //    Tone drive
    gpio_set_function(TONE_DRIVE,   GPIO_FUNC_SIO);
    gpio_set_dir(TONE_DRIVE, GPIO_OUT);
    gpio_set_drive_strength(TONE_DRIVE, GPIO_DRIVE_STRENGTH_4MA);
    gpio_put(TONE_DRIVE, TONE_OFF);

    // GPIO Inputs
    //    User Input Switch
    gpio_init(USER_INPUT_SW);
    gpio_pull_up(USER_INPUT_SW);
    gpio_set_dir(USER_INPUT_SW, GPIO_IN);

    // Check the user input switch to see if it's pressed during startup.
    //  If yes, set 'debug_mode_enabled'
    if (user_switch_pressed()) {
        debug_mode_enable(true);
    }

    // Initialize the display
    disp_module_init();

    disp_string(1, 3, "Silky", false, true);
    disp_string(2, 3, "Design", false, true);

    disp_row_clear(4, false);
    disp_string(4, 0, "Init: ADC", false, true);
    // Initialize hardware AD converter, enable onboard temperature sensor and
    //  select its channel.
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4); // Inputs 0-3 are GPIO pins, 4 is the built-in temp sensor

    // Get the configuration
    disp_row_clear(4, false);
    disp_string(4, 0, "Init: Config", false, true);
    config_module_init();

    // Initialize the multicore subsystem
    disp_row_clear(4, false);
    disp_string(4, 0, "Init: MC", false, true);
    multicore_module_init();

    //
    // Functional Hardware
    //

    // Initialize the servo subsystem
    disp_row_clear(4, false);
    disp_string(4, 0, "Init: Servos", false, true);
    servo_module_init();
    // Initialize the receiver subsystem
    disp_row_clear(4, false);
    disp_string(4, 0, "Init: Receiver", false, true);
    receiver_module_init();
    // Initialize the power monitor
    disp_row_clear(4, false);
    disp_string(4, 0, "Init: Power-Mon", false, true);
    pwrmon_module_init();


    disp_row_clear(4, false);
    disp_string(4, 0, "Leg Ready", false, true);

    return(retval);
}

extern void beep() {
    tone_sound_duration(200);
}

extern void beep_long() {
    tone_sound_duration(500);
}

void boot_to_bootsel() {
    reset_usb_boot(0, 0);
}

void _tone_sound_duration_cont(void *user_data) {
    tone_on(false);
}
void tone_sound_duration(int ms) {
    debug_printf("tone_sound_duration(%d)", ms);
    // tone_on(true);
    // if (!cmt_message_loop_0_running()) {
    //     sleep_ms(ms);
    //     _tone_sound_duration_cont(NULL);
    // }
    // else {
    //     cmt_sleep_ms(ms, _tone_sound_duration_cont, NULL);
    // }
}

void tone_on(bool on) {
    // gpio_put(TONE_DRIVE, (on ? TONE_ON : TONE_OFF));
}

void _tone_on_off_cont(void* data) {
    int32_t *pattern = (int32_t*)data;
    tone_on_off(pattern);
}
void tone_on_off(const int32_t *pattern) {
    // while (*pattern) {
    //     tone_sound_duration(*pattern++);
    //     int off_time = *pattern++;
    //     if (off_time == 0) {
    //         return;
    //     }
    //     if (!cmt_message_loop_0_running()) {
    //         sleep_ms(off_time);
    //     }
    //     else {
    //         cmt_sleep_ms(off_time, _tone_on_off_cont, (void*)pattern);
    //     }
    // }
}

static void _led_flash_cont(void* user_data) {
    led_on(false);
}
void led_flash(int ms) {
    led_on(true);
    if (!cmt_message_loop_0_running()) {
        sleep_ms(ms);
        _led_flash_cont(NULL);
    }
    else {
        cmt_sleep_ms(ms, _led_flash_cont, NULL);
    }
}

void led_on(bool on) {
#ifdef BOARD_IS_PICO
    gpio_put(PICO_DEFAULT_LED_PIN, on);
#else
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);
#endif
}

void _led_on_off_cont(void* user_data) {
    int32_t* pattern = (int32_t*)user_data;
    led_on_off(pattern);
}
void led_on_off(const int32_t *pattern) {
    while (*pattern) {
        led_flash(*pattern++);
        int off_time = *pattern++;
        if (off_time == 0) {
            return;
        }
        if (!cmt_message_loop_0_running()) {
            sleep_ms(off_time);
        }
        else {
            cmt_sleep_ms(off_time, _led_on_off_cont, (void*)pattern);
        }
    }
}

uint32_t now_ms() {
    return (us_to_ms(time_us_64()));
}

uint64_t now_us() {
    return (time_us_64());
}

/* References for this implementation:
 * raspberry-pi-pico-c-sdk.pdf, Section '4.1.1. hardware_adc'
 * pico-examples/adc/adc_console/adc_console.c */
float onboard_temp_c() {
    /* 12-bit conversion, assume max value == ADC_VREF == 3.3 V */
    const float conversionFactor = 3.3f / (1 << 12);

    adc_select_input(4); // Inputs 0-3 are GPIO pins, 4 is the built-in temp sensor
    float adc = (float)adc_read() * conversionFactor;
    float tempC = 27.0f - (adc - 0.706f) / 0.001721f;

    return (tempC);
}

float onboard_temp_f() {
    return (onboard_temp_c() * 9 / 5 + 32);
}

bool user_switch_pressed() {
    return (gpio_get(USER_INPUT_SW) == USER_SW_CLOSED);
}

void debug_printf(const char* format, ...) {
    if (debug_mode_enabled()) {
        char buf[512];
        int index = 0;
        index += snprintf(&buf[index], sizeof(buf) - index, "D:");
        va_list xArgs;
        va_start(xArgs, format);
        index += vsnprintf(&buf[index], sizeof(buf) - index, format, xArgs);
        va_end(xArgs);
        disp_string(disp_info_char_lines()-1, 0, buf, false, true);
        printf("%s\n", buf);
    }
}

void error_printf(const char* format, ...) {
    char buf[512];
    int index = 0;
    index += snprintf(&buf[index], sizeof(buf) - index, "E:");
    va_list xArgs;
    va_start(xArgs, format);
    index += vsnprintf(&buf[index], sizeof(buf) - index, format, xArgs);
    va_end(xArgs);
    disp_string(disp_info_char_lines()-1, 0, buf, false, true);
    printf("%s\n", buf);
}

void info_printf(const char* format, ...) {
    char buf[512];
    int index = 0;
    index += snprintf(&buf[index], sizeof(buf) - index, "I:");
    va_list xArgs;
    va_start(xArgs, format);
    index += vsnprintf(&buf[index], sizeof(buf) - index, format, xArgs);
    va_end(xArgs);
    disp_string(disp_info_char_lines()-1, 0, buf, false, true);
    printf("%s\n", buf);
}

void warn_printf(const char* format, ...) {
    char buf[512];
    int index = 0;
    index += snprintf(&buf[index], sizeof(buf) - index, "W:");
    va_list xArgs;
    va_start(xArgs, format);
    index += vsnprintf(&buf[index], sizeof(buf) - index, format, xArgs);
    va_end(xArgs);
    disp_string(disp_info_char_lines()-1, 0, buf, false, true);
    printf("%s\n", buf);
}

