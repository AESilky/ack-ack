/**
 * hwctrl Cooperative Multi-Tasking.
 *
 * 'Heap' for memory to hold a list of handlers for a message. This is used
 * rather than malloc/free, to avoid any memory fragmentation.
 *
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#ifndef CMT_HEAP_H_
#define CMT_HEAP_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "cmt_t.h"

/** @brief Message Handler Linked-List Entry */
typedef struct CMT_MSG_HDLR_LL_ENTRY_ {
    msg_handler_fn handler;
    struct CMT_MSG_HDLR_LL_ENTRY_* next;
    uint corenum;  // The core number this handler is for.
} cmt_msg_hdlr_ll_ent_t;

/**
 * @brief Get (allocate) a message handler linked-list entry.
 *
 * @return cmt_msg_hdlr_ll_ent_t* Entry for use.
 */
extern cmt_msg_hdlr_ll_ent_t* cmt_alloc_mhllent();

/**
 * @brief Return a message handler linked-list entry.
 *
 * @param mhllent Message handler linked-list entry pointer to return.
 */
extern void cmt_return_mhllent(cmt_msg_hdlr_ll_ent_t* mhllent);


extern void cmt_heap_module_init();

#ifdef __cplusplus
    }
#endif
#endif // CMT_HEAP_H_
