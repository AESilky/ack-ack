/*
    Initialize receiving SBUS data using the PIO.

    Copyright 2025 AESilky (SilkyDESIGN)
    SPDX-License-Identifier: MIT

*/
#include "rx_sbus.h"

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "generated/rx_sbus.pio.h"

void pio_rx_sbus_deinit(PIO pio, uint sm, int offset) {
    pio_sm_set_enabled(pio, sm, false);
    const pio_program_t* pio_prgm = &rx_sbus_program;
    pio_remove_program(pio, pio_prgm, offset);
}

pio_sm_cfg pio_rx_sbus_init(PIO pio, uint sm, uint pin, uint baud) {
    pio_sm_set_enabled(pio, sm, false);

    // disable pull-up and pull-down on gpio pin
    gpio_disable_pulls(pin);

    pio_sm_cfg smcfg;
    // install the program in the PIO shared instruction space
    const pio_program_t* pio_prgm = &rx_sbus_program;
    smcfg.offset = pio_add_program(pio, pio_prgm);
    if (smcfg.offset < 0) {
        return smcfg;      // the program could not be added
    }
    pio_gpio_init(pio, pin);
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, false);

    smcfg.sm_cfg = rx_sbus_program_get_default_config(smcfg.offset);
    sm_config_set_in_pins(&smcfg.sm_cfg, pin); // for WAIT, IN
    sm_config_set_jmp_pin(&smcfg.sm_cfg, pin); // for JMP
    // Run at 16X BAUD. This is required for the PIO program
    // to read in the middle of the bits.
    float div = (float)clock_get_hz(clk_sys) / (baud * 16);
    sm_config_set_clkdiv(&smcfg.sm_cfg, div);

    pio_sm_init(pio, sm, smcfg.offset, &smcfg.sm_cfg);

    return smcfg;
}

void pio_rx_sbus_msgget(PIO pio, uint sm, volatile unsigned char* buf) {
    // 8-bit read from the uppermost byte of the FIFO, as data is left-justified
    io_rw_8* rxfifo_shift = (io_rw_8*)&pio->rxf[sm] + 3;

    for (int i=0; i<SBUS_MSG_LEN; i++) {
        while (pio_sm_is_rx_fifo_empty(pio, sm))
            tight_loop_contents();
        *(buf+i) = (char)*rxfifo_shift;
    }
}
