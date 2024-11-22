/**
 * HWControl Operating System - Base.
 *
 * Setup for the message loop and idle processing.
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#ifndef _DCS_H_
#define _DCS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "cmt/cmt.h"

#define DCS_CORE_NUM 1

/**
 * @brief Message loop context for use by the loop handler.
 * @ingroup dcs
 */
extern msg_loop_cntx_t dcs_msg_loop_cntx;

/**
 * @brief Initialize the Drive Control System
 * @ingroup dcs
 */
extern void dcs_module_init(void);

/**
 * @brief Start the Drive Control System  (DCS core 1 (endless) message-loop).
 * @ingroup dcs
 */
extern void start_dcs(void);


#ifdef __cplusplus
}
#endif
#endif // _DCS_H_
