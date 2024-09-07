/**
 * @brief Test, demo, and debugging routines.
 * @ingroup test
 *
 * Many of these aren't actually tests. Rather, they tend to be routines
 * that display patterns, send things to the terminal, get rotary control
 * values, etc., that can be helpful for seeing, demo'ing, and debugging
 * functionality.
 *
 * Copyright 2023-24 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#include "test.h"
#include "system_defs.h"

#include "board.h"
#include "config/config.h"
#include "term/term.h"
#include "util/util.h"

#include "hardware/rtc.h"
#include "pico.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "pico/printf.h"
#include "pico/time.h"
#include "pico/types.h"
#include "pico/util/datetime.h"

#include <string.h>

// The following are from 'config.c' to be used in
// `test_config_new_free()`
#define _TEST_CFG_MEM_MARKER_ 3224
struct _TEST_CFG_W_MARKER {
    uint16_t marker;
    config_t config;
};

int test_config_new_free(){
    int errors = 0;
    config_t* cfg = config_new(NULL);
    if (NULL == cfg) {
        errors++;
        error_printf("Test - Config: config_new returned NULL.");
        return (errors);
    }
    cfg->cfg_version = 9876;
    // Create a copy
    config_t* cfg_copy = config_new(cfg);
    if (NULL == cfg_copy) {
        errors++;
        error_printf("Test - Config: config_new returned NULL when creating copy.");
        config_free(cfg);
        return (errors);
    }
    if (cfg_copy->cfg_version != cfg->cfg_version) {
        errors++;
        error_printf("Test - Config: config copy cfg_version not correct.");
    }
    struct _TEST_CFG_W_MARKER* cfgwm = (struct _TEST_CFG_W_MARKER*)((uint8_t*)cfg - (sizeof(struct _TEST_CFG_W_MARKER) - sizeof(config_t)));
    if (cfgwm->marker != _TEST_CFG_MEM_MARKER_) {
        errors++;
        error_printf("Test - Config: Config structure memory marker not found.");
    }
    config_free(cfg_copy);
    config_free(cfg);
    if (errors == 0) {
        debug_printf("Test - Config: No errors running `test_config_new_free`");
    }
    else {
        error_printf("Test - Config: %d errors running `test_config_new_free`");
    }

    return (errors);
}


void test_error_printf() {
    error_printf("Test of printing an error: %d.", 15u);
}

void test_strdatetime() {
    char buf[128];
    datetime_t now;

    rtc_get_datetime(&now);

    strdatetime(buf, 127, &now, SDTC_TIME);
    printf("Time (h:mm): %s\n", buf);
    strdatetime(buf, 127, &now, SDTC_TIME_SECONDS);
    printf("Time (h:mm:ss): %s\n", buf);
    strdatetime(buf, 127, &now, SDTC_TIME_2DIGITS | SDTC_TIME_SECONDS);
    printf("Time (hh:mm:ss): %s\n", buf);
    strdatetime(buf, 127, &now, SDTC_TIME_24HOUR);
    printf("Time (24 hour): %s\n", buf);
    strdatetime(buf, 127, &now, SDTC_TIME_AMPM);
    printf("Time (AM/PM): %s\n", buf);
    strdatetime(buf, 127, &now, SDTC_DATE);
    printf("Date: %s\n", buf);
    strdatetime(buf, 127, &now, SDTC_DATE_SLASH);
    printf("Date ('/'): %s\n", buf);
    strdatetime(buf, 127, &now, SDTC_DATE_2DIGITS | SDTC_YEAR_2DIGITS);
    printf("Date (mm-dd-yy): %s\n", buf);
    strdatetime(buf, 127, &now, SDTC_DATE_2DIGITS | SDTC_DATE_ORDER_DM);
    printf("Date (dd-mm-yyyy): %s\n", buf);
    strdatetime(buf, 127, &now, SDTC_DATE | SDTC_TIME);
    printf("Date Time: %s\n", buf);
    strdatetime(buf, 127, &now, SDTC_TIME_BEFORE_DATE);
    printf("Time Date: %s\n", buf);
    strdatetime(buf, 127, &now, SDTC_LONG_TXT);
    printf("Date (string): %s\n", buf);
    strdatetime(buf, 127, &now, SDTC_DATE_SHORT_DM);
    printf("Date (short day/month): %s\n", buf);
    strdatetime(buf, 127, &now, SDTC_LONG_TXT_AT);
    printf("Text date 'at' time: %s\n", buf);
    strdatetime(buf, 127, &now, SDTC_LONG_TXT_ON);
    printf("Time 'on' text date: %s\n", buf);
}

alarm_id_t _test_term_notify_on_input_to_id;
static char _input;
static volatile bool _test_term_notify_on_input_called;
static void _test_term_notify_on_input(void) {
    // Function that is registered.
    _input = '\000';
    if (term_input_available()) {
        _input = term_getc();
    }
    _test_term_notify_on_input_called = true;
}
static int64_t _test_term_notify_on_input_to(alarm_id_t id, void* not_used) {
    cancel_alarm(id);
    _test_term_notify_on_input_to_id = 0;
    error_printf("\nTEST - test_term_notify_on_input timed out.");
    _input = -1;
    term_register_notify_on_input(NULL);
    _test_term_notify_on_input_called = true; // Say this was called, but the char is '-1'

    return (0); // Don't reschedule this timer
}
char test_term_notify_on_input(uint32_t timeout) {
    uint32_t time_waited = 0; // A secondary timeout
    _test_term_notify_on_input_called = false;
    _input = -1; // Return -1 if we timeout
    _test_term_notify_on_input_to_id = add_alarm_in_ms(timeout, _test_term_notify_on_input_to, NULL, true);
    // Register our callback and wait...
    term_register_notify_on_input(_test_term_notify_on_input);

    while (!_test_term_notify_on_input_called && time_waited < timeout + 250) {
        sleep_ms(10);
        time_waited += 10;
        if (time_waited % 500 == 0) {
            putchar('.');
        }
    }
    if (_test_term_notify_on_input_to_id != 0) {
        cancel_alarm(_test_term_notify_on_input_to_id);
    }
    term_register_notify_on_input(NULL);

    return (_input);
}

void test_term_scroll_area() {
    term_reset();
    term_set_type(VT_510_TYPE_SPEC, VT_510_ID_SPEC);
    term_set_title("μKOB v0.1");
    //term_set_title("leg v0.1");
    // Set the screen/page to a known size
    term_set_size(24, 80);
    // Put some text in the top fixed area
    term_cursor_moveto(1, 1);
    printf("TOP-FIXED-LEFT");
    term_cursor_moveto(1, 65);
    printf("TOP-FIXED-RIGHT");
    term_cursor_moveto(2, 1);
    printf("TOP-FIXED-LEFT-2");
    term_cursor_moveto(2, 63);
    printf("TOP-FIXED-RIGHT-2");
    // Put some text in the bottom fixed area
    term_cursor_moveto(14, 1);
    printf("BOTTOM-FIXED-LEFT-14");
    term_cursor_moveto(14, 59);
    printf("BOTTOM-FIXED-RIGHT-14");
    term_cursor_moveto(25, 1);
    printf("BOTTOM-FIXED-LEFT-24");
    term_cursor_moveto(25, 59);
    printf("BOTTOM-FIXED-RIGHT-24");
    term_cursor_moveto(2, 30);
    printf("Iteration    of 80");
    term_cursor_moveto(1, 35);
    printf("Cursor:");
    // Print a lines down the right side
    for (int l = 3; l < 14; l++) {
        term_cursor_moveto(l, 73);
        printf("Line %2d", l);
    }
    // Set a top fixed of 2 and a scroll area of 10
    term_set_margin_top_bottom(3, 13);
    term_cursor_on(false);
    // Now, print multiple lines in the scroll area
    // Print a scrolling barber pole
    char ca = 0;
    for (int i = 0; i < 20; i++) {
        for (int col = 0; col < 80; col++) {
            char c = '@' + ca + col;
            c &= 0x1F;
            c |= 0x40;
            putchar(c);
            term_cursor_save();
            text_position_t curpos = term_get_cursor_position();
            term_set_origin_mode(TERM_OM_UPPER_LEFT);
            term_cursor_moveto(1, 43);
            printf("%hd,%-3hd", curpos.line, curpos.column);
            term_set_origin_mode(TERM_OM_IN_MARGINS);
            term_cursor_restore();
        }
        term_cursor_save();
        term_set_origin_mode(TERM_OM_UPPER_LEFT);
        term_cursor_moveto(2, 40);
        printf("%2d", i+1);
        term_set_origin_mode(TERM_OM_IN_MARGINS);
        term_cursor_restore();
        sleep_ms(80);
        ca++;
    }
    term_set_origin_mode(TERM_OM_UPPER_LEFT); // Allow cursor in full screen
    term_set_margin_top_bottom(0,0);
    sleep_ms(2000);
    term_cursor_moveto(8, 40);
    term_erase_line();
    term_cursor_up_1();
    term_erase_bol();
    term_cursor_down(2);
    term_erase_eol();
    sleep_ms(2000);
    term_cursor_moveto(25, 0);
    term_cursor_on(true);
}

void test_term_screen_page_size() {
    // Set the size
    term_set_size(25, 80);
    term_clear();
    // Print 1-0... across the screen
    int text_color = 0;
    term_color_fg(text_color);
    for (int col = 1; col <= 80; col++) {
        if ((col - 1) % 10 == 0) {
            text_color++;
            term_color_fg(text_color);
        }
        putchar('0' + (col % 10));
    }
    putchar('\n');
    // Print a diagonal
    term_color_default();
    for (int line = 1; line <= 25; line++) {
        int col = (line - 1) * 4;
        int ac = (col < 75 ? col : 75);
        term_cursor_moveto(line, ac);
        printf("%2d,%-2d", line, col);
    }
    // Print the term ID in the center
    char buf[64];
    term_cursor_moveto(12, 35);
    if (term_get_id_info(VT_220_ID_SPEC, buf, 63) > 1) {
        printf("Term ID: 'ESC%s'", buf+1);
    }
    else {
        printf("No Term ID returned");
    }
    term_cursor_moveto(13, 35);
    if (term_get_name(buf, 63) > 1) {
        printf("Term Name: '%s'", buf);
    }
    else {
        printf("No Term Name returned");
    }
}
