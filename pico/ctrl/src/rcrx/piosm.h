/*
    PIO State Machine helpers.

    Copyright 2025 AESilky (SilkyDESIGN)
    SPDX-License-Identifier: MIT

*/
#ifndef PIOSM_H_
#define PIOSM_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "hardware/pio.h"

/**
 * @brief Structure containing the PIO SM program offset and configuration.
 *
 * This helps with restarting a State Machine and de-initializing a State
 * Machine, as both of these operations need both the configuration and
 * the program offset.
 *
 * @param offset uint Program offset
 * @param sm_cfg pio_sm_config Configuration of the State Machine
 */
typedef struct {
    uint offset;  // PIO program offset
    pio_sm_config sm_cfg;  // The configuration for the State Machine
} pio_sm_cfg;

/**
 * @brief Get the enabled (running) state of a PIO State Machine.
 *
 * @param pio The PIO block
 * @param sm The State Machine in the block
 * @return true The SM is enabled
 * @return false The SM is disabled
 */
static inline bool sm_enabled(PIO pio, uint sm) {
    return ((pio->ctrl & (1u << sm)) != 0);
}

/**
 * @brief Reset a PIO State Machine, including putting the PC at the start.
 *
 * This clears (most of) the status registers, the ISR and OSR, and sets the
 * PC back to the beginning of the program.
 * This leaves the State Machine disabled.
 *
 * @param pio The PIO block
 * @param sm  The State Machine in the block
 * @param smcfg The pio_sm_cfg containing the program offset and configuration
 */
static inline void sm_reset(PIO pio, uint sm, pio_sm_cfg smcfg) {
    pio_sm_init(pio, sm, smcfg.offset, &smcfg.sm_cfg);
    pio_sm_clear_fifos(pio, sm);
}

#ifdef __cplusplus
}
#endif
#endif // PIOSM_H_
