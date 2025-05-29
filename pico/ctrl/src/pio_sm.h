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
 * @param pio PIO instance
 * @param sm uint State Machine
 * @param offset uint Program offset
 * @param sm_cfg pio_sm_config Configuration of the State Machine
 */
typedef struct {
    PIO pio;                // The PIO
    uint sm;                // The State Machine
    uint offset;            // PIO program offset
    pio_sm_config sm_cfg;   // The configuration for the State Machine
} pio_sm_pocfg;

/**
 * @brief Get the PIO State Machine PC value.
 *
 * This is the PC adjusted for the program load offset.
 *
 * @param smpocfg The pio_sm_pocfg containing the PIO, SM, and offset
 * @return uint8_t The PC value (adjusted for the program offset)
 */
static inline uint8_t piosm_pc(pio_sm_pocfg smpocfg) {
    return (pio_sm_get_pc(smpocfg.pio, smpocfg.sm) - smpocfg.offset);
}


/**
 * @brief Get the enabled (running) state of a PIO State Machine.
 *
 * @param pio The PIO block
 * @param sm The State Machine in the block
 * @return true The SM is enabled
 * @return false The SM is disabled
 */
static inline bool piosm_enabled(PIO pio, uint sm) {
    return ((pio->ctrl & (1u << sm)) != 0);
}

/**
 * @brief Get the enabled (running) state of a PIO State Machine.
 *
 * @param smpocfg The PIO-SM Pgm-offset and Config
 * @return true The SM is enabled
 * @return false The SM is disabled
 */
static inline bool piosm_enabled2(pio_sm_pocfg smpocfg) {
    return (piosm_enabled(smpocfg.pio, smpocfg.sm));
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
 * @param smcfg The pio_sm_pocfg containing the program offset and configuration
 */
static inline void piosm_reset(pio_sm_pocfg smpocfg) {
    pio_sm_init(smpocfg.pio, smpocfg.sm, smpocfg.offset, &smpocfg.sm_cfg);
    pio_sm_clear_fifos(smpocfg.pio, smpocfg.sm);
}

#ifdef __cplusplus
}
#endif
#endif // PIOSM_H_
