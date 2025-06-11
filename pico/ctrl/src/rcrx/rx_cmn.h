/*
    RC Receive Common - Used by both SBUS and SRXL2.

    Copyright 2025 AESilky (SilkyDESIGN)
    SPDX-License-Identifier: MIT

*/
#ifndef RX_CMN_H_
#define RX_CMN_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "rcrx_t.h"

#include "cmt/cmt_t.h"
#include "pio_sm.h"

#include "hardware/dma.h"

extern msg_handler_fn _rxcmn_proto_spec_rx_err_hndlr;
extern msg_handler_fn _rxcmn_mh_data_rdy;  // Current message handler for RX Data Available
extern msg_handler_fn _rxcmn_mh_proc_protocol_msg;  // Current message handler for process protocol message
extern msg_id_t _rxcmn_data_rdy_msg;
extern volatile bool _rxcmn_msg_processing;

/**
 * @brief Function prototype for the enable_next_rx.
 */
typedef void (*enrx_fn)(void);
extern enrx_fn _rxcmn_en_next_rx;


// Memory Buffers for receiving and maintaining channel/control data
//   We use this ordering of the RC_RX buffers to allow doing a
//   single DMA operation to copy the 'current' to the 'previous'
//   and the 'enqueue' to the 'current'.
#define RC_RX_BUF_SIZE 80
typedef struct RC_RX_MSG_BUFFERS_ {
    volatile uint8_t msg_enqueue[RC_RX_BUF_SIZE];
    volatile uint32_t crc32_last;
} rc_msg_bufs_t;

#define RC_DETECT_BUF_SIZE 60
typedef union RCRX_BUFFERS_ {
    volatile uint32_t detect_buf[RC_DETECT_BUF_SIZE];
    rc_msg_bufs_t msg_bufs;
} rc_bufs_t;
extern rc_bufs_t _rc_bufs; // Global for debugging

extern uint32_t _rcrx_lerr_tms;  // Time of the last error.

// Message Counts
extern uint32_t _rcrx_msg_cnt;
extern uint32_t _rcrx_msg_while_busy_cnt;
extern uint32_t _rcrx_msg_same_data_cnt;

// RX PIO-SM and DMA Configurations
extern int _rxcmn_dma_pio_rd;                     // DMA channel used to pull data from the PIO-SM
extern dma_channel_config _rxcmn_dma_pio_rd_cfg;  // Keep the config so the channel is easy to re-run
extern pio_sm_pocfg _rxcmn_pio_smrx_pocfg;        // Configuration for the PIO RX State Machine (the one receiving from the RX)
extern dma_channel_config _rxcmn_dma_bc_cfg;      // Keep the config so the channel is easy to re-run

// DMA CRC Seed value for checking received messages
#define CRC32_INIT      ((uint32_t)-1l)


#define RCRX_ERROR_MASK 0x0011
#define RCRX_FRAME_ERR  0x0001
#define RCRX_PARITY_ERR 0x0011
#define RCRX_ERROR_RESET_TIME (60*1000) // Millisecond time duration without error to clear count
#define RCRX_ERROR_DISABLE_THRSH 10     // Number of errors within the reset time to disable the RX


extern void __isr rxcmn_irq_dma_from_pio();
extern void __isr rxcmn_irq_pio_rx_err_handler();

/**
 * @brief Message Handler for the RX RC Error
 *
 * @param msg The message. Data is the irq-flags from the State Machine.
 */
extern void rxcmn_mh_pio_rx_error(cmt_msg_t* msg);

/**
 * @brief Continuation of RX RC message received. Run after buffers copied.
 *
 * @param msg
 */
extern void rxcmn_mh_rx_msg_proc_cnt(cmt_msg_t* msg);

/**
 * @brief Message Handler for the RX RC Message Ready
 *
 * @param msg The message. No data is contained in it.
 */
extern void rxcmn_mh_rx_msg_proc(cmt_msg_t* msg);

extern void rxcmn_count_pio_rx_error(io_rw_32 pio_irqbits, io_rw_32 dma_wr_addr);

extern uint64_t rxcmn_get_rxmsg_cnt();

extern void rxcmn_list_pio_dma_state(void* data);

extern void rxcmn_enable_next_msg();

extern void rxcmn_update_error_count();

extern void rxcmn_module_init(rcrx_state_t* channel_state);

#ifdef __cplusplus
}
#endif
#endif // RX_SBUS_H_
