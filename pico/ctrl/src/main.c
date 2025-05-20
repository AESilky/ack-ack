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
#include "dcs/dcs.h"
#include "hwos/hwos.h"
//
#include "display/display.h"
#include"tests.h"

//
#include "neopix/neopix.h"


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
    bi_decl(bi_program_description("OS and Control for AckAck-Rover Hardware"));

    // Uncomment to force starting in Debug Mode
    //debug_mode_enable(true);

    // Board/base level initialization
    if (board_init() != 0) {
        board_panic("Board init failed.");
    }

    led_on_off(say_hi);

    sleep_ms(800);

    // Initialize the multicore subsystem
    multicore_module_init(debug_mode_enabled());

    // Initialize the Cooperative Multi-Tasking subsystem
    cmt_module_init();

    // Set up the Hardware O.S. (needs to be done before starting the Direction Control System)
    hwos_module_init();

    // ZZZ - TEST the RC RX function...
    #include "rcrx/rcrx.h"
    rcrx_module_init();
    rcrx_start();


    // Set up the Drive Control System
    dcs_module_init();

    // Launch the Drive Control System (core-1 Message Dispatching Loop)
    //  This also starts the HID and other 'core-1' functionality.
    start_dcs();

    // Turn the green LED on.
    ledA_on(true);

    // Launch the Hardware Operation System (core-0 (endless) Message Dispatching Loop).
    // (!!! THIS NEVER RETURNS !!!)
    start_hwos();

    // How did we get here?!
    error_printf("hwctrl - Somehow we are out of our endless message loop in `main()`!!!");
    disp_clear(true);
    disp_string(1, 0, "!!!!!!!!!!!!!!!!", false, true);
    disp_string(2, 0, "! OS LOOP EXIT !", false, true);
    disp_string(3, 0, "!!!!!!!!!!!!!!!!", false, true);
    // ZZZ Reboot!!!
    return 0;
}
