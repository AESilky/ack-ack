/**
 * HWControl main application.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#include "pico/binary_info.h"
//
#include "system_defs.h" // Main system/board/application definitions
//
#include "board.h"
#include "debug_support.h"

//
#include "multicore.h"
#include "cmt/cmt.h"

//
#include "hwrt/hwrt.h"
//
#include "display/display.h"
#include "termx/termx_min.h"
#include "tests.h"

#include <stdio.h>


#define DOT_MS 60 // Dot at 20 WPM
#define UP_MS  DOT_MS
#define DASH_MS (2 * DOT_MS)
#define CHR_SP (3 * DOT_MS)

 // 'H' (....) 'I' (..)
static const int32_t say_hi[] = {
    DOT_MS,
    UP_MS,
    DOT_MS,
    UP_MS,
    DOT_MS,
    UP_MS,
    DOT_MS,
    CHR_SP,
    DOT_MS,
    UP_MS,
    DOT_MS,
    1000, // Pause before repeating
    0 };

int main()
{
    // useful information for picotool
#if (BOARD_ADDR == 0)
    bi_decl(bi_program_description("Runtime and Drive Control for AckAck-Rover"));
    char board = '0';
#else
    bi_decl(bi_program_description("Human Interface Device & Navigation (HID_NAV) for AckAck-Rover"));
    char board = '1';
#endif

    // Board/base level initialization
    if (board_init() != 0) {
        board_panic("Board init failed.");
    }
    // Force setting Debug Mode based on Board Address (override User Switch)
    if (board_addr() == 0) {
        debug_mode_enable(false);
    }
    else {
        debug_mode_enable(true);
    }

    // int8_t sb = -128;
    // uint8_t  ub = -128;
    // for (int i = 0; i < 256; i++) {
    //     printf("signed byte: %d (%02X)  unsigned byte: %u (%02X)\n", sb, sb, ub, ub);
    //     sb++;
    //     ub++;
    // }

    printf("%sACK-ACK Board %c%s\n", TERMX_START_RED_STR, board, TERMX_DEFAULT_COLOR_STR);
    led_on_off(say_hi);

    sleep_ms(800);

    // Initialize the multicore subsystem
    multicore_module_init(debug_mode_enabled());

    // Initialize the Cooperative Multi-Tasking subsystem
    cmt_module_init();

    // Turn the Secondary 'A' LED on.
    ledA_on(true);

    // Starting Core-1 will run the `core1_main` which is defined for the appropriate
    // Board-0 or Board-1 functionality.
    start_core1();

    // Launch the Hardware Runtime (core-0 (endless) Message Dispatching Loop).
    // The HWRT starts the appropriate secondary operations (core-1 message loop)
    // (!!! THIS NEVER RETURNS !!!)
    start_hwrt();

    // How did we get here?!
    error_printf("hwctrl - Somehow we are out of our endless message loop in `main()`!!!");
#if (BOARD_ADDR == 1)
    disp_clear(true);
    disp_string(1, 0, "!!!!!!!!!!!!!!!!!!!", false, true);
    disp_string(2, 0, "! HW RT LOOP EXIT !", false, true);
    disp_string(3, 0, "!!!!!!!!!!!!!!!!!!!", false, true);
#endif
    // ZZZ Reboot!!!
    return 0;
}
