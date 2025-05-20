/*
    Remote Control Receive.

    This module reads manual remote control signals from a Spektrum or FrSKY
    radio control receiver. The Spektrum receiver is connected using SRXL2.
    The FrSKY receiver is connected using SBUS (non-inverted).

    See the header in rcrx.h for details


    Copyright 2025 AESilky (SilkyDESIGN)
    SPDX-License-Identifier: MIT

*/

#include "rcrx.h"
#include "rx_sbus.h"
#include "serial_rd.h"

#include "board.h"
#include "cmt/cmt.h"
#include "cmt/cmt_t.h"
#include "system_defs.h"

#include "pico/stdlib.h"
#include "hardware/dma.h" //DMA is used to move data from the PIO to a buffer
#include "hardware/irq.h"

#include <stdio.h>
#include <string.h>

static bool _initialized = false;
msg_handler_fn _mh_rx_data_rdy;  // Current message handler for RX Data Available
msg_handler_fn _mh_buf_copy_done;  // Current message handler for buffer copy done
msg_id_t _rx_data_rdy_msg;


// Values used while detecting the BAUD and PROTOCOL
#define BAUD_PROTOCOL_CHECKS_CNT 3
static int _bp_check_indx;
static const uint _baud_checks[] = {400000, 115200, 100000};
static const rxprotocol_t _rx_proto_types[] = {RXP_SRXL2, RXP_SRXL2, RXP_SBUS};
static const bool _uart_inverse[] = {false, false, true};
static const char* _rxtype_names[] = { "Unknown", "SBUS", "SRXL2" };

// DMA and PIO instance data
static int _dma_pio_rd;         // DMA channel used to pull data from the PIO-SM
static int _dma_buf_copy;       // DMA channel used to copy the RX buffers
static dma_channel_config _dma_pio_rd_cfg; // Keep the config so the channel is easy to re-run
static dma_channel_config _dma_bc_cfg;     // Keep the config so the channel is easy to re-run
static pio_sm_cfg _pio_sm_cfg;  // The offset and config for the PIO-SM

// RC Receiver Baud and Protocol
static uint _baud;
static rxprotocol_t _rx_protocol;

// Memory Buffers for receiving and maintaining channel/control data
//   We use this ordering of the RC_RX buffers to allow doing a
//   single DMA operation to copy the 'current' to the 'previous'
//   and the 'enqueue' to the 'current'.
#define RC_RX_BUF_SIZE 80
typedef struct RC_RX_MSG_BUFFERS_ {
    volatile uint8_t msg_enqueue[RC_RX_BUF_SIZE];
    volatile uint8_t msg_cur[RC_RX_BUF_SIZE];
    volatile uint8_t msg_pre[RC_RX_BUF_SIZE];
    volatile uint32_t crc32_cur;
    volatile uint32_t crc32_pre;
} rc_msg_bufs_t;
#define RC_DETECT_BUF_SIZE 60
typedef union {
    volatile uint32_t detect_buf[RC_DETECT_BUF_SIZE];
    rc_msg_bufs_t msg_bufs;
} rc_bufs_t;
rc_bufs_t _rc_bufs; // Global for debugging
// DMA CRC Seed value for checking received messages
#define CRC32_INIT      ((uint32_t)-1l)

// RCRX Error Tracking
#define RCRX_ERROR_MASK 0x0011
#define RCRX_FRAME_ERR  0x0001
#define RCRX_PARITY_ERR 0x0011
#define RCRX_ERROR_RESET_TIME (60*1000) // Millisecond time duration without error to clear count
#define RCRX_ERROR_DISABLE_THRSH 10     // Number of errors within the reset time to disable the RX
long _rcrx_errs;        // Total number of RX errors since being reset. Global for debugging
int _rcrx_perrs;       // Number of Parity errors (part of total).
bool _rcrx_disabled;   // Excessive errors disabled RC RX.
int _rcrx_err_sr;      // Errors since the last error count reset.
int _rcrx_erst_cnt;    // Count of error count reset.
uint32_t _rcrx_lerr_t; // Time of the last error.

// Message Counts
static long _rcrx_msg_cnt;
static long _rcrx_msg_while_busy_cnt;
static long _rcrx_msg_same_data_cnt;


// ///////////////////////////////////////////////////////////////////////// //
// Internal Function Declarations                                            //
// ///////////////////////////////////////////////////////////////////////// //

static bool _chk_sngl_0n1_bits(volatile const uint32_t* buf, int samples, int* zeros, int* ones);
static void _enable_rx();
static void _get_baud_chk_sample();
static uint8_t _get_pio_sm_pc();
static void _la_print_buf(volatile const uint32_t* buf, int samples);
static void _rx_msg_pio_sm_pc(void* data);
static void _rx_next_msg();
void mh_rcrx_error(cmt_msg_t* msg);
void mh_rcrx_msg_proc(cmt_msg_t* msg);


// ///////////////////////////////////////////////////////////////////////// //
// Interrupt Handlers                                                        //
// ///////////////////////////////////////////////////////////////////////// //

void __isr irq_dma_from_pio() {
    // Disable the SM
    pio_sm_set_enabled(PIO_RC_BLOCK, PIO_RC_SM, false);
    // Get the CRC value from the DMA incase it is being calculated/used
    uint32_t crc = dma_sniffer_get_data_accumulator();
    // Clear the interrupt request.
    dma_irqn_acknowledge_channel(IRQn_RCRX_DMA_FROM_PIO, _dma_pio_rd);

    // If a handler is set, post our message indicating that RC RX data is available
    if (_mh_rx_data_rdy) {
        cmt_msg_t msg;
        msg.data.value32u = crc; // Include the CRC in the message
        cmt_msg_init3(&msg, _rx_data_rdy_msg, MSG_PRI_NORM, _mh_rx_data_rdy);
        postHWCtrlMsg(&msg);
    }
}

void __isr irq_dma_buf_xfer() {
    // Clear the interrupt request.
    dma_irqn_acknowledge_channel(IRQn_RCRX_DMA_XFER, _dma_buf_copy);

    // If a handler is set, post our message indicating that the copy is complete
    if (_mh_buf_copy_done) {
        cmt_msg_t msg;
        cmt_msg_init3(&msg, MSG_RC_COPY_DONE, MSG_PRI_NORM, _mh_buf_copy_done);
        postHWCtrlMsg(&msg);
    }
}

/**
 * @brief IRQ Handler for RX Error (Parity +/ Framing).
 *
 * Posts a MSG_RC_RX_ERR message with the IRQ flags and the handler set
 * to mh_rcrx_error. It stops the State Machine and clears the interrupt.
 */
void __isr irq_pio_rx_handler() {
    cmt_msg_t msg;
    io_rw_32 pio_irqbits = PIO_RC_BLOCK->irq;
    //
    // Stop the PIO-SM before clearing the IRQ
    //
    // The message handler will clear the IRQ
    //
    pio_sm_set_enabled(PIO_RC_BLOCK, PIO_RC_SM, false);
    cmt_msg_init3(&msg, MSG_RC_RX_ERR, MSG_PRI_NORM, mh_rcrx_error);
    msg.data.value32u = pio_irqbits;
    postHWCtrlMsg(&msg);
}

// ///////////////////////////////////////////////////////////////////////// //
// Message Handlers                                                          //
// ///////////////////////////////////////////////////////////////////////// //

/**
 *  @brief This is the message handler that is called when a buffer of data is
 *  available to test.
 *
 * @param msg The message. No data is contained in it.
*/
void mh_detect_baud_protocol(cmt_msg_t* msg) {
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
            pio_serial_rd_deinit(PIO_RC_BLOCK, PIO_RC_SM, _pio_sm_cfg.offset, _uart_inverse[_bp_check_indx]);
            //
            // Post a message indicating that the RC RX has been detected
            cmt_msg_t msg;
            cmt_msg_init(&msg, MSG_RC_DETECTED);
            msg.data.rcrx_bp.baud = _baud;
            msg.data.rcrx_bp.protocol = _rx_protocol;
            postHWCtrlMsg(&msg);
            postDCSMsg(&msg);

            _enable_rx();   // Enable everything to receive RC RX data
            return;
        }
    }

    // Didn't find single 0 and 1 bits at that rate.
    // Try the next one
    pio_serial_rd_deinit(PIO_RC_BLOCK, PIO_RC_SM, _pio_sm_cfg.offset, _uart_inverse[_bp_check_indx]);
    _bp_check_indx++;
    if (_bp_check_indx == BAUD_PROTOCOL_CHECKS_CNT) {
        printf("RC-RX all BAUD rates checked. Starting over.\n");
        _bp_check_indx = 0;
    }
    _get_baud_chk_sample();
}

/**
 * @brief Message Handler for the RX RC Error
 *
 * @param msg The message. Data is the irq-flags from the State Machine.
 */
void mh_rcrx_error(cmt_msg_t* msg) {
    // Cancel the DMA handling the received data.
    //  Due to errata RP2350-E5(see the RP2350 datasheet for further detail),
    //  it is necessary to clear the enable bit of the channel being aborted,
    //  and any chained channels, prior to the abort to prevent (re)triggering.
    //
    // disable the channel IRQ
    dma_irqn_set_channel_enabled(IRQn_RCRX_DMA_FROM_PIO, _dma_pio_rd, false);

    // Get the info about the error
    io_rw_32 pio_irqbits = (io_rw_32)msg->data.value32u;
    _rcrx_errs++;
    if ((pio_irqbits & RCRX_ERROR_MASK) == RCRX_PARITY_ERR) {
        _rcrx_perrs++;
    }
    // Update the other error info
    uint32_t now = now_ms();
    if ((now - _rcrx_lerr_t) > RCRX_ERROR_RESET_TIME) {
        _rcrx_err_sr = 0;   // Reset the errors since reset count.
        _rcrx_erst_cnt++;   // Track how many times we reset
    }
    if (++_rcrx_err_sr > RCRX_ERROR_DISABLE_THRSH) {
        _rcrx_disabled = true; // Too many errors in the period, disable.
        printf("\nTOO MANY RC-RX ERRORS - Disabling RC-RX\n");
    }

    // See where in the received message the error occurred
    io_rw_32 dma_wr_addr = dma_channel_hw_addr(_dma_pio_rd)->write_addr;
    // check that it is within the enqueue buffer
    int indx = ((void*)dma_wr_addr - (void*)_rc_bufs.msg_bufs.msg_enqueue);
    printf("\nRC RX Error: %04X  Buffer Index: %d  Errors: %ld\n  ESR: %d", pio_irqbits, indx - 1, _rcrx_errs, _rcrx_err_sr);
    _rx_msg_pio_sm_pc(false); // Report the PIO-SM status
    if (indx <= RC_RX_BUF_SIZE) {
        printf(" Buf: ");
        for (int i=0; i<indx; i++) {
            printf("%02hhX ", _rc_bufs.msg_bufs.msg_enqueue[i]);
        }
        printf("\n");
    }

    // abort the channel
    dma_channel_abort(_dma_pio_rd);
    // clear any spurious IRQ (if there was one)
    dma_irqn_acknowledge_channel(IRQn_RCRX_DMA_FROM_PIO, _dma_pio_rd);

    // Re-post the error message so other parts of the system know about it
    cmt_msg_rm_hdlr(msg);
    postHWCtrlMsg(msg);
    postDCSMsg(msg);

    if (!_rcrx_disabled) {
        // Set up for another message
        PIO_RC_BLOCK->irq = 0x11; // Writing '1' clears the IRQ Flag bit
        _rx_next_msg();
    }
}

/**
 * @brief Continuation of RX RC message received. Run after buffers copied.
 *
 * @param msg
 */
void mh_rcrx_msg_proc_cnt(cmt_msg_t* msg) {
    // Disable these DMA IRQs
    dma_irqn_set_channel_enabled(IRQn_RCRX_DMA_XFER, _dma_buf_copy, false);
    irq_set_enabled(SYSIRQ_RCRX_DMA_XFER, false);

    // Show that we are processing
    ledB_on(true);
    long same_data_cnt = _rcrx_msg_same_data_cnt;
    _rcrx_msg_same_data_cnt = 0;

    // ZZZ - For now, just print the buffer, knowing that it is SBUS 25 bytes
    // ZZZ - Every 500 received
    static int msg_to_process_cnt = 0;
    if (msg_to_process_cnt++ % 500 == 0) {
        volatile uint8_t* buf = _rc_bufs.msg_bufs.msg_cur;
        printf("\nRC Msg Rx: Total: %ld  Same: %ld  Busy: %ld  Processed: %ld  Errs: %ld ESR: %d :\n  ", _rcrx_msg_cnt, same_data_cnt, _rcrx_msg_while_busy_cnt, msg_to_process_cnt, _rcrx_errs, _rcrx_err_sr);
        for (int i = 0; i < 25; i++) {
            printf("%02hhX ", *(buf++));
        }
        printf("\n");
    }

    _rx_next_msg();
    _mh_buf_copy_done = NULL_MSG_HDLR;
    ledB_on(false);
}

/**
 * @brief Message Handler for the RX RC Message Ready
 *
 * @param msg The message. No data is contained in it.
 */
void mh_rcrx_msg_proc(cmt_msg_t* msg) {
    _rcrx_msg_cnt++; // Count the message
    ledA_on(true);

    // See if we are still working on the previous message
    if (_mh_buf_copy_done) {  // If this still has a handler set, we haven't finished the prev msg
        _rcrx_msg_while_busy_cnt++;
    }
    else {
        _mh_rx_data_rdy = NULL_MSG_HDLR;    // Clear the message handler.
        _rx_data_rdy_msg = MSG_HWOS_NOOP;   // No message.

        // See if the accumulated message is different from the current one...
        uint32_t a_crc = msg->data.value32u;
        if (a_crc == _rc_bufs.msg_bufs.crc32_cur) {
            _rcrx_msg_same_data_cnt++;
            _rx_next_msg();
        }
        else {
            // The enqueued message is different from the current one.
            // Copy the buffers and set up to process it once the copy is complete
            //
            _rc_bufs.msg_bufs.crc32_pre = _rc_bufs.msg_bufs.crc32_cur;
            _rc_bufs.msg_bufs.crc32_cur = a_crc;
            // Set up the Copy DMA channel and our message handler
            _mh_buf_copy_done = mh_rcrx_msg_proc_cnt;
            //
            dma_channel_configure(_dma_buf_copy, &_dma_bc_cfg,
                (volatile void*)&_rc_bufs.msg_bufs.msg_pre[RC_RX_BUF_SIZE - 1],  // Destination
                (volatile void*)&_rc_bufs.msg_bufs.msg_cur[RC_RX_BUF_SIZE - 1],  // Source
                RC_RX_BUF_SIZE * 2,     // Copy 'current' to 'previous' and 'enqueue' to 'current'
                false);                 // Don't start yet
            //
            // Tell the DMA to raise IRQ line 1 when the channel finishes a block
            dma_irqn_acknowledge_channel(IRQn_RCRX_DMA_XFER, _dma_buf_copy);  // clear any spurious IRQ (if there was one)
            dma_irqn_set_channel_enabled(IRQn_RCRX_DMA_XFER, _dma_buf_copy, true);
            // Enable interrupts
            irq_set_enabled(SYSIRQ_RCRX_DMA_XFER, true);
            dma_channel_start(_dma_buf_copy);
        }
    }
    ledA_on(false);
}

// ///////////////////////////////////////////////////////////////////////// //
// Internal Functions                                                        //
// ///////////////////////////////////////////////////////////////////////// //

static uint8_t _get_pio_sm_pc() {
    uint8_t ppc = pio_sm_get_pc(PIO_RC_BLOCK, PIO_RC_SM);
    uint8_t pc = ppc - _pio_sm_cfg.offset;

    return pc;
}

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
 * @brief Enable the PIO and DMA for receipt of the RC-RX at the detected
 * baud and protocol.
 */
static void _enable_rx() {
    printf("Enabling RC-RX @%d for Protocol:%d (%s)\n", _baud, _rx_protocol, get_rxtype_name(_rx_protocol));
    // ZZZ - For now, use RX-SBUS. Will need to select based on protocol

    // Set up the message and handler to use for receiving RX messages
    _mh_rx_data_rdy = mh_rcrx_msg_proc;     // Message handler to process RC RX message.
    _rx_data_rdy_msg = MSG_RC_RX_MSG_RDY;   // Message for RC RX message received
    _mh_buf_copy_done = NULL_MSG_HDLR;      // NULL the BUFFER COPY message handler so messages won't be posted

    // Clear the Current and Previous message buffer CRCs
    _rc_bufs.msg_bufs.crc32_cur = 0u;
    _rc_bufs.msg_bufs.crc32_pre = 0u;

    _pio_sm_cfg = pio_rx_sbus_init(PIO_RC_BLOCK, PIO_RC_SM, RC_RXTEL_GPIO, _baud);
    hard_assert(_pio_sm_cfg.offset >= 0);
    // Set up the interrupt for the PIO State Machine
    irq_set_exclusive_handler(PIO_RCRX_IRQ_ERR, irq_pio_rx_handler); // Set the IRQ handler
    irq_set_enabled(PIO_RCRX_IRQ_ERR, false); // Disable the IRQ for now
    pio_set_irqn_source_enabled(PIO_RC_BLOCK, PIO_RCRX_IRQ_IDX, PIO_INTR_SM0_LSB, true); // Interrupt on IRQ-Bit0 set

    //
    uint piosmpc = _get_pio_sm_pc();
    printf("PIO-SM-PC: %d\n", piosmpc);

    //
    // Init the PIO RD DMA to read from the PIO when there is data ready
    _dma_pio_rd_cfg = dma_channel_get_default_config(_dma_pio_rd); //Get configurations for the RC channel
    channel_config_set_transfer_data_size(&_dma_pio_rd_cfg, DMA_SIZE_8); //Set RC PIO channel data transfer size to 8 bits
    channel_config_set_read_increment(&_dma_pio_rd_cfg, false); // Read increment to false (read from PIO)
    channel_config_set_write_increment(&_dma_pio_rd_cfg, true); // Write increment to true (advance through buffer)
    channel_config_set_dreq(&_dma_pio_rd_cfg, PIO_RCRX_DREQ); //Set the transfer request signal to the PIO-SM rx-fifo not empty.
    // (bit-reverse) CRC32 sniff set-up
    channel_config_set_sniff_enable(&_dma_pio_rd_cfg, true);
    dma_sniffer_set_data_accumulator(CRC32_INIT);
    dma_sniffer_set_output_reverse_enabled(true);
    // Enable CRC generation of the data to check for new messages
    dma_sniffer_enable(_dma_pio_rd, DMA_SNIFF_CTRL_CALC_VALUE_CRC32, true);

    //
    // Configure PIO RD DMA channel to read from the MSB and write to the Message Current buffer.
    dma_channel_configure(_dma_pio_rd, &_dma_pio_rd_cfg,
        _rc_bufs.msg_bufs.msg_enqueue,  // Destination
        (uint8_t *)&PIO_RC_BLOCK->rxf[PIO_RC_SM] + 3,  // PIO-SM RX FIFO to read from (+3 to read the MSB)
        25,                             // ZZZ - This is SBUS. SRXL2 requires more work
        false);                         // Don't start yet
    //
    // Tell the DMA to raise its IRQ when the channel finishes a block
    dma_irqn_set_channel_enabled(IRQn_RCRX_DMA_FROM_PIO, _dma_pio_rd, true);

    //
    // Init the Buffer Copy DMA to move 'current' to 'previous' and 'enqueue' to 'current'
    //    This is done by copying from the end of the buffers to the begining, using the fact that
    //      the buffers are arranged in order: 'enqueue' - 'current' - 'previous'
    //      So, by copying from the end toward the beginning, and copying from 'current' to 'previous' for 2x
    //      the buffer size, all of the buffers will be copied in a single DMA operation.
    _dma_bc_cfg = dma_channel_get_default_config(_dma_buf_copy); //Get configurations for the Buf Copy
    channel_config_set_transfer_data_size(&_dma_bc_cfg, DMA_SIZE_8); //Set size to 8 bits
    channel_config_set_read_increment(&_dma_bc_cfg, true); // Read decrement (move backwards through buffer)
    _dma_bc_cfg.ctrl |= DMA_CH0_CTRL_TRIG_INCR_READ_REV_BITS; // SDK doesn't provide a function to set this
    channel_config_set_write_increment(&_dma_bc_cfg, true); // Write decrement to true (move backwards through buffer)
    _dma_bc_cfg.ctrl |= DMA_CH0_CTRL_TRIG_INCR_WRITE_REV_BITS; // SDK doesn't provide a function to set this
    channel_config_set_dreq(&_dma_bc_cfg, DREQ_FORCE); // Transfer as fast as possible
    // Enable the interrupts
    irq_set_enabled(SYSIRQ_RCRX_DMA_FROM_PIO, true);
    irq_set_enabled(PIO_RCRX_IRQ_ERR, true);
    //
    // Get ready to receive messages
    _rcrx_errs = 0;
    _rcrx_perrs = 0;
    _rcrx_erst_cnt = 0;
    _rcrx_err_sr = 0;
    _rcrx_lerr_t = 0;
    _rcrx_disabled = false;
    _rcrx_msg_cnt = 0;
    _rcrx_msg_while_busy_cnt = 0;
    _rcrx_msg_same_data_cnt = 0;
    // ZZZ - Temp, clear the receive buffer
    memset((void*)_rc_bufs.msg_bufs.msg_enqueue, 0xFF, RC_RX_BUF_SIZE);
    memset((void*)_rc_bufs.msg_bufs.msg_cur, 0xAA, RC_RX_BUF_SIZE);
    memset((void*)_rc_bufs.msg_bufs.msg_pre, 0x55, RC_RX_BUF_SIZE);
    //
    // Restart the PIO-SM so that it is waiting for the idle period.
    sm_reset(PIO_RC_BLOCK, PIO_RC_SM, _pio_sm_cfg);
    //
    // Now start the DMA and PIO-SM
    dma_channel_start(_dma_pio_rd);
    pio_sm_set_enabled(PIO_RC_BLOCK, PIO_RC_SM, true);
    // When a full message has been received the DMA will interrupt and post a message.
    // Use a sleep to periodically print the PIO-SM-PC
    cmt_sleep_ms(3000, _rx_msg_pio_sm_pc, (void*)true);
    return;
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
    pio_sm_set_enabled(PIO_RC_BLOCK, PIO_RC_SM, false);
    //
    // Init/Re-init the PIO-SM clk to the correct rate for the BAUD check.
    // (The PIO-SM should already be initialized correctly, except for possibly the BAUD.)
    uint baud = _baud_checks[_bp_check_indx];
    _pio_sm_cfg = pio_serial_rd_init(PIO_RC_BLOCK, PIO_RC_SM, RC_RXTEL_GPIO, baud, _uart_inverse[_bp_check_indx]);
    hard_assert(_pio_sm_cfg.offset >= 0);
    //
    uint piosmpc = _get_pio_sm_pc();
    printf("PIO-SM-PC: %d\n", piosmpc);
    //
    // Init the PIO RD DMA to read from the PIO when there is data ready
    dma_channel_config c1 = dma_channel_get_default_config(_dma_pio_rd); //Get configurations for the RC channel
    channel_config_set_transfer_data_size(&c1, DMA_SIZE_32); //Set RC PIO channel data transfer size to 32 bits
    channel_config_set_read_increment(&c1, false); // Read increment to false (read from PIO)
    channel_config_set_write_increment(&c1, true); // Write increment to true (advance through buffer)
    channel_config_set_dreq(&c1, PIO_RCRX_DREQ); //Set the transfer request signal to the PIO-SM rx-fifo not empty.
    //
    // Configure RC PIO channel to trigger DMA when data is available.
    dma_channel_configure(_dma_pio_rd, &c1,
        &_rc_bufs.detect_buf,                       // Destination
        &PIO_RC_BLOCK->rxf[PIO_RC_SM],              // PIO-SM RX FIFO to read from
        RC_DETECT_BUF_SIZE,                         // Number of samples to transfer (one block)
        false);                                     // Don't start yet
    //
    // Tell the DMA to raise IRQ line 1 when the channel finishes a block
    dma_irqn_set_channel_enabled(IRQn_RCRX_DMA_FROM_PIO, _dma_pio_rd, true);
    irq_set_enabled(SYSIRQ_RCRX_DMA_FROM_PIO, true);
    //
    // Now start the DMA and PIO-SM
    dma_channel_start(_dma_pio_rd);
    pio_sm_set_enabled(PIO_RC_BLOCK, PIO_RC_SM, true);
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
    //
    // Try 400,000 BAUD first (SRXL2 high)
    _bp_check_indx = 0;
    //
    // Set up the message and handler to use for checking the BAUD and Protocol
    _mh_rx_data_rdy = mh_detect_baud_protocol; // Message handler to try to detect baud.
    _rx_data_rdy_msg = MSG_RC_DETECT_DA; // Message for detecting
    _get_baud_chk_sample();
}

static void _rx_msg_pio_sm_pc(void* data) {
    bool retrigger = (data != NULL);
    uint8_t pio_sm = _get_pio_sm_pc();
    io_rw_32 pio_irqbits = PIO_RC_BLOCK->irq;
    bool pio_sm_enbl = sm_enabled(PIO_RC_BLOCK, PIO_RC_SM);
    printf("RC RX Msg PIO PC: %2hhu  IRQ: %04X  ENABLED: %d    MsgRcvd: %ld  Repeated: %ld  Errs: %ld ESR: %d\n", pio_sm, pio_irqbits, pio_sm_enbl, _rcrx_msg_cnt, _rcrx_msg_same_data_cnt, _rcrx_errs, _rcrx_err_sr);
    if (retrigger) {
        // trigger another report
        cmt_sleep_ms(30000, _rx_msg_pio_sm_pc, (void*)true);
    }
}

static void _rx_next_msg() {
    //
    // Reset the PIO-SM so that it is waiting for the idle period.
    sm_reset(PIO_RC_BLOCK, PIO_RC_SM, _pio_sm_cfg);
    //
    // Set up the message and handler to use for receiving RX messages
    _mh_rx_data_rdy = mh_rcrx_msg_proc;     // Message handler to process RC RX message.
    _rx_data_rdy_msg = MSG_RC_RX_MSG_RDY;   // Message for RC RX message received
    _mh_buf_copy_done = NULL_MSG_HDLR;      // NULL the BUFFER COPY message handler so messages won't be posted

    memset((void*)_rc_bufs.msg_bufs.msg_enqueue, 0xFF, RC_RX_BUF_SIZE);
    //
    // (bit-reverse) CRC32 sniff set-up
    channel_config_set_sniff_enable(&_dma_pio_rd_cfg, false);
    dma_sniffer_set_data_accumulator(CRC32_INIT);
    channel_config_set_sniff_enable(&_dma_pio_rd_cfg, true);
    dma_sniffer_set_output_reverse_enabled(true);
    // Enable CRC generation of the data to check for new messages
    dma_sniffer_enable(_dma_pio_rd, DMA_SNIFF_CTRL_CALC_VALUE_CRC32, true);
    //
    // Now start the DMA and PIO-SM
    dma_channel_set_write_addr(_dma_pio_rd, _rc_bufs.msg_bufs.msg_enqueue, true);
    pio_sm_set_enabled(PIO_RC_BLOCK, PIO_RC_SM, true);
}

// ///////////////////////////////////////////////////////////////////////// //
// Public Methods                                                            //
// ///////////////////////////////////////////////////////////////////////// //

const char* get_rxtype_name(rxprotocol_t type) {
    return _rxtype_names[type];
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
    _mh_rx_data_rdy = NULL_MSG_HDLR;    // No message handler to start.
    _rx_data_rdy_msg = MSG_HWOS_NOOP;   // No message to start with.
    _mh_buf_copy_done = NULL_MSG_HDLR;  // No message handler to start.

    // Get a DMA channel that will move data from one RC RX buffer to another
    _dma_buf_copy = dma_claim_unused_channel(true);
    // Get a DMA channel that will take data from the PIO-SM RXFIFO,
    _dma_pio_rd = dma_claim_unused_channel(true);
    // Configure the processor to run irq_dma_from_pio() when DMA IRQ1 is
    // asserted and irq_dma_buf_xfer() when DMA IRQ0 is asserted.
    irq_set_exclusive_handler(SYSIRQ_RCRX_DMA_FROM_PIO, irq_dma_from_pio);
    irq_set_exclusive_handler(SYSIRQ_RCRX_DMA_XFER, irq_dma_buf_xfer);
}

