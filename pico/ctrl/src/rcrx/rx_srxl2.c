/*
    Receive SRXL2 data using two PIO-SMs and two DMAs.

    Copyright 2025 AESilky (SilkyDESIGN)
    SPDX-License-Identifier: MIT

    Some portions, primarily functions beginning with '_srxl or SRXL_',
    Copyright (c) 2019-2023 Horizon Hobby, LLC
    SPDX-License-Identifier: MIT
*/
#include "rx_srxl2.h"
#include "generated/rx_srxl2_msg.pio.h"
#include "generated/rx_srxl2_uart.pio.h"

#include "rcrx_t.h"
#include "rx_cmn.h"
#include "srxl2_t.h"

#include "board.h"
#include "cmt/cmt.h"
#include "debug_support.h"
#include "system_defs.h"

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"

#include <stdio.h>
#include <string.h>

/**
 * @brief Special value that configures the DMA for endless transfer.
 *
 * @see RP2350 Datasheet, Section 12.6.2.2.1. Count Modes
 */
#define R2350_DMA_ENDLESS_XFERCOUNT 0xF0000001

#define SRXL2_MSG_PID_IDX   0
#define SRXL2_MSG_TYPE_IDX  1
#define SRXL2_MSG_LEN_IDX   2

#define SRXL2_PID           0xA6

#define SRXL_CRC_OPTIMIZE_MODE SRXL_CRC_OPTIMIZE_SIZE
//#define SRXL_CRC_OPTIMIZE_MODE SRXL_CRC_OPTIMIZE_SPEED
#if (RCRX_CHANNELS_SUPPORTED < 32)
    #define CHANNELS_TO_PROCESS RCRX_CHANNELS_SUPPORTED
#else
    #define CHANNELS_TO_PROCESS 32
#endif

static bool _initialized = false;

int _dma_pio_to_pio;
dma_channel_config _dma_pio_to_pio_cfg;  // Keep the config so the channel is easy to re-run
pio_sm_pocfg _rx_srxl2_piosm_msg_cfg;

static rcrx_state_t* _channel_state;
static uint16_t _frames_lost;

#if (SRXL_CRC_OPTIMIZE_MODE == SRXL_CRC_OPTIMIZE_SPEED)
const uint16_t srxlCRCTable[] =
{
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,

    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,

    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,

    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};
#endif

// ///////////////////////////////////////////////////////////////////////// //
// Function Declarations                                                     //
// ///////////////////////////////////////////////////////////////////////// //

static uint16_t _chval_raw_convert(uint16_t raw_val);

void rx_srxl2_mh_rx_msg_rcvd(cmt_msg_t* msg);

static void _rx_srxl2_enable_next_msg();

static uint16_t _srxlCrc16(uint8_t* packet);

static bool _srxlParsePacket(uint8_t* packet, uint8_t length);

// ///////////////////////////////////////////////////////////////////////// //
// Interrupt Handlers                                                        //
// ///////////////////////////////////////////////////////////////////////// //

/**
 * @brief IRQ Handler for RX message complete (from the MSG PIO-SM).
 *
 * Posts a MSG_RC_RX_MSG_RCVD message to initiate processing of the SRXL2 message.
 */
void __isr rx_srxl2_irq_pio_msgcmplt_handler() {
    cmt_msg_t msg;
    io_rw_32 pio_irqbits = _rx_srxl2_piosm_msg_cfg.pio->irq;
    //
    // Stop the PIO-SM before clearing the IRQ
    //
    pio_sm_set_enabled(_rx_srxl2_piosm_msg_cfg.pio, _rx_srxl2_piosm_msg_cfg.sm, false);
    _rx_srxl2_piosm_msg_cfg.pio->irq = 0x02; // Writing '1' clears the IRQ Flag bit
    //
    // Initialize and post the message
    //
    cmt_msg_init3(&msg, MSG_RC_RX_MSG_RCVD, MSG_PRI_NORM, rx_srxl2_mh_rx_msg_rcvd);
    msg.data.value32u = pio_irqbits;
    postHWCtrlMsg(&msg);
}

// ///////////////////////////////////////////////////////////////////////// //
// Message Handlers                                                          //
// ///////////////////////////////////////////////////////////////////////// //

/**
 * @brief Handler for RX errors, specific to this protocol handler.
 *
 * This is called by the common RX error handler to clean things up after it
 * has cleaned up the common stuff.
 *
 * @param msg The message posted by the RX error IRQ handler.
 */
void rx_srxl2_mh_rx_err(cmt_msg_t* msg) {
    // The common handler cancels and cleans up the serial input PIO-SM and
    // the PIO-SM to buffer DMA.
}

void rx_srxl2_mh_rx_msg_proc(cmt_msg_t* msg) {
    // Show that we are processing
    ledB_on(true);
    //long same_data_cnt = _rcrx_msg_same_data_cnt;
    _rcrx_msg_same_data_cnt = 0;

    // Parse the received data into the _channel_state
    uint8_t* db = (uint8_t*)&_rc_bufs.msg_bufs.msg_enqueue;
    uint8_t len = db[SRXL2_MSG_LEN_IDX];
    _srxlParsePacket(db, len);

    ledB_on(false);
    _rxcmn_en_next_rx();
}



void rx_srxl2_mh_rx_msg_rcvd(cmt_msg_t* msg) {
    // Cancel the DMA handling the received data.
    //  Due to errata RP2350-E5(see the RP2350 datasheet for further detail),
    //  it is necessary to clear the enable bit of the channel being aborted,
    //  and any chained channels, prior to the abort to prevent (re)triggering.
    //

    // We need to abort the channel
    dma_channel_abort(_rxcmn_dma_pio_rd);
    // clear any spurious IRQ (if there was one)
    dma_irqn_acknowledge_channel(IRQn_RCRX_DMA_FROM_PIO, _rxcmn_dma_pio_rd);
    // Get the CRC value from the DMA for the dup/diff message indication
    uint32_t crc = dma_sniffer_get_data_accumulator();

    // Now we can do the regular Enqueue to Current buffer transfer and processing.
    msg->data.value32u = crc;
    rxcmn_mh_rx_msg_proc(msg);
}



// ///////////////////////////////////////////////////////////////////////// //
// Internal Functions                                                        //
// ///////////////////////////////////////////////////////////////////////// //

/**
 * @brief Convert a raw channel value (SRXL value) to our system representation.
 *
 * Convert to the system representation (see the README in this module)
 *
 * @param raw_val The raw SRXL channel value.
 * @return uint16_t The system value
 */
static uint16_t _chval_raw_convert(uint16_t rval) {
    return rval;
}

/**
 * @brief Enable the PIO and DMA for receipt of the RC-RX
 */
static void _enable_rx() {
    ledA_on(false);
    // Set up the message and handler to use for receiving RX messages
    _rxcmn_mh_data_rdy = rxcmn_mh_rx_msg_proc;      // Message handler to process RC RX message.
    _rxcmn_data_rdy_msg = MSG_RC_RX_MSG_RDY;        // Message for RC RX message received
    _rxcmn_mh_proc_protocol_msg = rx_srxl2_mh_rx_msg_proc;
    _rxcmn_en_next_rx = _rx_srxl2_enable_next_msg;   // Set up both PIO-SMs and interrupts
    _rxcmn_proto_spec_rx_err_hndlr = NULL_MSG_HDLR;

    // Clear the Current and Previous message buffer CRCs
    _rc_bufs.msg_bufs.crc32_last = 0u;

    // Set up the interrupt for the PIO State Machines
    irq_set_exclusive_handler(PIO_RCRX_SYSIRQ_MSG, rx_srxl2_irq_pio_msgcmplt_handler);
    irq_set_enabled(PIO_RCRX_SYSIRQ_MSG, false);
    pio_set_irqn_source_enabled(_rx_srxl2_piosm_msg_cfg.pio, PIO_RCRX_IRQ_MSG_IDX, PIO_INTR_SM1_LSB, true);

    irq_set_exclusive_handler(PIO_RCRX_SYSIRQ_ERR, rxcmn_irq_pio_rx_err_handler); // Set the IRQ handler
    irq_set_enabled(PIO_RCRX_SYSIRQ_ERR, false); // Disable the IRQ for now
    pio_set_irqn_source_enabled(_rxcmn_pio_smrx_pocfg.pio, PIO_RCRX_IRQ_ERR_IDX, PIO_INTR_SM0_LSB, true); // Interrupt on IRQ-Bit0 set

    //
    uint piosmpc = piosm_pc(_rxcmn_pio_smrx_pocfg);
    printf("PIO-RX-SM  PC: %d\n", piosmpc);
    piosmpc = piosm_pc(_rx_srxl2_piosm_msg_cfg);
    printf("PIO-MSG-SM PC: %d\n", piosmpc);

    //
    // Init the PIO RD DMA to read from the Message PIO-SM when there is data ready
    _rxcmn_dma_pio_rd_cfg = dma_channel_get_default_config(_rxcmn_dma_pio_rd); //Get configurations for the RC channel
    channel_config_set_transfer_data_size(&_rxcmn_dma_pio_rd_cfg, DMA_SIZE_8); //Set RC PIO channel data transfer size to 8 bits
    channel_config_set_read_increment(&_rxcmn_dma_pio_rd_cfg, false); // Read increment to false (read from PIO)
    channel_config_set_write_increment(&_rxcmn_dma_pio_rd_cfg, true); // Write increment to true (advance through buffer)
    channel_config_set_dreq(&_rxcmn_dma_pio_rd_cfg, PIO_RCRX_DREQ); //Set the transfer request signal to the MSG PIO-SM rx-fifo not empty.
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
        SRXL2_MAX_MSG_LEN + 1,          // SRXL2 maximum message length +1 (to assure the DMA doesn't end on its own)
                                        //  Message PIO pgm will interrupt us when done
                                        //  (this keeps processing the same even on max-len msg)
        false);                         // Don't start yet

    //
    // Init the PIO to PIO XFER (Message Checker) DMA to read from the RX PIO-SM and write to the MSG PIO-SM when there is data ready
    _dma_pio_to_pio_cfg = dma_channel_get_default_config(_dma_pio_to_pio); //Get configurations for the RC channel
    channel_config_set_transfer_data_size(&_dma_pio_to_pio_cfg, DMA_SIZE_8); //Set RC PIO channel data transfer size to 8 bits
    channel_config_set_read_increment(&_dma_pio_to_pio_cfg, false); // Read increment to false (read from PIO)
    channel_config_set_write_increment(&_dma_pio_to_pio_cfg, false); // Write increment to false (write to PIO)
    channel_config_set_dreq(&_dma_pio_to_pio_cfg, PIO_RC_SRXL2_SI_DREQ); //Set the transfer request signal to the SI PIO-SM rx-fifo not empty.
    //
    // Configure PIO to PIO DMA channel to read from the RXFIFO of the RX PIO-SM and write to the TXFIFO of the MSG PIO-SM.
    dma_channel_configure(_dma_pio_to_pio, &_dma_pio_to_pio_cfg,
        (uint8_t*)&_rxcmn_pio_smrx_pocfg.pio->txf[PIO_RC_SM_RX],     // PIO-SM MSG TXFIFO to write to
        (uint8_t*)&_rxcmn_pio_smrx_pocfg.pio->rxf[PIO_RC_SM_SRXL2_SI] + 3, // PIO-SM RX RXFIFO to read from (+3 to read the MSB)
        R2350_DMA_ENDLESS_XFERCOUNT,    // Endless transfer (RP2350 only)
        false);                         // Don't start yet


    // Enable the interrupts
    irq_set_enabled(PIO_RCRX_SYSIRQ_MSG, true);
    irq_set_enabled(PIO_RCRX_SYSIRQ_ERR, true);  // IRQ from PIO-SM indicating an error (framing)

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
    // Restart the MSG PIO-SM so that it is waiting for the beginning of a message
    piosm_reset(_rx_srxl2_piosm_msg_cfg);
    // Restart the PIO-SM so that it is waiting for the RX idle period.
    piosm_reset(_rxcmn_pio_smrx_pocfg);
    //
    // Now start the DMAs and PIO-SMs
    dma_channel_start(_rxcmn_dma_pio_rd);
    dma_channel_start(_dma_pio_to_pio);
    pio_sm_set_enabled(_rx_srxl2_piosm_msg_cfg.pio, _rx_srxl2_piosm_msg_cfg.sm, true);
    pio_sm_set_enabled(_rxcmn_pio_smrx_pocfg.pio, _rxcmn_pio_smrx_pocfg.sm, true);
    // When a full message has been received the MSG PIO will interrupt and post a message.
    // Use a sleep to periodically print the PIO-SM-PC
    cmt_sleep_ms(3000, rxcmn_list_pio_ch_state, (void*)true);
    return;
}

static void _rx_srxl2_enable_next_msg() {
    //
    // Reset the PIO-SM's so they are waiting for the idle period and beginning of message.
    pio_sm_set_enabled(_rxcmn_pio_smrx_pocfg.pio, _rxcmn_pio_smrx_pocfg.sm, false);
    pio_sm_set_enabled(_rx_srxl2_piosm_msg_cfg.pio, _rx_srxl2_piosm_msg_cfg.sm, false);
    piosm_reset(_rxcmn_pio_smrx_pocfg);
    piosm_reset(_rx_srxl2_piosm_msg_cfg);

    // For debugging, fill the buffer with a known value
    if (debug_mode_enabled()) {
        memset((void*)_rc_bufs.msg_bufs.msg_enqueue, 0xFF, RC_RX_BUF_SIZE);
    }
    //
    // (bit-reverse) CRC32 sniff set-up
    channel_config_set_sniff_enable(&_rxcmn_dma_pio_rd_cfg, false);
    dma_sniffer_set_data_accumulator(CRC32_INIT);
    channel_config_set_sniff_enable(&_rxcmn_dma_pio_rd_cfg, true);
    dma_sniffer_set_output_reverse_enabled(true);
    // Enable CRC generation of the data to check for new messages
    dma_sniffer_enable(_rxcmn_dma_pio_rd, DMA_SNIFF_CTRL_CALC_VALUE_CRC32, true);
    //
    // Now start the DMA and PIO-SM
    _rxcmn_msg_processing = false;
    dma_channel_set_write_addr(_rxcmn_dma_pio_rd, _rc_bufs.msg_bufs.msg_enqueue, true);
    pio_sm_set_enabled(_rx_srxl2_piosm_msg_cfg.pio, _rx_srxl2_piosm_msg_cfg.sm, true);
    pio_sm_set_enabled(_rxcmn_pio_smrx_pocfg.pio, _rxcmn_pio_smrx_pocfg.sm, true);
}

static void _rx_srxl2_pio_msg_deinit(pio_sm_pocfg sm_pocfg) {
    pio_sm_set_enabled(sm_pocfg.pio, sm_pocfg.sm, false);
    irq_remove_handler(PIO_RCRX_SYSIRQ_MSG, rx_srxl2_irq_pio_msgcmplt_handler);
    const pio_program_t* pio_prgm = &rx_srxl2_msg_program;
    pio_remove_program(sm_pocfg.pio, pio_prgm, sm_pocfg.offset);
}

static pio_sm_pocfg _rx_srxl2_pio_msg_init(PIO pio, uint sm) {
    pio_sm_set_enabled(pio, sm, false);

    pio_sm_pocfg sm_pocfg;
    sm_pocfg.pio = pio;
    sm_pocfg.sm = sm;
    // install the program in the PIO shared instruction space
    const pio_program_t* pio_prgm = &rx_srxl2_msg_program;
    sm_pocfg.offset = pio_add_program(pio, pio_prgm);
    if (sm_pocfg.offset < 0) {
        return sm_pocfg;      // the program could not be added
    }

    sm_pocfg.sm_cfg = rx_srxl2_msg_program_get_default_config(sm_pocfg.offset);
    // The MSG program doesn't use any pins, it gets its data from the TXFIFO
    // and sends it to the RXFIFO, stalling until data is available (in TXFIFO),
    // and runs as fast as it can (so there isn't a clock-div to set).
    pio_sm_init(pio, sm, sm_pocfg.offset, &sm_pocfg.sm_cfg);

    return sm_pocfg;
}


static void _rx_srxl2_pio_uart_deinit(pio_sm_pocfg sm_pocfg) {
    pio_sm_set_enabled(sm_pocfg.pio, sm_pocfg.sm, false);
    irq_remove_handler(PIO_RCRX_SYSIRQ_ERR, rxcmn_irq_pio_rx_err_handler);
    const pio_program_t* pio_prgm = &rx_srxl2_uart_program;
    pio_remove_program(sm_pocfg.pio, pio_prgm, sm_pocfg.offset);
}

static pio_sm_pocfg _rx_srxl2_pio_uart_init(PIO pio, uint sm, uint pin, uint baud) {
    pio_sm_set_enabled(pio, sm, false);

    // disable pull-up and pull-down on gpio pin
    gpio_disable_pulls(pin);

    pio_sm_pocfg sm_pocfg;
    sm_pocfg.pio = pio;
    sm_pocfg.sm = sm;
    // install the program in the PIO shared instruction space
    const pio_program_t* pio_prgm = &rx_srxl2_uart_program;
    sm_pocfg.offset = pio_add_program(pio, pio_prgm);
    if (sm_pocfg.offset < 0) {
        return sm_pocfg;      // the program could not be added
    }
    pio_gpio_init(pio, pin);
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, false);

    sm_pocfg.sm_cfg = rx_srxl2_uart_program_get_default_config(sm_pocfg.offset);
    sm_config_set_in_pins(&sm_pocfg.sm_cfg, pin); // for WAIT, IN
    sm_config_set_jmp_pin(&sm_pocfg.sm_cfg, pin); // for JMP
    // Run at 20X BAUD. This helps the PIO program read in the middle of the bits.
    float div = (float)clock_get_hz(clk_sys) / (baud * rx_srxl2_uart_BIT_CLK_MULT);
    sm_config_set_clkdiv(&sm_pocfg.sm_cfg, div);

    pio_sm_init(pio, sm, sm_pocfg.offset, &sm_pocfg.sm_cfg);

    return sm_pocfg;
}

// Compute SRXL CRC over packet buffer (assumes length is correctly set)
static uint16_t _srxlCrc16(uint8_t* packet) {
    uint16_t crc = 0;                // Seed with 0
    uint8_t length = packet[2] - 2;  // Exclude 2 CRC bytes at end of packet from the length

    if (length <= SRXL_MAX_BUFFER_SIZE - 2) {
#if (SRXL_CRC_OPTIMIZE_MODE == SRXL_CRC_OPTIMIZE_SIZE)
        // Bitwise calculation method
        for (uint8_t i = 0; i < length; ++i) {
            crc = crc ^ ((uint16_t)packet[i] << 8);
            for (int b = 0; b < 8; b++) {
                if (crc & 0x8000)
                    crc = (crc << 1) ^ 0x1021;
                else
                    crc = crc << 1;
            }
        }
#else
        // Table-lookup method
        for (uint8_t i = 0; i < length; ++i) {
            // Get indexed position in lookup table using XOR of current CRC hi byte
            uint8_t pos = (uint8_t)((crc >> 8) ^ packet[i]);
            // Shift LSB up and XOR with the resulting lookup table entry
            crc = (uint16_t)((crc << 8) ^ (uint16_t)(srxlCRCTable[pos]));
        }
#endif
    }
    return crc;
}

/**
    @brief  Parse an SRXL received packet

    AES:
        Modified to remove 'Master', 'busIndex', and a number of other 'ifdef...', as this
            module is receive/listen only (it never talks on the bus)
        Modified to set values on our `_channel_state` data.

    @param  packet:     Pointer to received packet data
    @param  length:     Length in bytes of received packet data
    @return bool:       True if a valid packet was received, else false
*/
static bool _srxlParsePacket(uint8_t* packet, uint8_t length) {
    // Validate parameters
    if (!packet || length < 5 || length > SRXL_MAX_BUFFER_SIZE)
        return false;

    // Validate SRXL ID and length
    if (packet[0] != SPEKTRUM_SRXL_ID || packet[2] != length)
        return false;

    // Validate CRC
    uint16_t calc_crc = _srxlCrc16(packet);
    uint16_t rx_crc = (((uint16_t)packet[length - 2] << 8) | packet[length - 1]);
    if (rx_crc != calc_crc) {
        // return false;
    }

    // Cast packet into our union'ed type
    SrxlPacket* pRx = (SrxlPacket*)packet;

    // Parse the specific data
    switch (pRx->header.packetType) {
    case SRXL_CTRL_ID:  // 0xCD (Channel Data)
    {
        SrxlControlData* pCtrlData = &(pRx->control.payload);

        // Validate command
        if (pCtrlData->cmd > SRXL_CTRL_CMD_FWDPGM) {
            // break;
        }

        // VTX
        if (pCtrlData->cmd == SRXL_CTRL_CMD_VTX) {
            // break;
        }
        // Channel Data or Failsafe Data
        else if (true || pCtrlData->cmd == SRXL_CTRL_CMD_CHANNEL || pCtrlData->cmd == SRXL_CTRL_CMD_CHANNEL_FS) {
            bool isFailsafe = (pCtrlData->cmd == SRXL_CTRL_CMD_CHANNEL_FS);
            _channel_state->rssi = pCtrlData->channelData.rssi;
            _channel_state->frames_lost = pCtrlData->channelData.frameLosses;

            // Save Channel values
            uint8_t channelIndex = 0;
            uint32_t channelMaskBit = 1;
            for (uint8_t i = 0; i < CHANNELS_TO_PROCESS && channelMaskBit <= pCtrlData->channelData.mask; ++i, channelMaskBit <<= 1) {
                if (pCtrlData->channelData.mask & channelMaskBit) {
                    uint16_t rval = pCtrlData->channelData.values[channelIndex++];
                    _channel_state->ch_data[i].raw_v = rval;
                    _channel_state->ch_data[i].v = _chval_raw_convert(rval);
                }
            }
            _channel_state->failsafe = isFailsafe;
        }
        break;
    }
    case SRXL_HANDSHAKE_ID:  // 0x21
    {
        if (length < sizeof(SrxlHandshakePacket))
            return false;

        SrxlHandshakeData* pHandshake = &(pRx->handshake.payload);

        // If this is an unprompted handshake (dest == 0)
        if (pHandshake->destDevID == 0) {
        }

        break;
    }
    case SRXL_PARAM_ID:  // 0x50
    {
        // TODO: Add later
        break;
    }
    case SRXL_RSSI_ID:  // 0x55
    {
        // TODO: Add later
        break;
    }
    case SRXL_BIND_ID:  // 0x41
    {
        if (length < sizeof(SrxlBindPacket))
            return false;

        SrxlBindPacket* pBindInfo = &(pRx->bind);

        // If this is a bound data report
        if (pBindInfo->request == SRXL_BIND_REQ_BOUND_DATA) {
        }
        // If this bind packet is directed at us
        else if (pBindInfo->deviceID == 0xFF) {
            // Check for Enter Bind Mode (only valid if sent to a specific receiver)
            if (pBindInfo->request == SRXL_BIND_REQ_ENTER) {
            }
            else if (pBindInfo->request == SRXL_BIND_REQ_STATUS) {
                // Bind info is filled on startup or bind, so just flag to send
            }
            // Handle set bind info request
            else if (pBindInfo->request == SRXL_BIND_REQ_SET_BIND) {
            }
        }

        break;
    }
    case SRXL_TELEM_ID:  // 0x80
    {
        if (length < sizeof(SrxlTelemetryPacket))
            return false;

        // NOTE: This data should be sent by exactly one telemetry device in response to a bus master request,
        //       so it is safe to update the global pTelemRcvr here even though this is a bus-specific function.

        SrxlTelemetryPacket* pTelem = &(pRx->telemetry);
        // If the telemetry destination is set to broadcast, that indicates a request to re-handshake
        if (pTelem->destDevID == 0xFF) {
            // If the master only found one device, don't poll again -- just tell the requesting device who we are
        }

        break;
    }
    default:
        break;
    }
    return true;
}


// ///////////////////////////////////////////////////////////////////////// //
// Public Methods                                                            //
// ///////////////////////////////////////////////////////////////////////// //

void rx_srxl2_start() {
    _enable_rx();
}



// ///////////////////////////////////////////////////////////////////////// //
// Initialization & Deinitialization                                                            //
// ///////////////////////////////////////////////////////////////////////// //


void rx_srxl2_module_deinit() {
    // Stop and de-initialize the PIO-SMs
    _rx_srxl2_pio_uart_deinit(_rxcmn_pio_smrx_pocfg);
    _rx_srxl2_pio_msg_deinit(_rxcmn_pio_smrx_pocfg);
    // Remove the IRQ handlers
    irq_remove_handler(SYSIRQ_RCRX_DMA_FROM_PIO, rxcmn_irq_dma_from_pio);
    // Give the DMA Channels back.
    dma_channel_unclaim(_rxcmn_dma_pio_rd);
    _rxcmn_dma_pio_rd = -1;
    dma_channel_unclaim(_dma_pio_to_pio);
    _dma_pio_to_pio = -1;
    _rxcmn_proto_spec_rx_err_hndlr = NULL_MSG_HDLR;

    _initialized = false;
}

void rx_srxl2_module_init(uint baud, rcrx_state_t* channel_state) {
    assert(!_initialized);
    _initialized = true;

    _channel_state = channel_state;
    rxcmn_module_init(channel_state);

    _rxcmn_mh_data_rdy = NULL_MSG_HDLR;    // No message handler to start.
    _rxcmn_data_rdy_msg = MSG_HWRT_NOOP;   // No message to start with.
    _rxcmn_mh_proc_protocol_msg = NULL_MSG_HDLR;  // No message handler to start.

    // Get a DMA channel that will take data from the PIO-SM RXFIFO,
    _rxcmn_dma_pio_rd = dma_claim_unused_channel(true);
    // Get a DMA channel that will pull from PIO-SM RX RXFIFO and push it to PIO-SM MSG TXFIFO,
    _dma_pio_to_pio = dma_claim_unused_channel(true);
    // Configure the processor to run DMA from PIO routine when DMA IRQ1 is
    // asserted and DMA buffer transfer routine when DMA IRQ0 is asserted.
    irq_set_exclusive_handler(SYSIRQ_RCRX_DMA_FROM_PIO, rxcmn_irq_dma_from_pio);

    _rxcmn_pio_smrx_pocfg = _rx_srxl2_pio_uart_init(PIO_RC_BLOCK, PIO_RC_SM_SRXL2_SI, RC_RXTEL_GPIO, baud);
    if (_rxcmn_pio_smrx_pocfg.offset < 0) {
        board_panic("RX SRXL2 could not load PIO-SM UART program!!!");
    }
    _rx_srxl2_piosm_msg_cfg = _rx_srxl2_pio_msg_init(PIO_RC_BLOCK, PIO_RC_SM_RX);
    if (_rx_srxl2_piosm_msg_cfg.offset < 0) {
        board_panic("RX SRXL2 could not load PIO-SM MSG program!!!");
    }

    return;
}
