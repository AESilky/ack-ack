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
#include "cmt_heap.h"

/* Enough entries for 4 handlers for each possible message (off course, they can be used as needed) */
#define CMT_MHLLENT_CNT (256*4)
cmt_msg_hdlr_ll_ent_t cmt_mhllent[CMT_MHLLENT_CNT];

cmt_msg_hdlr_ll_ent_t* _free_list;


cmt_msg_hdlr_ll_ent_t* cmt_alloc_mhllent() {
    // Get one from the free list and hand it out.
    cmt_msg_hdlr_ll_ent_t* ent = _free_list;
    _free_list = ent->next;

    return (ent);
}

void cmt_return_mhllent(cmt_msg_hdlr_ll_ent_t* mhllent) {
    // Put the entry back into the free list.
    if (mhllent != (cmt_msg_hdlr_ll_ent_t*)NULL) {
        mhllent->next = _free_list;
        _free_list = mhllent;
    }
}


void cmt_heap_module_init() {
    // Link all of our entries into the free list.
    _free_list = &cmt_mhllent[0];
    for (int i=0; i < (CMT_MHLLENT_CNT - 1); i++) {
        cmt_mhllent[i].next = &cmt_mhllent[i+1];
        cmt_mhllent[i+1].next = (cmt_msg_hdlr_ll_ent_t*)NULL;
    }
}
