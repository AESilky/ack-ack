/**
 * HWControl Core 1 (DCS) main start-up and management.
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

#include "cmt/cmt.h"
#include "dcs/dcs.h"
#include "multicore.h"

#include "board.h"

#include "pico/multicore.h"

void core1_main() {
    info_printf(true, "CORE-%d - *** Started ***\n", get_core_num());

    // Enter into the (endless) Drive Control System Message Dispatching Loop
    message_loop(&dcs_msg_loop_cntx, (start_fn)0);
}
