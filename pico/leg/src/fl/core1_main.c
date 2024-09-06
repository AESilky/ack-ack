/**
 * Core 1 (Functional-Level) main start-up and management.
 *
 * This contains the main routine (the entry point) for operations on Core 1.
 *
 * See the core1_main.h header for important information.
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#include "core1_main.h"
#include "fl.h"

#include "cmt/cmt.h"
#include "cmt/multicore.h"

#include "board.h"

#include "pico/multicore.h"

void core1_main() {
    info_printf("CORE-%d Started\n", get_core_num());

    // Set up the Functional-Level
    fl_module_init();

    // Enter into the (endless) Functional-Level Message Dispatching Loop
    message_loop(&fl_msg_loop_cntx);
}
