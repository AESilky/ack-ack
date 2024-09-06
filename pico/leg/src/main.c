/**
 * main entry point.
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#include "pico/binary_info.h"
//
#include "board.h"
#include "debug_support.h"
//
#include "os/os.h"
#include "config/config.h"
#include "fl/fl.h"
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
    bi_decl(bi_program_description("OS and Control for AES Ack-Ack Leg"));

    // Board/base level initialization
    if (board_init() != 0) {
        panic("Board init failed.");
    }

    // Indicate that we are awake
    if (debug_mode_enabled()) {
        tone_sound_duration(250);
    }
    int sc = sizeof(char);
    int ss = sizeof(short);
    int si = sizeof(int);
    int sl = sizeof(long);
    debug_printf("Size of char: %d  short: %d  int: %d  long: %d\n", sc, ss, si, sl);
    // Uncomment to force starting in Debug Mode
    //debug_mode_enable(true);

    led_on_off(say_hi);

    sleep_ms(1000);
    //disp_font_test();
    debug_mode_enable(true);

    // ZZZ - Test servo
    #include "servo/servo.h"
    #include "servo/receiver.h"
    int32_t inc = 15;
    int32_t tangle = 0;
    servo_enable(0);
    channel_enable(0);
    while(true) {
        if (tangle >= 890) {
            tangle = 890;
            inc *= -1;
        }
        else if (tangle <= -890) {
            tangle = -890;
            inc *= -1;
        }
        // servo_set_angle(0, tangle);
        tangle += inc;
        int32_t chang = channel_get_angle(0);
        uint32_t chns = channel_get_ns(0);
        debug_printf("% 3d:%4u", (chang/10), (chns/1000));
        sleep_us(100);
    }

    // Set up the OS (needs to be done before starting the Functional-Level)
    os_module_init();

    // Launch the Functional-Level (core-1 Message Dispatching Loop)
    // (This may seem backwards, starting FL before OS, but `start_os()` enters the endless loop)
    start_fl();

    // Launch the OS (core-0 (endless) Message Dispatching Loop
    // (!!! THIS NEVER RETURNS !!!)
    start_os();

    // How did we get here?!
    error_printf("leg - Somehow we are out of our endless message loop in `main()`!!!");
    disp_clear(true);
    disp_string(1, 0, "!!!!!!!!!!!!!!!!", false, true);
    disp_string(2, 0, "! OS LOOP EXIT !", false, true);
    disp_string(3, 0, "!!!!!!!!!!!!!!!!", false, true);
    // ZZZ Reboot!!!
    return 0;
}
