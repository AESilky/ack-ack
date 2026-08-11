/*
    Receive SRXL2 data using two PIO-SMs and two DMAs.

    Copyright 2025 AESilky (SilkyDESIGN)
    SPDX-License-Identifier: MIT

*/
#ifndef RX_SRXL2_H_
#define RX_SRXL2_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "rcrx_t.h"

#include "pio_sm.h"

#include "stdint.h"
#include "hardware/pio.h"

/**
 * @brief DMA index for the RX PIO-SM to the MSG PIO-SM transfer.
 *
 * Exposed for status and debugging.
 */
extern int _dma_pio_to_pio;

/**
 * @brief PIO-SM configuration for SRXL2 Message detection (used for debugging/status)
 */
extern pio_sm_pocfg _rx_srxl2_piosm_msg_cfg;

/*
    Packets have varying lengths, with a minimum size of 5 bytes (3 header bytes, 0 data bytes and 2 CRC bytes)
    and a maximum size of 80 bytes.

    All Spektrum SRXL packets begin with a 3-byte header that contains the SRXL Manufacturer ID
    (0xA6), a Packet Type, and a packet Length in bytes.

    The payload data (0 to 75 bytes) for that packet type follows that header, and the packet ends with a 16-bit
    XMODEM CRC in big endian byte order.

    <0xA6><Packet_Type><Length><Data_bytes><CRC_Hi><CRC_Lo>

    The CRC is computed over all preceding packet data.
*/

/** @brief SRXL2 Message Length Max */
#define SRXL2_MAX_MSG_LEN 80

/**
 * @brief De-Initialize the SRXL2 module, stopping the PIO-SMs and clearing the programs.
 *
 * This stops the PIO State Machines used for receiving the data and detecting the
 * end of a message. Then it clears the PIO program memory, allowing the PIO to be
 * used for other operations.
 *
 */
extern void rx_srxl2_module_deinit();

/**
 * @brief Start receiving SRXL2 messages from the receiver and processing them.
 *
 */
extern void rx_srxl2_start();

/**
 * @brief Initialize the SRXL2 module for receiving RC data.
 *
 * @param baud The BAUD rate to use: 115,200 or 400,000.
 * @param channel_state Pointer to a rcrx_state_t Channel State to update with received data.
 */
extern void rx_srxl2_module_init(uint baud, rcrx_state_t* channel_state);

#ifdef __cplusplus
}
#endif
#endif // RX_SRXL2_H_
