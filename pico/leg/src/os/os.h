/**
 * Back-End - Base.
 *
 * Setup for the message loop and idle processing.
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#ifndef _OS_H_
#define _OS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "cmt/cmt.h"

#define OS_CORE_NUM 0

/**
 * @brief Message loop context for use by the loop handler.
 * @ingroup backend
 */
extern msg_loop_cntx_t os_msg_loop_cntx;

/**
 * @brief Initialize the back-end
 * @ingroup backend
 */
extern void os_module_init(void);

/**
 * @brief Start the OS (core 0 (endless) message-loop).
 * @ingroup backend
 */
extern void start_os(void);


#ifdef __cplusplus
}
#endif
#endif // _OS_H_
