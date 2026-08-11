/*
    UART Receive implemented with the PIO. This allows more serial devices
    to be read from than the two (hard) UARTs that the Pico contains.

    Copyright 2025 AESilky (SilkyDESIGN)
    SPDX-License-Identifier: MIT

    Portions Copyright(c) 2020 Raspberry Pi(Trading) Ltd.
    SPDX - License - Identifier: BSD - 3 - Clause

*/

#include "pio_uart.h"

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "generated/uart.pio.h"


void pio_uart_baud_set(pio_sm_pocfg *smpocfg, uint baud) {
    // Run at 20X BAUD.
    // This is required for the PIO program to read in the middle of the bits.
    float div = (float)clock_get_hz(clk_sys) / (baud * pio_uart_rx_BIT_CLK_MULT);
    sm_config_set_clkdiv(&smpocfg->sm_cfg, div);

    pio_sm_init(smpocfg->pio, smpocfg->sm, smpocfg->offset, &smpocfg->sm_cfg);
}

pio_sm_pocfg pio_uart_rx_init(PIO pio, uint sm, uint pin, uint baud) {
    pio_sm_set_enabled(pio, sm, false);

    // disable pull-up and pull-down on gpio pin
    gpio_disable_pulls(pin);

    pio_sm_pocfg smpocfg;
    smpocfg.pio = pio;
    smpocfg.sm = sm;

    // install the program in the PIO shared instruction space
    const pio_program_t* pio_prgm = &pio_uart_rx_program;
    smpocfg.offset = pio_add_program(pio, pio_prgm);
    if (smpocfg.offset < 0) {
        return smpocfg;      // the program could not be added
    }
    pio_gpio_init(pio, pin);
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, false);

    smpocfg.sm_cfg = pio_uart_rx_program_get_default_config(smpocfg.offset);
    sm_config_set_in_pins(&smpocfg.sm_cfg, pin); // for WAIT, IN
    sm_config_set_jmp_pin(&smpocfg.sm_cfg, pin); // for JMP
    sm_config_set_in_pin_count(&smpocfg.sm_cfg, 1); // Only use 1 pin/bit for `mov x,PINS` for parity check
    pio_uart_baud_set(&smpocfg, baud);

    // Set up the error interrupt handler

    return smpocfg;
}

char uart_rx_program_getc(PIO pio, uint sm) {
    // 8-bit read from the uppermost byte of the FIFO, as data is left-justified
    io_rw_8* rxfifo_shift = (io_rw_8*)&pio->rxf[sm] + 3;
    while (pio_sm_is_rx_fifo_empty(pio, sm))
        tight_loop_contents();
    return (char)*rxfifo_shift;
}
