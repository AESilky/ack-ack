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
/** Contains the bit values read by the PIO */
static volatile uint8_t _sensdata;
static volatile uint8_t _sensdata_p;
#define SAMPLES_NEEDED_ 2
static int _sampleindx;
static uint8_t _samplerd[SAMPLES_NEEDED_];


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
        _sensdata_p = _sensdata;
        if (d != _sensdata) {
            _sensdata = d;
            // Some bits have changed, post a message
            cmt_msg_t msg;
            cmt_msg_init(&msg, MSG_SENSBANK_CHG);
            msg.data.sensbank_chg.prev_bits = _sensdata_p;
            msg.data.sensbank_chg.bits = _sensdata;
            postHWCtrlMsg(&msg);
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
    // Date is input only, so disable the TX FIFO to make the RX FIFO deeper.
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

uint8_t sensbank_get(void) {
    return _sensdata;
}

sensbank_chg_t sensbank_get_chg(void) {
    sensbank_chg_t chg = {.bits = _sensdata, .prev_bits = _sensdata_p};
    return chg;
}


// ############################################################################
// Initialization and Maintainence Functions
// ############################################################################
//

void sensbank_start(void) {
    // Set pio to tell us when the FIFO is NOT empty
    pio_set_irqn_source_enabled(PIO_SENSBANK_BLOCK, PIO_SENSBANK_IRQ_IDX, pio_get_rx_fifo_not_empty_interrupt_source(PIO_SENSBANK_SM), true);
    // Enable the interrupt and start the PIO state machine
    pio_sm_set_enabled(PIO_SENSBANK_BLOCK, PIO_SENSBANK_SM, true);
    irq_set_enabled(PIO_SENSBANK_IRQ, true);
}


void sensbank_module_init(void) {
    static bool _initialized = false;
    int offset;

    if (_initialized) {
        board_panic("sensbank_module_init already called");
    }
    _initialized = true;

    _sensdata = SENSBANK_ALL_OPEN;
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
