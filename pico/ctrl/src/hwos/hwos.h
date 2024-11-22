/**
 * HWControl Operating System - Base.
 *
 * Setup for the message loop and idle processing.
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#ifndef _HWOS_H_
#define _HWOS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "cmt/cmt.h"

#define HWOS_CORE_NUM 0

/**
 * @brief Message loop context for use by the loop handler.
 * @ingroup backend
 */
extern msg_loop_cntx_t hwos_msg_loop_cntx;

/**
 * @brief Initialize the back-end
 * @ingroup backend
 */
extern void hwos_module_init(void);

/**
 * @brief Start the Backend (core 0 (endless) message-loop).
 * @ingroup backend
 */
extern void start_hwos(void);


#ifdef __cplusplus
}
#endif
#endif // _HWOS_H_
