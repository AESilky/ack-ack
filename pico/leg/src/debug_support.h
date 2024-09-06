/**
 * leg Debugging flags and utilities.
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 */
#ifndef _DEBUG_SUP_H_
#define _DEBUG_SUP_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "stdbool.h"
#include "stdint.h"

extern volatile uint16_t debugging_flags;

/**
 * @brief Board level debug flag that can be changed by code.
 * @ingroup debug
 *
 * @return Debug flag state
 */
extern bool debug_mode_enabled();

/**
 * @brief Set the state of the board level debug flag.
 *        If changed, MSG_DEBUG_CHG will be posted if the message system is initialized.
 * @ingroup debug
 *
 * @param debug The new state
 * @return true If the state changed.
 */
extern bool debug_mode_enable(bool debug);

#ifdef __cplusplus
}
#endif
#endif // _DEBUG_SUP_H_
