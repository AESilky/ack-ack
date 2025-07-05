/*
    Remote Control Receive.

    This module reads manual remote control signals from a Spektrum or FrSKY
    radio control receiver. The Spektrum receiver is connected using SRXL2.
    The FrSKY receiver is connected using SBUS.

    See the header in rcrx.h for details


    Copyright 2025 AESilky (SilkyDESIGN)
    SPDX-License-Identifier: MIT

*/

#include "rcrx.h"
#include "rx_cmn.h"
#include "rx_sbus.h"
#include "rx_srxl2.h"
#include "serial_rd.h"

#include "board.h"
#include "cmt/cmt.h"
#include "cmt/cmt_t.h"
#include "system_defs.h"
#include "termx/termx_min.h"

#include "pico/stdlib.h"
#include "hardware/dma.h" //DMA is used to move data from the PIO to a buffer
#include "hardware/irq.h"

#include <stdio.h>
#include <string.h>

static bool _initialized = false;


// Values used while detecting the BAUD and PROTOCOL
#define BAUD_PROTOCOL_CHECKS_CNT 3
static int _bp_check_indx;
static const uint _baud_checks[] = {400000, 115200, 100000};
static const rxprotocol_t _rx_proto_types[] = {RXP_SRXL2, RXP_SRXL2, RXP_SBUS};
static const bool _uart_inverse[] = {false, false, true};
static const char* const _rxtype_names[] = { "Unknown", "SBUS", "SRXL2" };

// RC Receiver Baud and Protocol
static uint _baud;
static rxprotocol_t _rx_protocol;

// RC RX Channel State
static rcrx_state_t _channel_state;

// ///////////////////////////////////////////////////////////////////////// //
// Internal Function Declarations                                            //
// ///////////////////////////////////////////////////////////////////////// //

static bool _chk_sngl_0n1_bits(volatile const uint32_t* buf, int samples, int* zeros, int* ones);
static void _get_baud_chk_sample();
static void _la_print_buf(volatile const uint32_t* buf, int samples);


// ///////////////////////////////////////////////////////////////////////// //
// Interrupt Handlers                                                        //
// ///////////////////////////////////////////////////////////////////////// //

// ///////////////////////////////////////////////////////////////////////// //
// Message Handlers                                                          //
// ///////////////////////////////////////////////////////////////////////// //

/**
 *  @brief This is the message handler that is called when a buffer of data is
 *  available to test.
 *
 * @param msg The message. No data is contained in it.
*/
void rcrx_mh_detect_baud_protocol(cmt_msg_t* msg) {
    // TEMP - Print the samples
    _la_print_buf(_rc_bufs.detect_buf, RC_DETECT_BUF_SIZE);

    // See if we have a single low bit (start) and a single high bit (stop)
    // Note: This isn't valid generically, but is for our situation, since
    //       we are looking for data from either SRXL2 or SBUS.
    //       Generically, you could have all 0x0F data, which
    //       would be LLLLLHHHHH LLLLLHHHHH LLLLLHHHHH ... (Start-0F-Stop ...)
    //       and would make the baudrate appear to be much slower.
    int zeros;  // This will be set with the number of consecutive zero bits in the sample
    int ones;   // This will be set with the number of consecutive one bits in the sample
    if (_chk_sngl_0n1_bits(_rc_bufs.detect_buf, RC_DETECT_BUF_SIZE, &zeros, &ones)) {
        // We have a single 0 and single 1 bit sample. This could be right.
        // Check the number of consecutive zeros and ones...
        //  SRXL2 will have many more ones than zeros
        //  SBUS will have many more zeros than ones
        int chk = _uart_inverse[_bp_check_indx] ? zeros : ones;
        if (chk > (zeros + ones) / 2) {
            _baud = _baud_checks[_bp_check_indx];
            _rx_protocol = _rx_proto_types[_bp_check_indx];
            //
            // De-init the PIO-SM so that it is ready to accept the RX program.
            pio_serial_rd_deinit(PIO_RC_BLOCK, PIO_RC_SM_RX, _rxcmn_pio_smrx_pocfg.offset, _uart_inverse[_bp_check_indx]);
            //
            // Give the DMA Channel back so it can be used by the appropriate RX input module.
            dma_channel_unclaim(_rxcmn_dma_pio_rd);
            _rxcmn_dma_pio_rd = -1;
            //
            // Post a message indicating that the RC RX has been detected
            cmt_msg_t msg;
            cmt_msg_init(&msg, MSG_RC_DETECTED);
            msg.data.rcrx_bp.baud = _baud;
            msg.data.rcrx_bp.protocol = _rx_protocol;
            postHWRTMsg(&msg);
            postDCSMsg(&msg);

            printf("Enabling RC-RX @%d for Protocol:%d (%s)\n", _baud, _rx_protocol, rcrx_get_type_name(_rx_protocol));

            switch (_rx_protocol) {
                case RXP_SBUS:
                    rx_sbus_module_init(_baud, &_channel_state);   // Enable everything to receive RC RX data
                    rx_sbus_start();
                    break;
                case RXP_SRXL2:
                    rx_srxl2_module_init(_baud, &_channel_state);
                    rx_srxl2_start();
                    break;
                default:
                    board_panic("RCRX Tried to enable 'default' protocol case!");
                    break;
            }
            return;
        }
    }

    // Didn't find single 0 and 1 bits at that rate.
    // Try the next one
    pio_serial_rd_deinit(PIO_RC_BLOCK, PIO_RC_SM_RX, _rxcmn_pio_smrx_pocfg.offset, _uart_inverse[_bp_check_indx]);
    _bp_check_indx++;
    if (_bp_check_indx == BAUD_PROTOCOL_CHECKS_CNT) {
        printf("RC-RX all BAUD rates checked. Starting over.\n");
        _bp_check_indx = 0;
    }
    _get_baud_chk_sample();
}

// ///////////////////////////////////////////////////////////////////////// //
// Internal Functions                                                        //
// ///////////////////////////////////////////////////////////////////////// //

static bool _chk_sngl_0n1_bits(volatile const uint32_t* buf, int samples, int* zeros, int* ones) {
    bool sngl_0bit = false;
    bool sngl_1bit = false;
    int cons_0bit_cnt = 0;
    int max_0bit_cnt = 0;
    int cons_1bit_cnt = 0;
    int max_1bit_cnt = 0;
    for (int i = 0; i < samples; i++) {
        uint32_t d = buf[i];
        uint32_t bitmask = 1u;
        for (int bit = 0; bit < 32; bit++) {
            uint8_t b = (d & bitmask) ? 1 : 0;  // Set b to 0 or 1 based on the bit.
            if (b == 0) {
                cons_0bit_cnt++;
                if (cons_0bit_cnt > max_0bit_cnt) {
                    max_0bit_cnt = cons_0bit_cnt;
                }
                if (cons_1bit_cnt == 1) {
                    sngl_1bit = true;
                }
                cons_1bit_cnt = 0;
            }
            else { // bit is 1
                cons_1bit_cnt++;
                if (cons_1bit_cnt > max_1bit_cnt) {
                    max_1bit_cnt = cons_1bit_cnt;
                }
                if (cons_0bit_cnt == 1) {
                    sngl_0bit = true;
                }
                cons_0bit_cnt = 0;
            }
            bitmask <<= 1;
        }
    }
    if (zeros) {
        *zeros = max_0bit_cnt;
    }
    if (ones) {
        *ones = max_1bit_cnt;
    }
    // At this point, sngl_0bit and sngl_1bit are set based on the collected data
    printf("Single 0 bit: %d  Single 1 bit: %d  Max 0's: %d  Max 1's: %d\n", sngl_0bit, sngl_1bit, max_0bit_cnt, max_1bit_cnt);
    return (sngl_0bit && sngl_1bit);
}

/**
 * @brief Set up to get a sample of RC RX data to test for BAUD and Protocol.
 *
 * This sets up the PIO baud rate and the DMA. A message is posted by the DMA upon
 * completion and that handler will check the data. If it doesn't match, then
 * it will update the index and call into this method again for the next
 * test.
 */
static void _get_baud_chk_sample() {
    //
    // Stop the PIO-SM
    pio_sm_set_enabled(PIO_RC_BLOCK, PIO_RC_SM_RX, false);
    //
    // Init/Re-init the PIO-SM clk to the correct rate for the BAUD check.
    // (The PIO-SM should already be initialized correctly, except for possibly the BAUD.)
    uint baud = _baud_checks[_bp_check_indx];
    _rxcmn_pio_smrx_pocfg = pio_serial_rd_init(PIO_RC_BLOCK, PIO_RC_SM_RX, RC_RXTEL_GPIO, baud, _uart_inverse[_bp_check_indx]);
    //
    uint piosmpc = piosm_pc(_rxcmn_pio_smrx_pocfg);
    printf("PIO-SM-PC: %d\n", piosmpc);
    //
    // Init the PIO RD DMA to read from the PIO when there is data ready
    dma_channel_config c1 = dma_channel_get_default_config(_rxcmn_dma_pio_rd); //Get configurations for the RC channel
    channel_config_set_transfer_data_size(&c1, DMA_SIZE_32); //Set RC PIO channel data transfer size to 32 bits
    channel_config_set_read_increment(&c1, false); // Read increment to false (read from PIO)
    channel_config_set_write_increment(&c1, true); // Write increment to true (advance through buffer)
    channel_config_set_dreq(&c1, PIO_RCRX_DREQ); //Set the transfer request signal to the PIO-SM rx-fifo not empty.
    //
    // Configure RC PIO channel to trigger DMA when data is available.
    dma_channel_configure(_rxcmn_dma_pio_rd, &c1,
        &_rc_bufs.detect_buf,                       // Destination
        &PIO_RC_BLOCK->rxf[PIO_RC_SM_RX],              // PIO-SM RX FIFO to read from
        RC_DETECT_BUF_SIZE,                         // Number of samples to transfer (one block)
        false);                                     // Don't start yet
    //
    // Tell the DMA to raise IRQ line 1 when the channel finishes a block
    dma_irqn_set_channel_enabled(IRQn_RCRX_DMA_FROM_PIO, _rxcmn_dma_pio_rd, true);
    irq_set_enabled(SYSIRQ_RCRX_DMA_FROM_PIO, true);
    //
    // Now start the DMA and PIO-SM
    dma_channel_start(_rxcmn_dma_pio_rd);
    pio_sm_set_enabled(PIO_RC_BLOCK, PIO_RC_SM_RX, true);
    // When 20 samples have been read the DMA will interrupt and post a message.
}

/**
 * @brief Print a buffer full of samples in a 'logic-analyzer' style
 *  (using '-_' to represent high and low bit values).
 *
 * @param buf Pointer to a (read-only) buffer of uint32_t values
 * @param samples The number of values in the buffer
 */
static void _la_print_buf(volatile const uint32_t* buf, int samples) {
    for (int i = 0; i < samples; i++) {
        uint32_t d = buf[i];
        printf("%08.8X: ", d);  // Print the value (hex)
        // Now print the waveform
        uint32_t bitmask = 1u;
        for (int bit = 0; bit < 32; bit++) {
            printf(d & bitmask ? "-" : "_");
            bitmask <<= 1;
        }
        printf("\n");
    }
}

static void _get_baud_protocol() {
    // Initialize the PIO-SM for detecting the BAUD and Protocol
    // and the DMA for pulling the data into the memory buffer.
    //
    _rxcmn_dma_pio_rd = dma_claim_unused_channel(true);
    // Configure the processor to run irq_dma_from_pio() when DMA IRQ1 is
    // asserted.
    irq_set_exclusive_handler(SYSIRQ_RCRX_DMA_FROM_PIO, rxcmn_irq_dma_from_pio);

    // Try 400,000 BAUD first (SRXL2 high)
    _bp_check_indx = 0;
    //
    // Set up the message and handler to use for checking the BAUD and Protocol
    _rxcmn_mh_data_rdy = rcrx_mh_detect_baud_protocol; // Message handler to try to detect baud.
    _rxcmn_data_rdy_msg = MSG_RC_DETECT_DA; // Message for detecting
    _get_baud_chk_sample();
}

// ///////////////////////////////////////////////////////////////////////// //
// Public Methods                                                            //
// ///////////////////////////////////////////////////////////////////////// //

void rcrx_clear_ch_changed() {
    _channel_state.changed = 0x00;
}

void rcrx_clear_ch_state() {
    for (int i = 0; i < RCRX_CHANNELS_SUPPORTED; i++) {
        rcrx_ch_data_t* cd = &_channel_state.ch_data[i];
        cd->raw_v = 0;
        cd->v = INT16_MAX;
    }
    _channel_state.changed = 0x00;
    _channel_state.failsafe = false;
    _channel_state.frames_lost = 0;
    _channel_state.rssi = 0;
    _channel_state.local_errs_in_period = 0;
    _channel_state.local_err_cnt_all = 0;
    _channel_state.local_parity_err_cnt = 0;
    _channel_state.local_rx_disabled = false;
    _channel_state.msgs_processed = 0;
}


const rcrx_state_t* rcrx_get_ch_state() {
    return (&_channel_state);
}


rxprotocol_t rcrx_get_protocol() {
    return _rx_protocol;
}

uint64_t rcrx_get_rx_cnt() {
    return rxcmn_get_rxmsg_cnt();
}

const char* rcrx_get_type_name(rxprotocol_t type) {
    return _rxtype_names[type];
}

void rcrx_print_ch_state(bool hl_chg) {
    //
    // Channel Values
    rcrx_ch_data_t* cd = _channel_state.ch_data;
    uint16_t chg = _channel_state.changed;
    printf("   CH1    CH2    CH3    CH4    CH5    CH6    CH7    CH8\n");
    if (hl_chg) {
        for (int i=0; i<8; i++) {
            char* hl = "";
            char* nm = "";
            if (chg & (1 << i)) {
                hl = TERMX_START_RED_STR;
                nm = TERMX_DEFAULT_COLOR_STR;
            }
            printf("%s%6hd%s ", hl, cd[i].v, nm);
        }
        printf("\n");
    }
    else {
        printf("%6hd %6hd %6hd %6hd %6hd %6hd %6hd %6hd\n",
            cd[0].v, cd[1].v, cd[2].v, cd[3].v, cd[4].v, cd[5].v, cd[6].v, cd[7].v);
    }
    printf("   CH9   CH10   CH11   CH12   CH13   CH14   CH15   CH16\n");
    if (hl_chg) {
        for (int i = 8; i < 16; i++) {
            char* hl = "";
            char* nm = "";
            if (chg & (1 << i)) {
                hl = TERMX_START_RED_STR;
                nm = TERMX_DEFAULT_COLOR_STR;
            }
            printf("%s%6hd%s ", hl, cd[i].v, nm);
        }
        printf("\n");
    }
    else {
        printf("%6hd %6hd %6hd %6hd %6hd %6hd %6hd %6hd\n",
            cd[8].v, cd[9].v, cd[10].v, cd[11].v, cd[12].v, cd[13].v, cd[14].v, cd[15].v);
    }
    printf("  CHG  FS    LF    Errs ESR Msgs-Processed\n");
    printf(" %04hX  %2d %5lu %7lu  %2lu %14lu\n",
        chg, _channel_state.failsafe, _channel_state.frames_lost,
        _channel_state.local_err_cnt_all, _channel_state.local_errs_in_period, _channel_state.msgs_processed);
    fflush(stdout);
    _channel_state.changed = 0x0000;
}

void rcrx_start() {
    _get_baud_protocol();
    return;
}


// ///////////////////////////////////////////////////////////////////////// //
// Initialization                                                            //
// ///////////////////////////////////////////////////////////////////////// //


void rcrx_module_init() {
    assert(!_initialized);
    _initialized = true;

    _baud = 0;
    _rx_protocol = RXP_UNKNOWN;
    _rxcmn_mh_data_rdy = NULL_MSG_HDLR;    // No message handler to start.
    _rxcmn_data_rdy_msg = MSG_HWRT_NOOP;   // No message to start with.
}

