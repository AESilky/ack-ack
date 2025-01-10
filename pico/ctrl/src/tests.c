/**
 * HWControl Miscellaneous Tests.
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 *
 *
*/
#include "tests.h"

#include "display/display.h"
#include "expio/expio.h"

static const colorn16_t colors[] = {
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

void test_display_1(int loops) {
    uint16_t lines = disp_info_lines();
    uint16_t cols = disp_info_columns();
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
    int loop = 0;
    while (loop < loops || loops == 0) {
        loop++;
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
}
