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
 * @brief Scheduled Message Data Structure/Type
 */
typedef struct SCHEDULED_MSG_DATA_ {
    int32_t remaining;
    uint8_t corenum;
    int32_t ms_requested;
    cmt_msg_t msg;          // Message gets copied into this
} cmt_sch_msg_data_t;

/** @brief Scheduled Message Data Linked-List Entry */
typedef struct CMT_SCHMSGDATA_LL_ENTRY_ {
    cmt_sch_msg_data_t schmsg_data;
    struct CMT_SCHMSGDATA_LL_ENTRY_* next;
} cmt_schmsgdata_ll_ent_t;


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

/**
 * @brief Get (allocate) a scheduled message data linked-list entry.
 *
 * @return cmt_schmsgdata_ll_ent_t* Entry for use.
 */
extern cmt_schmsgdata_ll_ent_t* cmt_alloc_smdllent();

/**
 * @brief Return a message handler linked-list entry.
 *
 * @param mhllent Message handler linked-list entry pointer to return.
 */
extern void cmt_return_smdllent(cmt_schmsgdata_ll_ent_t* smllent);


extern void cmt_heap_module_init();

#ifdef __cplusplus
    }
#endif
#endif // CMT_HEAP_H_
