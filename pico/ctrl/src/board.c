/**
 * HWControl Board Initialization and General Functions.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
 * This sets up the Pico for use for hwctrl.
 * It:
 * 1. Configures the GPIO Pins for the proper IN/OUT, pull-ups, etc.
 * 2. Calls the init routines for Config, UI (Display, Touch, rotary)
 *
 * It provides some utility methods to:
 * 1. Turn the On-Board LED ON/OFF
 * 2. Flash the On-Board LED a number of times
 * 3. Error, Warn, Info, Debug 'printf' routines
 *
*/
#include "system_defs.h"

#include "pico.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "pico/printf.h"
#include "pico/time.h"
#include "pico/types.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/spi.h"
#include "hardware/timer.h"
#include "hardware/uart.h"
#include "pico/bootrom.h"
#if HAS_RP2040_RTC
#include "hardware/rtc.h"
#endif

#include "board.h"

#include "cmt/cmt.h"
#include "curswitch/curswitch.h"
#include "debug_support.h"
#include "display/display.h"
#include "expio/expio.h"
#include "spi_ops.h"
#include "util/util.h"

// Internal function declarations

static int _format_printf_datetime(char* buf, size_t len);

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

    // Chip selects for the SPI peripherals
    gpio_set_function(SPI_ADDR_0, GPIO_FUNC_SIO);
    gpio_set_dir(SPI_ADDR_0, GPIO_OUT);
    gpio_set_function(SPI_ADDR_1, GPIO_FUNC_SIO);
    gpio_set_dir(SPI_ADDR_1, GPIO_OUT);
    gpio_set_drive_strength(SPI_ADDR_0, GPIO_DRIVE_STRENGTH_2MA);           // CS goes to a single device
    gpio_set_drive_strength(SPI_ADDR_1, GPIO_DRIVE_STRENGTH_2MA);           // CS goes to a single device
    // Display Control/Data
    gpio_set_function(SPI_DISP_CD, GPIO_FUNC_SIO);
    gpio_set_dir(SPI_DISP_CD, GPIO_OUT);
    gpio_set_drive_strength(SPI_DISP_CD, GPIO_DRIVE_STRENGTH_2MA);          // C/D goes to a single device
    // Initial output state
    gpio_put(SPI_ADDR_0, 1);
    gpio_put(SPI_ADDR_1, 1);
    gpio_put(SPI_DISP_CD, 1);

    // SPI 0 Pins for Display and Expansion I/O
    gpio_set_function(SPI_DISP_EXP_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SPI_DISP_EXP_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(SPI_DISP_EXP_MISO, GPIO_FUNC_SPI);
    // SPI 0 Signal drive strengths
    gpio_set_drive_strength(SPI_DISP_EXP_SCK, GPIO_DRIVE_STRENGTH_2MA);     // Two devices connected
    gpio_set_drive_strength(SPI_DISP_EXP_MOSI, GPIO_DRIVE_STRENGTH_2MA);    // Two devices connected
    // SPI 0 Data In Pull-Up
    gpio_pull_up(SPI_DISP_EXP_MISO);
    // SPI 0 initialization for the Display and IO-Expansion. Use SPI at 5MHz.
    spi_init(SPI_DISP_EXP_DEVICE, SPI_DISP_EXP_SPEED);

    // SPI 1 Pins for Touch Panel
    gpio_set_function(SPI_TOUCH_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SPI_TOUCH_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(SPI_TOUCH_MISO, GPIO_FUNC_SPI);
    // SPI 1 Signal drive strengths
    gpio_set_drive_strength(SPI_TOUCH_SCK, GPIO_DRIVE_STRENGTH_2MA);     // Two devices connected
    gpio_set_drive_strength(SPI_TOUCH_MOSI, GPIO_DRIVE_STRENGTH_2MA);    // Two devices connected
    // SPI 1 initialization for the Touch Panel. Use SPI at 2MHz.
    spi_init(SPI_TOUCH_DEVICE, SPI_TOUCH_SPEED);

    // I2C Isn't directly used on the board, but is provided on headers for external use.
    i2c_init(I2C_EXTERN, I2C_EXTERN_CLK_SPEED);
    gpio_set_function(I2C_EXTERN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_EXTERN_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_EXTERN_SDA);
    gpio_pull_up(I2C_EXTERN_SCL);
    gpio_set_drive_strength(I2C_EXTERN_SDA, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_drive_strength(I2C_EXTERN_SCL, GPIO_DRIVE_STRENGTH_4MA);

    // UART Functions.
    //  UART 0 is used for communication with the host (setup, commands, status)
    //  UART 1 is used for controlling the Bus-Servos
    //    Bus-Servo TX Enable
    gpio_set_function(SERVO_CTRL_TX_EN_GPIO, GPIO_FUNC_SIO);
    gpio_set_dir(SERVO_CTRL_TX_EN_GPIO, GPIO_OUT);
    gpio_set_drive_strength(SERVO_CTRL_TX_EN_GPIO, GPIO_DRIVE_STRENGTH_2MA);
    //    Initial output state
    gpio_put(SERVO_CTRL_TX_EN_GPIO, SERVO_CTRL_TX_DIS);     // Bus-Servo TX Disabled


    // GPIO Outputs (other than SPI, I2C, UART, and chip-selects
    //  Sensor Selects
    gpio_set_function(SENSOR_SEL_A0, GPIO_FUNC_SIO);
    gpio_set_dir(SENSOR_SEL_A0, GPIO_OUT);
    gpio_set_drive_strength(SENSOR_SEL_A0, GPIO_DRIVE_STRENGTH_2MA);
    gpio_put(SENSOR_SEL_A0, 0);
    gpio_set_function(SENSOR_SEL_A1, GPIO_FUNC_SIO);
    gpio_set_dir(SENSOR_SEL_A1, GPIO_OUT);
    gpio_set_drive_strength(SENSOR_SEL_A1, GPIO_DRIVE_STRENGTH_2MA);
    gpio_put(SENSOR_SEL_A1, 0);
    gpio_set_function(SENSOR_SEL_A2, GPIO_FUNC_SIO);
    gpio_set_dir(SENSOR_SEL_A2, GPIO_OUT);
    gpio_set_drive_strength(SENSOR_SEL_A2, GPIO_DRIVE_STRENGTH_2MA);
    gpio_put(SENSOR_SEL_A2, 0);

    // GPIO Inputs
    //   Rotary Encoder
    gpio_set_function(ROTARY_A_GPIO, GPIO_FUNC_SIO);
    gpio_set_dir(ROTARY_A_GPIO, GPIO_IN);
    gpio_set_pulls(ROTARY_A_GPIO, true, false);
    gpio_set_function(ROTARY_B_GPIO, GPIO_FUNC_SIO);
    gpio_set_dir(ROTARY_B_GPIO, GPIO_IN);
    gpio_set_pulls(ROTARY_B_GPIO, true, false);
    //    Sensor Input
    gpio_set_function(SENSOR_READ, GPIO_FUNC_SIO);
    gpio_set_dir(SENSOR_READ, GPIO_IN);
    gpio_set_pulls(SENSOR_READ, false, false);
    //    Switch Matrix
    gpio_set_function(SW_BANK_GPIO, GPIO_FUNC_SIO);
    gpio_set_dir(SW_BANK_GPIO, GPIO_IN);
    gpio_set_pulls(SW_BANK_GPIO, false, false);

    // Check the user input switch to see if it's pressed during startup.
    //  If yes, set 'debug_mode_enabled'
    if (user_switch_pressed()) {
        debug_mode_enable(true);
    }

    // Initialize the SPI Ops module before any SPI operations
    spi_ops_module_init();
    // Now initialize the Expansion I/O chip so the other devices will work
    expio_module_init();

    // Initialize the display
    disp_module_init();

#if HAS_RP2040_RTC
    // Initialize the board RTC.
    // Start on Sunday the 1st of January 2023 00:00:01
    datetime_t t = {
            .year = 2023,
            .month = 01,
            .day = 01,
            .dotw = 0, // 0 is Sunday
            .hour = 00,
            .min = 00,
            .sec = 01
    };
    disp_line_clear(4, false);
    disp_string(4, 0, "Init: RTC", false, true);
    rtc_init();
    rtc_set_datetime(&t);
    // clk_sys is >2000x faster than clk_rtc, so datetime is not updated immediately when rtc_set_datetime() is called.
    // tbe delay is up to 3 RTC clock cycles (which is 64us with the default clock settings)
    sleep_us(100);
#endif

    disp_line_clear(4, false);

    disp_line_clear(4, false);
    disp_string(4, 0, "Init: ADC", false, true);
    // Initialize hardware AD converter, enable onboard temperature sensor and
    //  select its channel.
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4); // Inputs 0-3 are GPIO pins, 4 is the built-in temp sensor

    // Initialize the Cursor Switches module.
    curswitch_module_init();

    // The PWM is used for a recurring interrupt in CMT. It will initialize it.

    return(retval);
}

uint8_t board_addr(void) {
    return eio_board_addr();
}

void boot_to_bootsel() {
    reset_usb_boot(0, 0);
}

void display_backlight_on(bool on) {
    eio_display_backlight_on(on);
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
    gpio_put(PICO_DEFAULT_LED_PIN, on);
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

void ledA_on(bool on) {
    eio_leda_on(on);
}

void ledB_on(bool on) {
    eio_ledb_on(on);
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
    return (gpio_get(SW_MAIN_USER_GPIO) == SW_MAIN_USER_PRESSED);
}


void debug_printf(const char* format, ...) {
    if (debug_mode_enabled()) {
        char buf[512];
        int index = 0;
        va_list xArgs;
        va_start(xArgs, format);
        index += vsnprintf(&buf[index], sizeof(buf) - index, format, xArgs);
        va_end(xArgs);
        if (disp_ready()) {
            text_color_pair_t cp;
            disp_text_colors_get(&cp);
            disp_text_colors_set(C16_LT_BLUE, C16_BLACK);
            disp_prints(buf, Paint);
            disp_text_colors_cp_set(&cp);
        }
    }
}

void error_printf(const char* format, ...) {
    char buf[512];
    int index = 0;
    va_list xArgs;
    va_start(xArgs, format);
    index += vsnprintf(&buf[index], sizeof(buf) - index, format, xArgs);
    va_end(xArgs);
    if (disp_ready()) {
        text_color_pair_t cp;
        disp_text_colors_get(&cp);
        disp_text_colors_set(C16_RED, C16_BLACK);
        disp_prints(buf, Paint);
        disp_text_colors_cp_set(&cp);
    }
}

void info_printf(const char* format, ...) {
    char buf[512];
    int index = 0;
    va_list xArgs;
    va_start(xArgs, format);
    index += vsnprintf(&buf[index], sizeof(buf) - index, format, xArgs);
    va_end(xArgs);
    if (disp_ready()) {
        text_color_pair_t cp;
        disp_text_colors_get(&cp);
        disp_text_colors_set(C16_BLUE, C16_BLACK);
        disp_prints(buf, Paint);
        disp_text_colors_cp_set(&cp);
    }
}

void warn_printf(const char* format, ...) {
    char buf[512];
    int index = 0;
    va_list xArgs;
    va_start(xArgs, format);
    index += vsnprintf(&buf[index], sizeof(buf) - index, format, xArgs);
    va_end(xArgs);
    if (disp_ready()) {
        text_color_pair_t cp;
        disp_text_colors_get(&cp);
        disp_text_colors_set(C16_ORANGE, C16_BLACK);
        disp_prints(buf, Paint);
        disp_text_colors_cp_set(&cp);
    }
}

void board_panic(const char* fmt, ...) {
    va_list xArgs;
    va_start(xArgs, fmt);
    error_printf(fmt, xArgs);
    panic(fmt, xArgs);
    va_end(xArgs);
}

