/*
    UART Receive implemented with the PIO. This allows more serial devices
    to be read from that the two (hard) UARTs that the Pico contains.

    Copyright 2025 AESilky (SilkyDESIGN)
    SPDX-License-Identifier: MIT

    Portions Copyright(c) 2020 Raspberry Pi(Trading) Ltd.
    SPDX - License - Identifier: BSD - 3 - Clause

*/

#include "pio_uart.h"

#include "pico/stdlib.h"
#include "hardware/dma.h" //DMA is used to move the frame buffer to the PIO
#include "hardware/clocks.h"
#include "generated/uart.pio.h"

void pio_uart_rx_init(PIO pio, uint sm, uint offset, uint pin, uint baud) {
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, false);
    pio_gpio_init(pio, pin);
    gpio_pull_up(pin);

    pio_sm_config c = uart_rx_program_get_default_config(offset);
    sm_config_set_in_pins(&c, pin); // for WAIT, IN
    sm_config_set_jmp_pin(&c, pin); // for JMP
    // Shift to right, autopush disabled
    sm_config_set_in_shift(&c, true, false, 32);
    // Deeper FIFO as we're not doing any TX
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
    // SM transmits 1 bit per 8 execution cycles.
    float div = (float)clock_get_hz(clk_sys) / (8 * baud);
    sm_config_set_clkdiv(&c, div);

    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

char uart_rx_program_getc(PIO pio, uint sm) {
    // 8-bit read from the uppermost byte of the FIFO, as data is left-justified
    io_rw_8* rxfifo_shift = (io_rw_8*)&pio->rxf[sm] + 3;
    while (pio_sm_is_rx_fifo_empty(pio, sm))
        tight_loop_contents();
    return (char)*rxfifo_shift;
}
