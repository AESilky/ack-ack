/*
    Initialize receiving SBUS data using the PIO.

    Copyright 2025 AESilky (SilkyDESIGN)
    SPDX-License-Identifier: MIT

*/
#include "rx_sbus.h"
#include "generated/rx_sbus.pio.h"

#include "rcrx_t.h"
#include "rx_cmn.h"

#include "board.h"
#include "cmt/cmt.h"
#include "debug_support.h"
#include "system_defs.h"

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define SBUS_HEADER_IDX 0
#define SBUS_HEADER_VALUE 0x0F
#define SBUS_FOOTER_IDX 24
#define SBUS_FOOTER_VALUE 0x00
#define CH17_MASK_ 0x01
#define CH18_MASK_ 0x02
#define LOST_FRAME_MASK_ 0x04
#define FAILSAFE_MASK_ 0x08

#define SBUS_NORM_RANGE     1640    // 172 - 1811
#define SBUS_NORM_MULT_ADJ  12.1957
#define SBUS_NORM_ADD_ADJ   (-12098.1344)
#define SBUS_EXT_RANGE      2048    // 0 - 2047
#define SBUS_EXT_MULT_ADJ   14.6556
#define SBUS_EXT_ADD_ADJ   (-14999)

static bool _initialized = false;

static rcrx_state_t* _channel_state;
static uint16_t _frames_lost;

// ///////////////////////////////////////////////////////////////////////// //
// Function Declarations                                            //
// ///////////////////////////////////////////////////////////////////////// //

static int16_t _chval_raw_convert(uint16_t raw_val, bool extended);

static void _debug_print_bufraw(const volatile uint8_t* buf);

// ///////////////////////////////////////////////////////////////////////// //
// Interrupt Handlers                                                        //
// ///////////////////////////////////////////////////////////////////////// //


// ///////////////////////////////////////////////////////////////////////// //
// Message Handlers                                                          //
// ///////////////////////////////////////////////////////////////////////// //

uint16_t rx_sbus_protocol_processor() {
    // Check that it appears to be valid (has the correct header and footer byte values)
    volatile uint8_t* buf = _rc_bufs.msg_bufs.msg_enqueue;
    uint16_t changes = 0;
    if (buf[SBUS_HEADER_IDX] != SBUS_HEADER_VALUE || buf[SBUS_FOOTER_IDX] != SBUS_FOOTER_VALUE) {
        rxcmn_update_error_count();
        printf("RC-SBUS Header/Footer incorrect: %02hX|%02hX\n", buf[SBUS_HEADER_IDX], buf[SBUS_FOOTER_IDX]);
    }
    else {
        // Show that we are processing
        ledB_on(true);
        _rcrx_msg_same_data_cnt = 0;

        // Process the channel data from the received message.
        int ch_index = 0;
        bool extended = false;
        //  CH-1
        uint16_t rval = ((((uint16_t)buf[1]) | ((uint16_t)buf[2] << 8)) & 0x07FF);
        if (_channel_state->ch_data[ch_index].raw_v != rval) {
            _channel_state->ch_data[ch_index].raw_v = rval;
            _channel_state->ch_data[ch_index].v = _chval_raw_convert(rval, extended);
            changes |= (1 << ch_index);
        }
        ch_index = 1;
        //  CH-2
        rval = ((((uint16_t)buf[2] >> 3) | ((uint16_t)buf[3] << 5)) & 0x07FF);
        if (_channel_state->ch_data[ch_index].raw_v != rval) {
            _channel_state->ch_data[ch_index].raw_v = rval;
            _channel_state->ch_data[ch_index].v = _chval_raw_convert(rval, extended);
            changes |= (1 << ch_index);
        }
        ch_index = 2;
        //  CH-3
        rval = ((((uint16_t)buf[3] >> 6) | ((uint16_t)buf[4] << 2) | ((uint16_t)(buf[5] << 10))) & 0x07FF);
        if (_channel_state->ch_data[ch_index].raw_v != rval) {
            _channel_state->ch_data[ch_index].raw_v = rval;
            _channel_state->ch_data[ch_index].v = _chval_raw_convert(rval, extended);
            changes |= (1 << ch_index);
        }
        ch_index = 3;
        //  CH-4
        rval = ((((uint16_t)buf[5] >> 1) | ((uint16_t)buf[6] << 7)) & 0x07FF);
        if (_channel_state->ch_data[ch_index].raw_v != rval) {
            _channel_state->ch_data[ch_index].raw_v = rval;
            _channel_state->ch_data[ch_index].v = _chval_raw_convert(rval, extended);
            changes |= (1 << ch_index);
        }
        ch_index = 4;
        //  CH-5
        rval = ((((uint16_t)buf[6] >> 4) | ((uint16_t)buf[7] << 4)) & 0x07FF);
        if (_channel_state->ch_data[ch_index].raw_v != rval) {
            _channel_state->ch_data[ch_index].raw_v = rval;
            _channel_state->ch_data[ch_index].v = _chval_raw_convert(rval, extended);
            changes |= (1 << ch_index);
        }
        ch_index = 5;
        //  CH-6
        rval = ((((uint16_t)buf[7] >> 7) | ((uint16_t)buf[8] << 1) | ((uint16_t)buf[9] << 9)) & 0x07FF);
        if (_channel_state->ch_data[ch_index].raw_v != rval) {
            _channel_state->ch_data[ch_index].raw_v = rval;
            _channel_state->ch_data[ch_index].v = _chval_raw_convert(rval, extended);
            changes |= (1 << ch_index);
        }
        ch_index = 6;
        //  CH-7
        rval = ((((uint16_t)buf[9] >> 2) | ((uint16_t)buf[10] << 6)) & 0x07FF);
        if (_channel_state->ch_data[ch_index].raw_v != rval) {
            _channel_state->ch_data[ch_index].raw_v = rval;
            _channel_state->ch_data[ch_index].v = _chval_raw_convert(rval, extended);
            changes |= (1 << ch_index);
        }
        ch_index = 7;
        //  CH-8
        rval = ((((uint16_t)buf[10] >> 5) | ((uint16_t)buf[11] << 3)) & 0x07FF);
        if (_channel_state->ch_data[ch_index].raw_v != rval) {
            _channel_state->ch_data[ch_index].raw_v = rval;
            _channel_state->ch_data[ch_index].v = _chval_raw_convert(rval, extended);
            changes |= (1 << ch_index);
        }
        ch_index = 8;
        //  CH-9
        rval = ((((uint16_t)buf[12]) | ((uint16_t)buf[13] << 8)) & 0x07FF);
        if (_channel_state->ch_data[ch_index].raw_v != rval) {
            _channel_state->ch_data[ch_index].raw_v = rval;
            _channel_state->ch_data[ch_index].v = _chval_raw_convert(rval, extended);
            changes |= (1 << ch_index);
        }
        ch_index = 9;
        //  CH-10
        rval = ((((uint16_t)buf[13] >> 3) | ((uint16_t)buf[14] << 5)) & 0x07FF);
        if (_channel_state->ch_data[ch_index].raw_v != rval) {
            _channel_state->ch_data[ch_index].raw_v = rval;
            _channel_state->ch_data[ch_index].v = _chval_raw_convert(rval, extended);
            changes |= (1 << ch_index);
        }
        ch_index = 10;
        //  CH-11
        rval = ((((uint16_t)buf[14] >> 6) | ((uint16_t)buf[15] << 2) | ((uint16_t)buf[16] << 10)) & 0x07FF);
        if (_channel_state->ch_data[ch_index].raw_v != rval) {
            _channel_state->ch_data[ch_index].raw_v = rval;
            _channel_state->ch_data[ch_index].v = _chval_raw_convert(rval, extended);
            changes |= (1 << ch_index);
        }
        ch_index = 11;
        //  CH-12
        rval = ((((uint16_t)buf[16] >> 1) | ((uint16_t)buf[17] << 7)) & 0x07FF);
        if (_channel_state->ch_data[ch_index].raw_v != rval) {
            _channel_state->ch_data[ch_index].raw_v = rval;
            _channel_state->ch_data[ch_index].v = _chval_raw_convert(rval, extended);
            changes |= (1 << ch_index);
        }
        ch_index = 12;
        //  CH-13
        rval = ((((uint16_t)buf[17] >> 4) | ((uint16_t)buf[18] << 4)) & 0x07FF);
        if (_channel_state->ch_data[ch_index].raw_v != rval) {
            _channel_state->ch_data[ch_index].raw_v = rval;
            _channel_state->ch_data[ch_index].v = _chval_raw_convert(rval, extended);
            changes |= (1 << ch_index);
        }
        ch_index = 13;
        //  CH-14
        rval = ((((uint16_t)buf[18] >> 7) | ((uint16_t)buf[19] << 1) | ((uint16_t)buf[20] << 9)) & 0x07FF);
        if (_channel_state->ch_data[ch_index].raw_v != rval) {
            _channel_state->ch_data[ch_index].raw_v = rval;
            _channel_state->ch_data[ch_index].v = _chval_raw_convert(rval, extended);
            changes |= (1 << ch_index);
        }
        ch_index = 14;
        //  CH-15
        rval = ((((uint16_t)buf[20] >> 2) | ((uint16_t)buf[21] << 6)) & 0x07FF);
        if (_channel_state->ch_data[ch_index].raw_v != rval) {
            _channel_state->ch_data[ch_index].raw_v = rval;
            _channel_state->ch_data[ch_index].v = _chval_raw_convert(rval, extended);
            changes |= (1 << ch_index);
        }
        ch_index = 15;
        //  CH-16
        rval = ((((uint16_t)buf[21] >> 5) | ((uint16_t)buf[22] << 3)) & 0x07FF);
        if (_channel_state->ch_data[ch_index].raw_v != rval) {
            _channel_state->ch_data[ch_index].raw_v = rval;
            _channel_state->ch_data[ch_index].v = _chval_raw_convert(rval, extended);
            changes |= (1 << ch_index);
        }
        ch_index = 16;
        /* CH 17 */
        // FrSKY doesn't seem to send these binary channels
        // rval = ((buf[23] & CH17_MASK_) != 0 ? 1 : 0);
        // _channel_state->ch_data[ch_index].raw_v = rval;
        // _channel_state->ch_data[ch_index].v = (rval == 1 ? RC_SYS_CHVAL_MAX : RC_SYS_CHVAL_MIN);
        // ch_index = 17;
        /* CH 18 */
        // FrSKY doesn't send these binary channels
        // rval = ((buf[23] & CH18_MASK_) != 0 ? 1 : 0);
        // _channel_state->ch_data[ch_index].raw_v = rval;
        // _channel_state->ch_data[ch_index].v = (rval == 1 ? RC_SYS_CHVAL_MAX : RC_SYS_CHVAL_MIN);
        // ch_index = 18;
        /* Lost Frame */
        bool b = ((buf[23] & LOST_FRAME_MASK_) != 0);
        if (b) {
            _frames_lost++;
            _channel_state->frames_lost = _frames_lost;
        }
        /* Failsafe */
        b = ((buf[23] & FAILSAFE_MASK_) != 0);
        _channel_state->failsafe = b;

        _channel_state->changed |= changes;
        // ZZZ - DEBUG
        static int zzz = 15;
        static int zzz_test = 0;
        if ((changes & 0x10) == zzz_test) {
            if (0 == --zzz) {
                if (zzz_test == 0) {
                    // We've had 15 times that CH5 didn't change.
                    // Print the incoming buffer and the raw values.
                    printf("SBUS-RX CH5 no change...\n");
                    _debug_print_bufraw(buf);
                    zzz_test = 0x10;
                    zzz = 1;
                }
                else {
                    // We had 15+ times that CH5 didn't change and now CH5 has changed
                    // Print the incoming buffer and the raw values.
                    printf("SBUS-RX CH5 changed...\n");
                    _debug_print_bufraw(buf);
                    // Put things back to check again.
                    zzz = 15;
                    zzz_test = 0;
                }
            }
        }
        else if (zzz_test == 0) {
            zzz = 15;
        }

        ledB_on(false);
    }
    _rxcmn_en_next_rx();

    return (changes);
}


// ///////////////////////////////////////////////////////////////////////// //
// Internal Functions                                                        //
// ///////////////////////////////////////////////////////////////////////// //

/**
 * @brief Convert a raw channel value (SBUS value) to our system representation.
 *
 * Convert to the system representation (see the README in this module). We are
 * using MAVLink values where -10000 is -100%, 0 is 0%, 10000 is 100%
 *
 * @param raw_val The raw SBUS channel value.
 * @param extended True for SBUS extended (-150% to 150% : 0 to 2047)
 * @return uint16_t The system value
 */
static int16_t _chval_raw_convert(uint16_t raw_val, bool extended) {
    /* SBUS is 11 bits. FrSky receivers will output a range of 172 - 1811
        with channels set to a range of -100% to +100%. Using extended limits
        of -150% to +150% outputs a range of 0 to 2047, which is the maximum
        range achievable with 11 bits of data.
    */
    int16_t v;
    if (extended) {
        v = (int16_t)round(((float)raw_val * SBUS_EXT_MULT_ADJ) + SBUS_EXT_ADD_ADJ);
    }
    else {
        v = (int16_t)round(((float)raw_val * SBUS_NORM_MULT_ADJ) + SBUS_NORM_ADD_ADJ);
    }
    return v;
}

/**
 * @brief Debug print the incoming buffer (as bytes) and the raw values (11-bit values).
 *
 * @param buf The incoming byte buffer (25 bytes)
 */
static void _debug_print_bufraw(const volatile uint8_t* buf) {
    printf("\nINCOMING SBUS RX BUFFER and RAW values\n");
    for (int i = 0; i < 25; i++) {
        printf("%02hhX ", buf[i]);
    }
    printf("\n   ");
    for (int j = 0; j < 16; j++) {
        printf("%03hX ", _channel_state->ch_data[j].raw_v);
    }
    printf("\n\n");
}

/**
 * @brief Enable the PIO and DMA for receipt of the RC-RX
 */
static void _enable_rx() {
    ledA_on(false);
    // Set up the message and handler to use for receiving RX messages
    _rxcmn_mh_data_rdy = rxcmn_mh_rx_msg_proc;      // Message handler to process RC RX message.
    _rxcmn_data_rdy_msg = MSG_RC_RX_RAW_MSG_RDY;        // Message for RC RX message received
    _rxcmn_protocol_spec_proc = rx_sbus_protocol_processor;
    _rxcmn_en_next_rx = rxcmn_enable_next_msg;      // The 'common' processing does everything we need
    _rxcmn_proto_spec_rx_err_hndlr = NULL_MSG_HDLR;

    // Clear the message buffer CRC
    _rc_bufs.msg_bufs.crc32_last = 0u;

    // Set up the interrupt for the PIO State Machine
    irq_set_exclusive_handler(PIO_RCRX_SYSIRQ_ERR, rxcmn_irq_pio_rx_err_handler); // Set the IRQ handler
    irq_set_enabled(PIO_RCRX_SYSIRQ_ERR, false); // Disable the IRQ for now
    pio_set_irqn_source_enabled(_rxcmn_pio_smrx_pocfg.pio, PIO_RCRX_IRQ_ERR_IDX, PIO_INTR_SM0_LSB, true); // Interrupt on IRQ-Bit0 set

    //
    uint piosmpc = piosm_pc(_rxcmn_pio_smrx_pocfg);
    printf("PIO-SM-PC: %d\n", piosmpc);

    //
    // Init the PIO RD DMA to read from the PIO when there is data ready
    _rxcmn_dma_pio_rd_cfg = dma_channel_get_default_config(_rxcmn_dma_pio_rd); //Get configurations for the RC channel
    channel_config_set_transfer_data_size(&_rxcmn_dma_pio_rd_cfg, DMA_SIZE_8); //Set RC PIO channel data transfer size to 8 bits
    channel_config_set_read_increment(&_rxcmn_dma_pio_rd_cfg, false); // Read increment to false (read from PIO)
    channel_config_set_write_increment(&_rxcmn_dma_pio_rd_cfg, true); // Write increment to true (advance through buffer)
    channel_config_set_dreq(&_rxcmn_dma_pio_rd_cfg, PIO_RCRX_DREQ); //Set the transfer request signal to the PIO-SM rx-fifo not empty.
    // (bit-reverse) CRC32 sniff set-up
    channel_config_set_sniff_enable(&_rxcmn_dma_pio_rd_cfg, true);
    dma_sniffer_set_data_accumulator(CRC32_INIT);
    dma_sniffer_set_output_reverse_enabled(true);
    // Enable CRC generation of the data to check for new messages
    dma_sniffer_enable(_rxcmn_dma_pio_rd, DMA_SNIFF_CTRL_CALC_VALUE_CRC32, true);

    //
    // Configure PIO RD DMA channel to read from the RXFIFO MSB and write to the Message Enqueue buffer.
    dma_channel_configure(_rxcmn_dma_pio_rd, &_rxcmn_dma_pio_rd_cfg,
        _rc_bufs.msg_bufs.msg_enqueue,  // Destination
        (uint8_t*)&_rxcmn_pio_smrx_pocfg.pio->rxf[PIO_RC_SM_RX] + 3,  // PIO-SM RX FIFO to read from (+3 to read the MSB)
        SBUS_MSG_LEN,                   // SBUS is fixed length.
        false);                         // Don't start yet
    //
    // Tell the DMA to raise its IRQ when the channel finishes a block
    dma_irqn_set_channel_enabled(IRQn_RCRX_DMA_FROM_PIO, _rxcmn_dma_pio_rd, true);

    // Enable the system interrupts
    irq_set_enabled(SYSIRQ_RCRX_DMA_FROM_PIO, true);
    irq_set_enabled(PIO_RCRX_SYSIRQ_ERR, true);
    //
    // Get ready to receive messages
    _frames_lost = 0;
    _rcrx_lerr_tms = 0;
    _rcrx_msg_cnt = 0;
    _rcrx_msg_while_busy_cnt = 0;
    _rcrx_msg_same_data_cnt = 0;

    // To help with debugging, fill the receive buffers with known values
    if (debug_mode_enabled()) {
        memset((void*)_rc_bufs.msg_bufs.msg_enqueue, 0xFF, RC_RX_BUF_SIZE);
    }
    //
    // Restart the PIO-SM so that it is waiting for the idle period.
    piosm_reset(_rxcmn_pio_smrx_pocfg);
    //
    // Now start the DMA and PIO-SM
    dma_channel_start(_rxcmn_dma_pio_rd);
    pio_sm_set_enabled(_rxcmn_pio_smrx_pocfg.pio, PIO_RC_SM_RX, true);
    // When a full message has been received the DMA will interrupt and post a message.
    // Use a sleep to periodically print the PIO-SM-PC
    cmt_sleep_ms(3000, rxcmn_list_pio_dma_state, (void*)1);
    return;
}


static void _rx_sbus_pio_deinit(pio_sm_pocfg smpocfg) {
    pio_sm_set_enabled(smpocfg.pio, smpocfg.sm, false);
    const pio_program_t* pio_prgm = &rx_sbus_program;
    pio_remove_program(smpocfg.pio, pio_prgm, smpocfg.offset);
}

static pio_sm_pocfg _rx_sbus_pio_init(PIO pio, uint sm, uint pin, uint baud) {
    pio_sm_set_enabled(pio, sm, false);

    // disable pull-up and pull-down on gpio pin
    gpio_disable_pulls(pin);

    pio_sm_pocfg smpocfg;
    smpocfg.pio = pio;
    smpocfg.sm = sm;

    // install the program in the PIO shared instruction space
    const pio_program_t* pio_prgm = &rx_sbus_program;
    smpocfg.offset = pio_add_program(pio, pio_prgm);
    if (smpocfg.offset < 0) {
        return smpocfg;      // the program could not be added
    }
    pio_gpio_init(pio, pin);
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, false);

    smpocfg.sm_cfg = rx_sbus_program_get_default_config(smpocfg.offset);
    sm_config_set_in_pins(&smpocfg.sm_cfg, pin); // for WAIT, IN
    sm_config_set_jmp_pin(&smpocfg.sm_cfg, pin); // for JMP
    sm_config_set_in_pin_count(&smpocfg.sm_cfg, 1); // Only use 1 pin/bit for `mov x,PINS` for parity check
    // Run at 20X BAUD.
    // This is required for the PIO program to read in the middle of the bits.
    float div = (float)clock_get_hz(clk_sys) / (baud * rx_sbus_BIT_CLK_MULT);
    sm_config_set_clkdiv(&smpocfg.sm_cfg, div);

    pio_sm_init(pio, sm, smpocfg.offset, &smpocfg.sm_cfg);

    return smpocfg;
}


// ///////////////////////////////////////////////////////////////////////// //
// Public Methods                                                            //
// ///////////////////////////////////////////////////////////////////////// //

void rx_sbus_start() {
    _enable_rx();
}


// ///////////////////////////////////////////////////////////////////////// //
// Initialization & Deinitialization                                                            //
// ///////////////////////////////////////////////////////////////////////// //


void rx_sbus_module_deinit() {
    // Stop and de-initialize the PIO-SM
    _rx_sbus_pio_deinit(_rxcmn_pio_smrx_pocfg);
    // Remove the IRQ handlers
    irq_remove_handler(SYSIRQ_RCRX_DMA_FROM_PIO, rxcmn_irq_dma_from_pio);
    // Give the DMA Channels back.
    dma_channel_unclaim(_rxcmn_dma_pio_rd);
    _rxcmn_dma_pio_rd = -1;

    _initialized = false;
}

void rx_sbus_module_init(uint baud, rcrx_state_t* channel_state) {
    assert(!_initialized);
    _initialized = true;

    _channel_state = channel_state;
    rxcmn_module_init(channel_state);

    _rxcmn_mh_data_rdy = NULL_MSG_HDLR;     // No message handler to start.
    _rxcmn_data_rdy_msg = MSG_HWRT_NOOP;    // No message to start with.
    _rxcmn_protocol_spec_proc = NULL;       // No message handler to start.

    // Get a DMA channel that will take data from the PIO-SM RXFIFO,
    _rxcmn_dma_pio_rd = dma_claim_unused_channel(true);
    // Configure the processor to run DMA from PIO routine when DMA IRQ1 is
    // asserted and DMA buffer transfer routine when DMA IRQ0 is asserted.
    irq_set_exclusive_handler(SYSIRQ_RCRX_DMA_FROM_PIO, rxcmn_irq_dma_from_pio);

    _rxcmn_pio_smrx_pocfg = _rx_sbus_pio_init(PIO_RC_BLOCK, PIO_RC_SM_RX, RC_RXTEL_GPIO, baud);
    if (_rxcmn_pio_smrx_pocfg.offset < 0) {
        board_panic("RX SBUS could not load PIO-SM program!!!");
    }

    return;
}
