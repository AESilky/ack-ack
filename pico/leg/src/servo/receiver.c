/**
 * Radio control receiver functions.
 *
 * Use a PIO to read up to 4 channels of PWM signals and convert to angle.
 *
 * The PIO program runs a loop that takes 3 SM cycles times the 'period-multiplier'
 * for the period, and generates a pulse that is 3 SM cycles times the 'pulse-multiplier'.
 *
 * For the best resolution, the SM clock should be set as high as possible while being
 * able to hit the period and pulse with 32 bit multipliers.
 *
 * Standard servos run at a frequency of 50Hz (20ms (20,000µs) period), though high-performance
 * servos can run at a faster frequency (shorter period).
 * Standard servos represent 0° (neutral) with a pulse width of 1500µs, +90° with 2400µs
 * and -90° with 750µs. Therefore, 1° is a delta of 9.16µs.
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#include "receiver.h"
#include "system_defs.h"

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"

#include "pwm_rcv.pio.h"

// Default count values
#define DECIDEGREE_COUNT_DEF 920 // 9200ns per deg or 920ns per decidegree
#define NEUTRAL_COUNT_DEF 1500000


static int8_t _pio_irq;     // The PIO IRQ being used for the receiver
static volatile rc_channel_t _channels[PIO_RC_CHNL_COUNT];


// ////////// IRQ Functions ////////////

static void _on_recv_irq() {
    // IRQ called when the pio fifo for a receiver SM is not empty, i.e. there is data ready
    bool data_was_read;
    do {
        data_was_read = false;
        for (int i=0; i<PIO_RC_CHNL_COUNT; i++) {
            volatile rc_channel_t *channel = &_channels[i];
            if (!pio_sm_is_rx_fifo_empty(PIO_RECEIVER, PIO_SM_CHNL0+i)) {
                uint32_t raw = pio_sm_get(PIO_RECEIVER, PIO_SM_CHNL0+i);
                // Turn raw value into pulse width in nano-seconds
                channel->ns = ((0 - 1) - raw) * 100;
                channel->valid_pos = false;
                data_was_read = true;
            }
        }
    } while(data_was_read);
}


// ////////// Public Functions ////////////

void channel_disable(int channel_num) {
    channel_set_enabled(channel_num, false);
}

void channel_enable(int channel_num) {
    channel_set_enabled(channel_num, true);
}

void channel_get(int channel_num, rc_channel_t *channel) {
    volatile rc_channel_t *src = &_channels[channel_num];
    channel->zero_count = src->zero_count;
    channel->decidegree_count = src->decidegree_count;
    channel->ns = src->ns;
    channel->pos = src->pos;
    channel->valid_pos = src->valid_pos;
}

int32_t channel_get_angle(int channel_num) {
    volatile rc_channel_t *channel = &_channels[channel_num];
    int32_t pos = channel->pos;
    if (!channel->valid_pos) {
        // Need to calculate the position
        int32_t ns_offset = (channel->zero_count - channel->ns);
        pos = (ns_offset / (int32_t)channel->decidegree_count);
        channel->pos = pos;
        channel->valid_pos = true;
    }
    return (pos);
}

uint32_t channel_get_ns(int channel_num) {
    return (_channels[channel_num].ns);
}

void channel_set(int channel_num, rc_channel_t *channel) {
    volatile rc_channel_t *dst = &_channels[channel_num];
    dst->zero_count = channel->zero_count;
    dst->decidegree_count = channel->decidegree_count;
    dst->ns = channel->ns;
    dst->valid_pos = channel->valid_pos;
    dst->pos = channel->pos;
}

void channel_set_cnv_decideg(int channel_num, int32_t decideg_value) {
    volatile rc_channel_t *dst = &_channels[channel_num];
    dst->decidegree_count = decideg_value;
    dst->valid_pos = false; // Assume that the position is no longer valid.
}

void channel_set_cnv_zero(int channel_num, int32_t zero_value) {
    volatile rc_channel_t *dst = &_channels[channel_num];
    dst->zero_count = zero_value;
    dst->valid_pos = false; // Assume that the position is no longer valid.
}

void channel_set_enabled(int channel_num, bool enabled) {
    pio_sm_set_enabled(PIO_RECEIVER, PIO_SM_CHNL0+channel_num, enabled);
    bool any_enabled = enabled;
    for (int i=0; !any_enabled && i<PIO_RC_CHNL_COUNT; i++) {
        any_enabled = _channels[i].enabled;
    }
    if (any_enabled) {
        irq_set_enabled(_pio_irq, true); // Enable the IRQ
    }
    else {
        irq_set_enabled(_pio_irq, false); // Disable the IRQ
    }
}


void receiver_module_init() {
    _pio_irq = PIO_RC_IRQ;
    if (irq_get_exclusive_handler(_pio_irq)) {
        _pio_irq++;
        if (irq_get_exclusive_handler(_pio_irq)) {
            panic("All IRQs are in use");
        }
    }
    const uint irq_index = _pio_irq - PIO_RC_IRQ; // Get index of the IRQ
    irq_add_shared_handler(_pio_irq, _on_recv_irq, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY); // Add a shared IRQ handler
    irq_set_enabled(_pio_irq, false); // Disable the IRQ

    uint offset = pio_add_program(PIO_RECEIVER, &pwm_rcv_program);

    // Get the SM clock divisor for the PIO.
    //
    // Support various system clock settings.
    uint32_t clkhz = clock_get_hz(clk_sys);
    // Calculate a SM clock divisor so that a pulse width count value of 1 is 0.1µs.
    // In the PIO code, there are 2 SM clock cycles per period 'tick'.
    float clkdiv = (((float)clkhz) / 20000000.0f);

    for (int i = 0; i < PIO_RC_CHNL_COUNT; i++) {
        volatile rc_channel_t *channel = &_channels[i];
        channel->enabled = false;
        channel->zero_count = NEUTRAL_COUNT_DEF;
        channel->decidegree_count = DECIDEGREE_COUNT_DEF;
        channel->pos = 0;
        channel->valid_pos = false;
    }
    for (int i = 0; i < PIO_RC_CHNL_COUNT; i++) {
        pwm_rcv_program_init(PIO_RECEIVER, PIO_SM_CHNL0+i, offset, clkdiv, RECEIVER_CH1_PIN+i);
        pio_sm_clear_fifos(PIO_RECEIVER, PIO_SM_CHNL0+i);
        // Set pio to tell us when the RX FIFO is NOT empty
        pio_set_irqn_source_enabled(PIO_RECEIVER, irq_index, pis_sm0_rx_fifo_not_empty+i, true);
    }
}
