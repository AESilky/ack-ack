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

#include "board.h"

/* Enough entries for 4 handlers for each possible message (of course, they can be used as needed) */
#define CMT_MHLLENT_CNT (256*4)
static cmt_msg_hdlr_ll_ent_t _cmt_mhllent[CMT_MHLLENT_CNT];
static cmt_msg_hdlr_ll_ent_t* _mh_free_list;

/* Allow for 32 outstanding scheduled messages (includes 'sleep') */
#define CMT_SCHEDULED_MESSAGES_MAX 32
static cmt_schmsgdata_ll_ent_t _cmt_smllent[CMT_SCHEDULED_MESSAGES_MAX];
static cmt_schmsgdata_ll_ent_t* _smd_free_list;

cmt_msg_hdlr_ll_ent_t* cmt_alloc_mhllent() {
    // Get one from the free list and hand it out.
    cmt_msg_hdlr_ll_ent_t* ent = _mh_free_list;
    if (ent == (cmt_msg_hdlr_ll_ent_t*)NULL) {
        board_panic("!!! cmt_alloc_mhllent - Out of Message Handler LL entries. !!!");
    }
    _mh_free_list = ent->next;
    ent->next = (cmt_msg_hdlr_ll_ent_t*)NULL;
    return (ent);
}

void cmt_return_mhllent(cmt_msg_hdlr_ll_ent_t* mhllent) {
    // Put the entry back into the free list.
    if (mhllent != (cmt_msg_hdlr_ll_ent_t*)NULL) {
        mhllent->next = _mh_free_list;
        _mh_free_list = mhllent;
    }
}

cmt_schmsgdata_ll_ent_t* cmt_alloc_smdllent() {
    // Get an entry from the free list and hand it out.
    cmt_schmsgdata_ll_ent_t* ent = _smd_free_list;
    if (ent == (cmt_schmsgdata_ll_ent_t*)NULL) {
        board_panic("!!! cmt_alloc_smdllent - Out of Scheduled Message Data LL entries. !!!");
    }
    _smd_free_list = ent->next;
    ent->next = (cmt_schmsgdata_ll_ent_t*)NULL;
    return (ent);
}

void cmt_return_smdllent(cmt_schmsgdata_ll_ent_t* smdllent) {
    // Put the entry back into the free list.
    if (smdllent != (cmt_schmsgdata_ll_ent_t*)NULL) {
        smdllent->next = _smd_free_list;
        _smd_free_list = smdllent;
    }
}


void cmt_heap_module_init() {
    // Link all of our entries into the free lists.
    _mh_free_list = &_cmt_mhllent[0];
    for (int i=0; i < (CMT_MHLLENT_CNT - 1); i++) {
        _cmt_mhllent[i].next = &_cmt_mhllent[i+1];
        _cmt_mhllent[i+1].next = (cmt_msg_hdlr_ll_ent_t*)NULL;
    }
    _smd_free_list = &_cmt_smllent[0];
    for (int i = 0; i < (CMT_SCHEDULED_MESSAGES_MAX - 1); i++) {
        _cmt_smllent[i].next = &_cmt_smllent[i + 1];
        _cmt_smllent[i + 1].next = (cmt_schmsgdata_ll_ent_t*)NULL;
    }
}
