/**
 * HWControl Operating System - Base.
 *
 * Setup for the message loop and idle processing.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#ifndef _HWRT_H_
#define _HWRT_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "cmt/cmt.h"

#define HWRT_CORE_NUM 0

/**
 * @brief Initialize the back-end
 * @ingroup backend
 */
extern void hwrt_module_init(void);

/**
 * @brief Start the Backend (core 0 (endless) message-loop).
 * @ingroup backend
 */
extern void start_hwrt(void);


#ifdef __cplusplus
}
#endif
#endif // _HWRT_H_
