/**
 * hwctrl Debugging flags and utilities.
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 */
#include "debug_support.h"

#include "cmt/cmt.h"
#include "util/util.h"

volatile uint16_t debugging_flags = 0;
static bool _debug_mode_enabled = false;

bool debug_mode_enabled() {
    return _debug_mode_enabled;
}

bool debug_mode_enable(bool on) {
    bool temp = _debug_mode_enabled;
    _debug_mode_enabled = on;
    if (on != temp && cmt_message_loops_running()) {
        cmt_msg_t msg;
        cmt_msg_init(&msg, MSG_DEBUG_CHANGED);
        msg.data.debug = _debug_mode_enabled;
        postBothMsgDiscardable(&msg);
    }
    return (temp != on);
}

