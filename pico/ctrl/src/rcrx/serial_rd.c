/*
    Serial Read PIO initialization.

    Copyright 2025 AESilky (SilkyDESIGN)
    SPDX-License-Identifier: MIT

*/
#include "serial_rd.h"

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "generated/serial_rd.pio.h"

void pio_serial_rd_deinit(PIO pio, uint sm, int offset, bool inverse) {
    pio_sm_set_enabled(pio, sm, false);
    const pio_program_t* pio_prgm = inverse ? &serial_rd_inv_program : &serial_rd_norm_program;
    pio_remove_program(pio, pio_prgm, offset);
}

pio_sm_pocfg pio_serial_rd_init(PIO pio, uint sm, uint pin, uint baud, bool inverse) {
    pio_sm_set_enabled(pio, sm, false);

    // disable pull-up and pull-down on gpio pin
    gpio_disable_pulls(pin);

    pio_sm_pocfg sm_pocfg;
    sm_pocfg.pio = pio;
    sm_pocfg.sm = sm;

    // install the program in the PIO shared instruction space
    const pio_program_t* pio_prgm = (inverse ? &serial_rd_inv_program : &serial_rd_norm_program);
    sm_pocfg.offset = pio_add_program(pio, pio_prgm);
    if (sm_pocfg.offset < 0) {
        return sm_pocfg;      // the program could not be added
    }
    pio_gpio_init(pio, pin);
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, false);

    sm_pocfg.sm_cfg = (inverse ? serial_rd_inv_program_get_default_config(sm_pocfg.offset) : serial_rd_norm_program_get_default_config(sm_pocfg.offset));
    sm_config_set_in_pins(&sm_pocfg.sm_cfg, pin); // for WAIT, IN
    sm_config_set_jmp_pin(&sm_pocfg.sm_cfg, pin); // for JMP
    // Run at 2X max BAUD. This allows the PIO program to read in the middle
    // of the bits once it detects the start-bit.
    float div = (float)clock_get_hz(clk_sys) / (baud * 2);
    sm_config_set_clkdiv(&sm_pocfg.sm_cfg, div);

    pio_sm_init(pio, sm, sm_pocfg.offset, &sm_pocfg.sm_cfg);

    return sm_pocfg;
}
