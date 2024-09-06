#ifndef _SYSTEM_DEFS_H_
#define _SYSTEM_DEFS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "pico/stdlib.h"

#define LEG_VERSION_INFO "v0.1"  // ZZZ get from a central name/version string

/**
 * @brief Flag indicating that the board is a Pico-W
 */
#ifdef PICO_DEFAULT_LED_PIN
    #define BOARD_IS_PICO
#else
    #define BOARD_IS_PICOW
#endif


#undef putc     // Undefine so the standard macros will not be used
#undef putchar  // Undefine so the standard macros will not be used

#include "hardware/exception.h"
#include "hardware/pio.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

// I2C
//
// Note: 'Pins' are the GPIO number, not the physical pins on the device.
//
#define I2C_PORT                i2c1
#define I2C_SDA_PIN                2       // DP-4
#define I2C_SCL_PIN                3       // DP-5
#define SSD1306_I2C_CLK          400


// PIO Servo Control
#define PIO_SERVOS              pio0        // Use PIO0 for servo control
#define PIO_SERVO_COUNT            4        // The number of servos to control (1-4)
#define PIO_SM_SERVO0              0        // State Machine for first servo (servos are in order)
#define SERVO1_PIN                 6        // DP-9 Servo pins are in order

// PIO RC Receiver
#define PIO_RECEIVER            pio1        // Use PIO1 to decode receiver
#define PIO_RC_IRQ              PIO1_IRQ_0  // PIO IRQ to use for the receiver interrupt
#define PIO_RC_CHNL_COUNT          4        // The number of channels to monitor
#define PIO_SM_CHNL0               0        // State Machine for the first channel (channels are in order)
#define RECEIVER_CH1_PIN          10        // DP-14 Receiver Channel pins are in order


// Other GPIO
#define TONE_DRIVE              21          // DP-27 - Buzzer drive
#define USER_INPUT_SW           22          // DP-29

#define IRQ_INPUT_SW            USER_INPUT_SW    // DP-29

// User Input Switch support
#define USER_SW_OPEN             1      // Switch is connected to GND
#define USER_SW_CLOSED           0

// Buzzer/Tone support
#define TONE_OFF 0
#define TONE_ON  1

#ifdef __cplusplus
}
#endif
#endif // _SYSTEM_DEFS_H_
