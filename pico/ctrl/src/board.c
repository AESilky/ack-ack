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
#include "pico/malloc.h"
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
#include "debug_support.h"
#include "display/display.h"
#include "eeprom/eeprom.h"
#include "expio/expio.h"
#include "rcrx/rcrx.h"
#include "spi_ops.h"
#include "util/util.h"

#include <stdlib.h>

typedef struct I2C_DEV_LL {
    uint8_t addr;
    struct I2C_DEV_LL *next;
} i2c_dev_ll_t;

i2c_dev_ll_t* _i2c_devices;

// Internal function declarations

void _i2c_scan(i2c_inst_t* i2c) {
    for (int addr = 0; addr < (1 << 7); ++addr) {
        // Perform a 1-byte dummy read from the probe address. If a slave
        // acknowledges this address, the function returns the number of bytes
        // transferred. If the address byte is ignored, the function returns
        // -1.
        int ret;
        uint8_t rxdata;

        // Skip over reserved addresses...
        // I2C reserves some addresses for special purposes. We exclude these from the scan.
        // These are any addresses of the form 000 0xxx or 111 1xxx
        if ((addr & 0x78) == 0 || (addr & 0x78) == 0x78) {
            continue;
        }
        ret = i2c_read_timeout_us(i2c, addr, &rxdata, 1, false, 500);
        if (ret >= 0) {
            // Allocate a device marker and save the address
            i2c_dev_ll_t *dev = (i2c_dev_ll_t*)malloc(sizeof(i2c_dev_ll_t));
            dev->next = _i2c_devices;
            dev->addr = addr;
            _i2c_devices = dev;
        }
    }
}

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

    //
    // Do initialization that is common for both board addresses (0&1).
    //
    _i2c_devices = NULL; // This will be filled in with the I2C devices found later

    // Chip selects for the SPI peripherals
    gpio_set_function(SPI_ADDR_0, GPIO_FUNC_SIO);
    gpio_set_dir(SPI_ADDR_0, GPIO_OUT);
    gpio_set_function(SPI_ADDR_1, GPIO_FUNC_SIO);
    gpio_set_dir(SPI_ADDR_1, GPIO_OUT);
    gpio_set_drive_strength(SPI_ADDR_0, GPIO_DRIVE_STRENGTH_2MA);           // CS goes to a single device
    gpio_set_drive_strength(SPI_ADDR_1, GPIO_DRIVE_STRENGTH_2MA);           // CS goes to a single device
    // Initial output state
    gpio_put(SPI_ADDR_0, 1);
    gpio_put(SPI_ADDR_1, 1);

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

    // I2C Isn't directly used on the board, but is provided on headers for external use.
    //   GPIO config is as recommended in the RP2350 Datasheet.
    i2c_init(I2C_EXTERN, I2C_EXTERN_CLK_SPEED);
    gpio_set_function(I2C_EXTERN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_EXTERN_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_EXTERN_SDA);
    gpio_pull_up(I2C_EXTERN_SCL);
    gpio_set_drive_strength(I2C_EXTERN_SDA, GPIO_DRIVE_STRENGTH_8MA);
    gpio_set_drive_strength(I2C_EXTERN_SCL, GPIO_DRIVE_STRENGTH_8MA);
    gpio_set_slew_rate(I2C_EXTERN_SDA, GPIO_SLEW_RATE_SLOW);
    gpio_set_slew_rate(I2C_EXTERN_SCL, GPIO_SLEW_RATE_SLOW);
    gpio_set_input_hysteresis_enabled(I2C_EXTERN_SDA, true);
    gpio_set_input_hysteresis_enabled(I2C_EXTERN_SCL, true);

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

    //    Sensor Input
    gpio_set_function(SENSOR_READ, GPIO_FUNC_SIO);
    gpio_set_dir(SENSOR_READ, GPIO_IN);
    gpio_set_pulls(SENSOR_READ, false, false);

    //
    // Module initialization that is needed for other modules to initialize.
    //

    // Initialize the SPI Ops module before any SPI operations
    spi_ops_module_init();
    // Initialize the Expansion I/O chip so the other devices will work (including `board_jumper` used below)
    expio_module_init();

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
    rtc_init();
    rtc_set_datetime(&t);
    // clk_sys is >2000x faster than clk_rtc, so datetime is not updated immediately when rtc_set_datetime() is called.
    // tbe delay is up to 3 RTC clock cycles (which is 64us with the default clock settings)
    sleep_us(100);
#endif

    // Initialize hardware AD converter, enable onboard temperature sensor and
    //  select its channel.
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4); // Inputs 0-3 are GPIO pins, 4 is the built-in temp sensor

    //
    // Do board specific initialization based on ADDR 0 or 1
    //
#if (BOARD_ADDR == 0)
    // STOP input switch
    gpio_set_function(STOP_INPUT_SW, GPIO_FUNC_SIO);
    gpio_set_dir(STOP_INPUT_SW, GPIO_IN);
    gpio_set_pulls(SW_MAIN_USER_GPIO, true, false);
    // Sensor and Servo Power Control
    //  Power Distribution board has an enable for 12V, 7.5V, and 5.0V outputs
    gpio_set_function(AUX_PWR_CTRL, GPIO_FUNC_SIO);
    gpio_put(AUX_PWR_CTRL, SENSVO_PWR_OFF);         // Start with Power Disabled
    gpio_set_dir(AUX_PWR_CTRL, GPIO_OUT);
    gpio_set_drive_strength(AUX_PWR_CTRL, GPIO_DRIVE_STRENGTH_2MA);
    //    Initial output state
    gpio_put(AUX_PWR_CTRL, SENSVO_PWR_OFF);         // Start with Power Disabled
    // UART Functions.
    //  UART 0 is used for communication with the host (setup, commands, status)
    //  UART 1 is used for controlling the Bus-Servos
    //    Bus-Servo TX Enable
    gpio_set_function(SERVO_CTRL_TX_EN_GPIO, GPIO_FUNC_SIO);
    gpio_set_dir(SERVO_CTRL_TX_EN_GPIO, GPIO_OUT);
    gpio_set_drive_strength(SERVO_CTRL_TX_EN_GPIO, GPIO_DRIVE_STRENGTH_2MA);
    //    Initial output state
    gpio_put(SERVO_CTRL_TX_EN_GPIO, SERVO_CTRL_TX_DIS);     // Bus-Servo TX Disabled
    //    User Switch
    gpio_set_function(SW_MAIN_USER_GPIO, GPIO_FUNC_SIO);
    gpio_set_dir(SW_MAIN_USER_GPIO, GPIO_IN);
    gpio_set_pulls(SW_MAIN_USER_GPIO, false, false);
#endif
    // This could be an else, but it's only done once and this allows for more than 0 & 1.
#if (BOARD_ADDR == 1)
        //
        // Board '1' has the Display and Switch board connected.
        //
        // Display Control/Data signal
        gpio_set_function(SPI_DISP_CD, GPIO_FUNC_SIO);
        gpio_set_dir(SPI_DISP_CD, GPIO_OUT);
        gpio_set_drive_strength(SPI_DISP_CD, GPIO_DRIVE_STRENGTH_2MA);          // C/D goes to a single device
        // Initial output state
        gpio_put(SPI_DISP_CD, 1);
        // SPI 1 Pins for Touch Panel
        gpio_set_function(SPI_TOUCH_SCK, GPIO_FUNC_SPI);
        gpio_set_function(SPI_TOUCH_MOSI, GPIO_FUNC_SPI);
        gpio_set_function(SPI_TOUCH_MISO, GPIO_FUNC_SPI);
        // SPI 1 Signal drive strengths
        gpio_set_drive_strength(SPI_TOUCH_SCK, GPIO_DRIVE_STRENGTH_2MA);     // Two devices connected
        gpio_set_drive_strength(SPI_TOUCH_MOSI, GPIO_DRIVE_STRENGTH_2MA);    // Two devices connected
        // SPI 1 initialization for the Touch Panel. Use SPI at 2MHz.
        spi_init(SPI_TOUCH_DEVICE, SPI_TOUCH_SPEED);
        //   Rotary Encoder
        gpio_set_function(ROTARY_A_GPIO, GPIO_FUNC_SIO);
        gpio_set_dir(ROTARY_A_GPIO, GPIO_IN);
        gpio_set_pulls(ROTARY_A_GPIO, true, false);
        gpio_set_function(ROTARY_B_GPIO, GPIO_FUNC_SIO);
        gpio_set_dir(ROTARY_B_GPIO, GPIO_IN);
        gpio_set_pulls(ROTARY_B_GPIO, true, false);
        //    Switch Matrix
        gpio_set_function(SW_BANK_GPIO, GPIO_FUNC_SIO);
        gpio_set_dir(SW_BANK_GPIO, GPIO_IN);
        gpio_set_pulls(SW_BANK_GPIO, false, false);
#endif


    // Scan the I2C bus to see what devices (device at an address) are present
    _i2c_scan(I2C_EXTERN);

    // If the EEPROM is present, initialize the module.
    if (i2c_device_present(EEPROM_ADDR1)) {
        eeprom_module_init(I2C_EXTERN, EEPROM_ADDR1, EEPROM_IMMEDIATE);
    }
    else if (i2c_device_present(EEPROM_ADDR2)) {
        eeprom_module_init(I2C_EXTERN, EEPROM_ADDR2, EEPROM_BUFFERED);
    }


    // The PWM is used for a recurring interrupt in CMT. It will initialize it.

    return(retval);
}

uint8_t board_jumper(void) {
    return eio_board_jumper();
}

void boot_to_bootsel() {
    reset_usb_boot(0, 0);
}

void display_backlight_on(bool on) {
    eio_display_backlight_on(on);
}

bool i2c_device_present(uint8_t addr) {
    i2c_dev_ll_t *i2c_dev = _i2c_devices;
    while (i2c_dev) {
        if (i2c_dev->addr == addr) {
            return true;
        }
        i2c_dev = i2c_dev->next;
    }
    return false;
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
        cmt_run_after_ms(ms, _led_flash_cont, NULL);
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
            cmt_run_after_ms(off_time, _led_on_off_cont, (void*)pattern);
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
#if (BOARD_ADDR == 1)
        if (disp_ready()) {
            text_color_pair_t cp;
            disp_text_colors_get(&cp);
            disp_text_colors_set(C16_LT_BLUE, C16_BLACK);
            disp_prints(buf, Paint);
            disp_text_colors_cp_set(&cp);
        }
#else
        printf("%s", buf);
#endif
    }
}

void error_printf(const char* format, ...) {
    char buf[512];
    int index = 0;
    va_list xArgs;
    va_start(xArgs, format);
    index += vsnprintf(&buf[index], sizeof(buf) - index, format, xArgs);
    va_end(xArgs);
#if (BOARD_ADDR == 1)
    if (disp_ready()) {
        text_color_pair_t cp;
        disp_text_colors_get(&cp);
        disp_text_colors_set(C16_RED, C16_BLACK);
        disp_prints(buf, Paint);
        disp_text_colors_cp_set(&cp);
    }
#else
    printf("%s", buf);
#endif
}

void info_printf(const char* format, ...) {
    char buf[512];
    int index = 0;
    va_list xArgs;
    va_start(xArgs, format);
    index += vsnprintf(&buf[index], sizeof(buf) - index, format, xArgs);
    va_end(xArgs);
#if (BOARD_ADDR == 1)
    if (disp_ready()) {
        text_color_pair_t cp;
        disp_text_colors_get(&cp);
        disp_text_colors_set(C16_BLUE, C16_BLACK);
        disp_prints(buf, Paint);
        disp_text_colors_cp_set(&cp);
    }
#else
    printf("%s", buf);
#endif
}

void warn_printf(const char* format, ...) {
    char buf[512];
    int index = 0;
    va_list xArgs;
    va_start(xArgs, format);
    index += vsnprintf(&buf[index], sizeof(buf) - index, format, xArgs);
    va_end(xArgs);
#if (BOARD_ADDR == 1)
    if (disp_ready()) {
        text_color_pair_t cp;
        disp_text_colors_get(&cp);
        disp_text_colors_set(C16_ORANGE, C16_BLACK);
        disp_prints(buf, Paint);
        disp_text_colors_cp_set(&cp);
    }
#else
    printf("%s", buf);
#endif
}

void board_panic(const char* fmt, ...) {
    // Turn the LED on before the panic
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    va_list xArgs;
    va_start(xArgs, fmt);
    error_printf(fmt, xArgs);
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    panic(fmt, xArgs);
    va_end(xArgs);
}

