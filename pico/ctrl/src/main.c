/**
 * HWControl main application.
 *
 * Copyright 2023-24 AESilky
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
    bi_decl(bi_program_description("OS and Control for Ack-Rover Hardware"));

    // Uncomment to force starting in Debug Mode
    debug_mode_enable(true);

    // Board/base level initialization
    if (board_init() != 0) {
        panic("Board init failed.");
    }

    led_on_off(say_hi);

    sleep_ms(1000);
    //disp_font_test();

    uint16_t lines = disp_info_lines();
    uint16_t cols = disp_info_columns();
    colorn16_t colors[] = {
        C16_BLACK,
        C16_BLUE,
        C16_GREEN,
        C16_CYAN,
        C16_RED,
        C16_MAGENTA,
        C16_BROWN,
        C16_WHITE,
        C16_GREY,
        C16_LT_BLUE,
        C16_LT_GREEN,
        C16_LT_CYAN,
        C16_ORANGE,
        C16_VIOLET,
        C16_YELLOW,
    };
    uint8_t ac = 0;
    bool fixed_skip_scroll = false;
    uint16_t st = 3;
    uint16_t sb = 7;
    uint8_t cha = 0;
    char ch = '0';
    disp_text_colors_set(C16_BR_WHITE, colors[(ac++ % 15)]);
    disp_clear(Paint);
    disp_scroll_area_define(0, 0);
    disp_cursor_show(true);
    while (false) {
        // Print full lines down the display at fixed locations
        for (int l = 0; l < lines; l++) {
            if (fixed_skip_scroll) {
                if (l < st || l >= (lines - sb)) {
                    continue;
                }
            }
            ch = '0' + (cha++ % 10);
            disp_line_clear(l, Paint);
            disp_char(l, 0, '0' + (l % 10), Paint);
            for (int c = 2; c < cols; c++) {
                disp_char(l, c, ch + (c % 10), Paint);
            }
            disp_line_paint(l);
            disp_text_colors_set(C16_BR_WHITE, colors[(ac++ % 15)]);
        }
        if (!fixed_skip_scroll) {
            disp_text_colors_set(C16_BR_WHITE, colors[(ac++ % 15)]);
            disp_clear(Paint);
        }
        else {
            disp_cursor_home();
        }
        // Print full lines using the cursor so that it scrolls
        for (int l = 0; l < lines; l++) {
            disp_printc('0' + (l % 10), Paint);
            disp_printc(' ', Paint);
            ch = '0' + (cha++ % 10);
            for (int c = 2; c < cols; c++) {
                disp_text_colors_set(C16_BR_WHITE, colors[(ac++ % 15)]);
                disp_printc(ch + (c % 10), Paint);
            }
            ac++;
        }
        ac++;
        // Now print more lines to cause scroll
        for (int l = 0; l < lines; l++) {
            disp_printc('A' + (l % 10), Paint);
            disp_printc(' ', Paint);
            ch = '0' + (cha++ % 10);
            for (int c = 2; c < cols; c++) {
                disp_printc(ch + (c % 10), Paint);
            }
            disp_text_colors_set(C16_BR_WHITE, colors[(ac++ % 15)]);
        }
        // Set a top and bottom fixed area (no-scroll)
        disp_scroll_area_define(st, sb);
        disp_cursor_home();
        fixed_skip_scroll = !fixed_skip_scroll;
        ac += 3;
        //disp_scroll_area_clear(Paint);
    }

    // Initialize the multicore subsystem
    multicore_module_init(debug_mode_enabled());

    // Initialize the Cooperative Multi-Tasking subsystem
    cmt_module_init();

    // Set up the Hardware O.S. (needs to be done before starting the Direction Control System)
    hwos_module_init();

    // Launch the Drive Control System (core-1 Message Dispatching Loop)
    start_dcs();

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
