/**
 * @brief Sensor Bank reading.
 * @ingroup sensbank
 *
 * Continuously reads the Sensor Bank and posts messages with state and changes.
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */

#include "sensbank.h"
#include "sensbank.pio.h"
#include "adc1015.h"

#include "system_defs.h"
#include "board.h"
#include "cmt/cmt.h"

#include "hardware/clocks.h"

// ############################################################################
// Value Definitions
// ############################################################################
//
#define SENSBANK_ALL_OPEN    0xFF

// ############################################################################
// Function Declarations
// ############################################################################
//


// ############################################################################
// Data
// ############################################################################
//
static bool _adc_present;
/** Contains the bit values read by the PIO */
static volatile sensbank_cah_t _sensdata;
#define SAMPLES_NEEDED_ 2
static int _sampleindx;
static volatile uint8_t _samplerd[SAMPLES_NEEDED_];


// ############################################################################
// Interrupt Handlers
// ############################################################################
//

static void pio_irq_func(void) {
    // IRQ called when the pio fifo is not empty, i.e. there is a sensbank
    // value available. This occurs ~80 per second (12-13ms).
    while (!pio_sm_is_rx_fifo_empty(PIO_SENSBANK_BLOCK, PIO_SENSBANK_SM)) {
        uint32_t dw = pio_sm_get(PIO_SENSBANK_BLOCK, PIO_SENSBANK_SM);
        // We want the value to be the same twice, to debounce switch changes.
        uint8_t d = dw & 0x000000FF;
        _samplerd[_sampleindx++] = d;
        if (_sampleindx != SAMPLES_NEEDED_) {
            continue;
        }
        else {
            _sampleindx = 0;
            if (_samplerd[0] != _samplerd[1]) {
                continue;
            }
        }
        // There were two consecutive reads the same. Check the value.
        _sensdata.prev_bits = _sensdata.bits;
        if (d != _sensdata.bits) {
            _sensdata.bits = d;
            // Some bits have changed, post a message
            cmt_msg_t msg;
            cmt_msg_init(&msg, MSG_SENSBANK_CHG);
            msg.data.sensbank_chg.prev_bits = _sensdata.prev_bits;
            msg.data.sensbank_chg.bits = _sensdata.bits;
            postHWRTMsg(&msg);
            postDCSMsgDiscardable(&msg); // DCS is for status only
        }
    }
}


// ############################################################################
// Message Handlers
// ############################################################################
//


// ############################################################################
// Internal Functions
// ############################################################################
//
static void _sensbank_program_init(PIO pio, uint sm, uint offset, uint opin, uint ipin) {
    // Set the 3 o-pin directions to output at the PIO
    pio_sm_set_consecutive_pindirs(pio, sm, opin, 3, true);
    // Set the i-pin direction to input at the PIO
    pio_sm_set_consecutive_pindirs(pio, sm, ipin, 1, false);
    // Connect these GPIOs to this PIO block
    pio_gpio_init(pio, opin);
    pio_gpio_init(pio, opin + 1);
    pio_gpio_init(pio, opin + 2);
    pio_gpio_init(pio, ipin);

    pio_sm_config c = sensbank_program_get_default_config(offset);
    // Set the OUT base pin to the provided `opin` parameter. This is the A0 bit,
    // and the next 2 numbered GPIO are A1 and A2.
    sm_config_set_out_pins(&c, opin, 3);
    // Set the IN base pin to the provided `ipin` parameter. This is the sensor input.
    sm_config_set_in_pins(&c, ipin);
    // Shift 8 bits left with AutoPush. Shifting left puts the bit read from address 7 in bit 7.
    sm_config_set_in_shift(
        &c,
        false, // Shift-to-right = false (i.e. shift to left)
        false, // Auto-push not enabled
        8      // Auto-push threshold = 8
    );
    // Data is input only, so join the TX FIFO to the RX FIFO.
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);

    // Set the clock divider to sample the 8 inputs about 80 times a second.
    // (each bit sample takes 4 clock cycles)
    float div = clock_get_hz(clk_sys) / (80 * (4 * 8));
    sm_config_set_clkdiv(&c, div);

    // Load our configuration, but don't start it
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, false);
}


// ############################################################################
// Public Functions
// ############################################################################
//

sensbank_cah_t sensbank_get(void) {
    return _sensdata;
}

void sensbank_housekeeping() {
    if (_adc_present) {
        adc1015_housekeeping();
    }
}

// ############################################################################
// Initialization and Maintainence Functions
// ############################################################################
//

void sensbank_start(void) {
    // Set pio to tell us when the FIFO is NOT empty
    pio_set_irqn_source_enabled(PIO_SENSBANK_BLOCK, PIO_SENSBANK_IRQ_IDX, pio_get_rx_fifo_not_empty_interrupt_source(PIO_SENSBANK_SM), true);
    // Enable the interrupt and start the PIO state machine
    irq_set_enabled(PIO_SENSBANK_IRQ, true);
    pio_sm_set_enabled(PIO_SENSBANK_BLOCK, PIO_SENSBANK_SM, true);
    // If the ADC is connected, start reading the ADC
    uint8_t adc_addr = 0x48; // The ADC is at address 0x48 or 0x49
    if (i2c_device_present(adc_addr)) {
        _adc_present = true;
    }
    else if (i2c_device_present(++adc_addr)) {
        _adc_present = true;
    }
    if (_adc_present) {
        adc1015_module_init(I2C_EXTERN, adc_addr, 21); // Init the ADC and set the rate to 3 per second
        adc1015_start();
    }
}


void sensbank_module_init(void) {
    static bool _initialized = false;
    if (_initialized) {
        board_panic("sensbank_module_init already called");
    }
    _initialized = true;

    _adc_present = false;

    int offset;
    _sensdata.bits = SENSBANK_ALL_OPEN;
    _sensdata.prev_bits = SENSBANK_ALL_OPEN;
    _sampleindx = 0;

    // Load the PIO program
    offset = pio_add_program(PIO_SENSBANK_BLOCK, &sensbank_program);
    if (offset < 0) {
        board_panic("sensbank_module_init - Unable to load PIO program");
    }
    // Enable interrupt
    irq_set_exclusive_handler(PIO_SENSBANK_IRQ, pio_irq_func); // Set the IRQ handler
    irq_set_enabled(PIO_SENSBANK_IRQ, false); // Disable the IRQ for now

    _sensbank_program_init(PIO_SENSBANK_BLOCK, PIO_SENSBANK_SM, offset, SENSOR_SEL_A0, SENSOR_READ);
}
