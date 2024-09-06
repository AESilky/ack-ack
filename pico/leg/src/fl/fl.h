/**
 * leg User Interface - Base.
 *
 * Setup for the message loop and idle processing.
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#ifndef _UI_H_
#define _UI_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "cmt/cmt.h"

#define UI_CORE_NUM 1

/**
 * @brief Message loop context for use by the loop handler.
 * @ingroup ui
 */
extern msg_loop_cntx_t fl_msg_loop_cntx;

/**
 * @brief Start the Functional-Level (core 1 main and (endless) message-loop).
 */
extern void start_fl(void);

/**
 * @brief True if the Functional-Level has been initialized.
 */
extern bool fl_initialized();

/**
 * @brief Initialize the Functional-Level
 */
extern void fl_module_init(void);

#ifdef __cplusplus
}
#endif
#endif // _UI_H_
