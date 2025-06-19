
#include "rx_cmn.h"
#include "rcrx.h"
#include "rx_srxl2.h"

#include "board.h"
#include "cmt/cmt.h"
#include "debug_support.h"

#include "stdbool.h"
#include "stdint.h"

#include <stdio.h>     // For `printf`
#include <string.h>         // For `memset`

static bool _initialized = false;


/** @brief Protocol specific RX error handler. Called at end of common handling. */
msg_handler_fn _rxcmn_proto_spec_rx_err_hndlr;
/** @brief Message handler for RX data available. */
msg_handler_fn _rxcmn_mh_data_rdy;
/** @brief   Message handler for processing protocol message */
rcrx_msg_rcvd_fn _rxcmn_protocol_spec_proc;
/** @brief The Message ID to post when RX data is ready. */
msg_id_t _rxcmn_data_rdy_msg;
/** @brief Flag indicating that an RX message is being processed. */
volatile bool _rxcmn_msg_processing;
/** @brief Function to be called to enable receiving the next RX stream. */
enrx_fn _rxcmn_en_next_rx;

// RC RX Buffers
rc_bufs_t _rc_bufs;     // Global for debugging

// RC Channel State
static rcrx_state_t* _channel_state;

// RCRX Error Tracking
uint32_t _rcrx_lerr_tms;  // Time of the last error.

// Message Counts
uint32_t _rcrx_msg_cnt;
uint32_t _rcrx_msg_while_busy_cnt;
uint32_t _rcrx_msg_same_data_cnt;

// RX PIO-SM and DMA Configurations
int _rxcmn_dma_pio_rd;                     // DMA channel used to pull data from the PIO-SM
dma_channel_config _rxcmn_dma_pio_rd_cfg;  // Keep the config so the channel is easy to re-run
pio_sm_pocfg _rxcmn_pio_smrx_pocfg;        // Configuration for the PIO RX State Machine



// ///////////////////////////////////////////////////////////////////////// //
// Internal Function Declarations                                            //
// ///////////////////////////////////////////////////////////////////////// //



// ///////////////////////////////////////////////////////////////////////// //
// Interrupt Handlers                                                        //
// ///////////////////////////////////////////////////////////////////////// //

void __isr rxcmn_irq_dma_from_pio() {
    // Disable the SM
    pio_sm_set_enabled(_rxcmn_pio_smrx_pocfg.pio, _rxcmn_pio_smrx_pocfg.sm, false);
    // Get the CRC value from the DMA incase it is being calculated/used
    uint32_t crc = dma_sniffer_get_data_accumulator();
    // Clear the interrupt request.
    dma_irqn_acknowledge_channel(IRQn_RCRX_DMA_FROM_PIO, _rxcmn_dma_pio_rd);

    // If a handler is set, post our message indicating that RC RX data is available
    if (_rxcmn_mh_data_rdy) {
        cmt_msg_t msg;
        msg.data.value32u = crc; // Include the CRC in the message
        cmt_msg_init3(&msg, _rxcmn_data_rdy_msg, MSG_PRI_NORM, _rxcmn_mh_data_rdy);
        postHWRTMsg(&msg);
    }
}


/**
 * @brief IRQ Handler for RX Error (Parity +/ Framing).
 *
 * Posts a MSG_RC_RX_RAW_ERR message with the IRQ flags and the handler set
 * to mh_rcrx_error.
 */
void __isr rxcmn_irq_pio_rx_err_handler() {
    cmt_msg_t msg;
    io_rw_32 pio_irqbits = _rxcmn_pio_smrx_pocfg.pio->irq;
    //
    // Stop the PIO-SM before clearing the IRQ
    //
    pio_sm_set_enabled(_rxcmn_pio_smrx_pocfg.pio, _rxcmn_pio_smrx_pocfg.sm, false);
    _rxcmn_pio_smrx_pocfg.pio->irq = 0xFF; // Writing '1' clears the IRQ Flag bit
    //
    // Initialize and post the message
    //
    cmt_msg_init3(&msg, MSG_RC_RX_RAW_ERR, MSG_PRI_NORM, rxcmn_mh_pio_rx_error);
    msg.data.value32u = pio_irqbits;
    postHWRTMsg(&msg);
}


// ///////////////////////////////////////////////////////////////////////// //
// Message Handlers                                                          //
// ///////////////////////////////////////////////////////////////////////// //


void rxcmn_mh_pio_rx_error(cmt_msg_t* msg) {

    // Cancel the DMA handling the received data.
    //  Due to errata RP2350-E5(see the RP2350 datasheet for further detail),
    //  it is necessary to clear the enable bit of the channel being aborted,
    //  and any chained channels, prior to the abort to prevent (re)triggering.
    //
    // disable the system and DMA channel IRQ
    irq_set_enabled(SYSIRQ_RCRX_DMA_FROM_PIO, false);
    dma_irqn_set_channel_enabled(IRQn_RCRX_DMA_FROM_PIO, _rxcmn_dma_pio_rd, false);

    io_rw_32 pio_irqbits = (io_rw_32)msg->data.value32u;
    // See where in the received message the error occurred
    io_rw_32 dma_wr_addr = dma_channel_hw_addr(_rxcmn_dma_pio_rd)->write_addr;

    rxcmn_count_pio_rx_error(pio_irqbits, dma_wr_addr);

    // Abort the channel
    dma_channel_abort(_rxcmn_dma_pio_rd);
    // Read the Abort register until 0
    // (this isn't done in the SDK, but the datasheet says it is needed)
    while (dma_hw->abort) tight_loop_contents();
    // clear any spurious IRQ (if there was one)
    dma_irqn_acknowledge_channel(IRQn_RCRX_DMA_FROM_PIO, _rxcmn_dma_pio_rd);

    // Call a protocol specific error routine, if one is set.
    if (_rxcmn_proto_spec_rx_err_hndlr != NULL_MSG_HDLR) {
        _rxcmn_proto_spec_rx_err_hndlr(msg);
    }

    // The interrupt handler for the PIO error disabled the PIO-SM, but we
    // should clear out the RXFIFO.
    pio_sm_clear_fifos(_rxcmn_pio_smrx_pocfg.pio, _rxcmn_pio_smrx_pocfg.sm);

    // Re-enable the interrupts that we disabled above.
    dma_irqn_set_channel_enabled(IRQn_RCRX_DMA_FROM_PIO, _rxcmn_dma_pio_rd, true);
    irq_set_enabled(SYSIRQ_RCRX_DMA_FROM_PIO, true);

    if (!_channel_state->local_rx_disabled) {
        // Set up for another message
        _rxcmn_en_next_rx();
    }

    // Re-post the error message so other parts of the system know about it
    cmt_msg_rm_set_hdlr(msg);
    postHWRTMsg(msg);
    postDCSMsg(msg);
}

void rxcmn_mh_rx_msg_proc(cmt_msg_t* msg) {
    _rcrx_msg_cnt++; // Count the message
    ledA_on(true);
    _channel_state->msgs_processed = _rcrx_msg_cnt;

    // See if we are still working on the previous message
    if (_rxcmn_msg_processing) {  // If the previous message is still processing, count that we were busy
        _rcrx_msg_while_busy_cnt++;
    }
    else {
        _rxcmn_msg_processing = true;
        // See if the accumulated message is different from the current one...
        uint32_t a_crc = msg->data.value32u;
        if (a_crc == _rc_bufs.msg_bufs.crc32_last) {
            _rcrx_msg_same_data_cnt++;
            _rxcmn_en_next_rx();
        }
        else {
            // The enqueued message is different from the last one.
            // Process the message
            //
            _rc_bufs.msg_bufs.crc32_last = a_crc;
            //
            if (_rxcmn_protocol_spec_proc) {
                bool failsafe_was = _channel_state->failsafe;
                uint16_t chgs = _rxcmn_protocol_spec_proc();
                //
                // If failsafe changed, post a message.
                bool failsafe_now = _channel_state->failsafe;
                if (failsafe_was != failsafe_now) {
                    cmt_msg_t mfs;
                    cmt_msg_init(&mfs, MSG_RC_FAILSAFE_CHG);
                    mfs.data.bv = failsafe_now;
                    postHWRTMsg(&mfs);
                    postDCSMsg(&mfs);
                }
                //
                // If any channels have changed, let the system know.
                if (chgs != 0) {
                    cmt_msg_t mchcng;
                    cmt_msg_init(&mchcng, MSG_RC_RECEIVED);
                    mchcng.data.value16u = chgs;
                    postBothMsgDiscardable(&mchcng);
                }
            }
        }
    }
    ledA_on(false);
}


// ///////////////////////////////////////////////////////////////////////// //
// Internal Functions                                                        //
// ///////////////////////////////////////////////////////////////////////// //




// ///////////////////////////////////////////////////////////////////////// //
// Public Methods                                                            //
// ///////////////////////////////////////////////////////////////////////// //

void rxcmn_count_pio_rx_error(io_rw_32 pio_irqbits, io_rw_32 dma_wr_addr) {
    // Get the info about the error (could be parity or framing)
    bool parity_err = (pio_irqbits & RCRX_ERROR_MASK) == RCRX_PARITY_ERR;
    // Update the error count
    rxcmn_update_error_count();
    if (parity_err) {
        _channel_state->local_parity_err_cnt++;
    }
    // check that it is within the enqueue buffer
    int indx = dma_wr_addr - (((io_rw_32)_rc_bufs.msg_bufs.msg_enqueue) + 1);
    if (parity_err) {
        indx--; // Parity error pushes the expected parity as the last byte
    }
    fflush(stdout);
    printf("\nRC RX ERROR: %04X  Byte at Buffer Index: %d  Errors: %ld  ESR: %u\n", pio_irqbits, indx, _channel_state->local_err_cnt_all, _channel_state->local_errs_in_period);
    rxcmn_list_pio_dma_state((void*)0); // Report the PIO-SM status
    if (indx <= RC_RX_BUF_SIZE) {
        printf(" Buf: ");
        for (int i = 0; i <= indx; i++) {
            printf("%02hhX ", _rc_bufs.msg_bufs.msg_enqueue[i]);
        }
        printf("\n");
        if (parity_err) {
            uint8_t pchk = _rc_bufs.msg_bufs.msg_enqueue[indx + 1];
            uint8_t pr = (pchk & 0xF0) >> 4;
            uint8_t pe = (pchk & 0x0F);
            printf(" Parity [Received:Expected]: %1hhX:%1hhX\n", pr, pe);
        }
    }
    fflush(stdout);
}

void rxcmn_list_pio_dma_state(void* data) {
    bool retrigger = (data != NULL);
    uint8_t pio_sm_pc = piosm_pc(_rxcmn_pio_smrx_pocfg);
    io_rw_32 pio_irqbits = _rxcmn_pio_smrx_pocfg.pio->irq;
    io_ro_32 pio_fstat = _rxcmn_pio_smrx_pocfg.pio->fstat;
    bool pio_sm_enbl = piosm_enabled(_rxcmn_pio_smrx_pocfg.pio, _rxcmn_pio_smrx_pocfg.sm);
    dma_channel_hw_t* dma = dma_channel_hw_addr(_rxcmn_dma_pio_rd);
    io_rw_32 dma_wr_addr = dma->write_addr;
    uint32_t dma_xfer_cnt = dma->transfer_count;
    uint32_t dma_ctrl = dma->ctrl_trig;
    fflush(stdout);
    printf("\nRC-RX PIO PC: %2hhu IRQ: %04X ST: %04X EN: %d  DMA  CTRL: %08lX ADDR: %08lX CNT: % 2hu\n  Rcvd: %lu  Dup: %lu  Errs: %lu ESR: %u\n",
        pio_sm_pc, pio_irqbits, pio_fstat, pio_sm_enbl,
        dma_ctrl, dma_wr_addr, (uint8_t)dma_xfer_cnt,
        _rcrx_msg_cnt, _rcrx_msg_same_data_cnt, _channel_state->local_err_cnt_all, _channel_state->local_errs_in_period);
    // If the protocol is SRXL2, also get the info for the MSG PIO-SM
    if (rcrx_get_protocol() == RXP_SRXL2) {
        io_rw_32 dma_xfer_cnt = dma_channel_hw_addr(_dma_pio_to_pio)->transfer_count;
        pio_sm_pc = piosm_pc(_rx_srxl2_piosm_msg_cfg);
        pio_irqbits = _rx_srxl2_piosm_msg_cfg.pio->irq;
        pio_sm_enbl = piosm_enabled(_rx_srxl2_piosm_msg_cfg.pio, _rx_srxl2_piosm_msg_cfg.sm);
        printf("RC SRXL2-MSG PIO PC: %2hhu  IRQ: %04X  EN: %d  PIO-PIO XFER CNT: %u\n", pio_sm_pc, pio_irqbits, pio_sm_enbl, dma_xfer_cnt);
    }
    printf("\n");
    fflush(stdout);

    if (retrigger) {
        // trigger another report
        cmt_sleep_ms(7000, rxcmn_list_pio_dma_state, (void*)true);
    }
}

void rxcmn_enable_next_msg() {
    _rxcmn_msg_processing = false;
    //
    // Reset the PIO-SM so that it is waiting for the idle period.
    piosm_reset(_rxcmn_pio_smrx_pocfg);

    // For debugging, fill the buffer with a known value
    if (debug_mode_enabled()) {
        memset((void*)_rc_bufs.msg_bufs.msg_enqueue, 0xFF, RC_RX_BUF_SIZE);
    }
    //
    // Re-Configure PIO RD DMA channel, don't start it yet.
    dma_channel_set_config(_rxcmn_dma_pio_rd, &_rxcmn_dma_pio_rd_cfg, false);
    //
    // (bit-reverse) CRC32 sniff set-up
    dma_sniffer_set_data_accumulator(CRC32_INIT);
    channel_config_set_sniff_enable(&_rxcmn_dma_pio_rd_cfg, true);
    dma_sniffer_set_output_reverse_enabled(true);
    // Enable CRC generation of the data to check for new messages
    dma_sniffer_enable(_rxcmn_dma_pio_rd, DMA_SNIFF_CTRL_CALC_VALUE_CRC32, true);
    //
    // Now start the DMA and PIO-SM
    dma_channel_set_write_addr(_rxcmn_dma_pio_rd, _rc_bufs.msg_bufs.msg_enqueue, true);
    pio_sm_set_enabled(_rxcmn_pio_smrx_pocfg.pio, _rxcmn_pio_smrx_pocfg.sm, true);
}

uint64_t rxcmn_get_rxmsg_cnt() {
    return _rcrx_msg_cnt;
}

void rxcmn_update_error_count() {
    uint32_t now = now_ms();
    if ((now - _rcrx_lerr_tms) > RCRX_ERROR_RESET_TIME) {
        _channel_state->local_errs_in_period = 0;   // Reset the errors in period count.
    }
    _rcrx_lerr_tms = now;
    _channel_state->local_err_cnt_all++;
    if (++_channel_state->local_errs_in_period > RCRX_ERROR_DISABLE_THRSH) {
        _channel_state->local_rx_disabled = true; // Too many errors in the period, disable.
        printf("\nTOO MANY RC-RX ERRORS - Disabling RC-RX\n");
    }
}


void rxcmn_module_init(rcrx_state_t* channel_state) {
    assert(!_initialized);
    _initialized = true;

    _channel_state = channel_state;

}