/**
 * hwctrl Cooperative Multi-Tasking - Message Handlers.
 *
 * Contains message loop, timer, and other CMT enablement functions.
 *
 * This contains the declaration for the Sleep Message Handler. It is
 * included in both cores handler lists.
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#ifndef CMT_MH_H_
#define CMT_MH_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "cmt_t.h"

/**
 * @brief Handler Entry for CMT Sleep. This is put in both message loop
 *      handler lists, so a sleep can be handled for either.
 * @ingroup cmt
 *
 */
extern const msg_handler_entry_t cmt_sm_tick_handler_entry;


#ifdef __cplusplus
    }
#endif
#endif // CMT_MH_H_

