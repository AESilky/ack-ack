/**
 * @brief Human Interface Device functionality.
 * @ingroup hid
 *
 * Displays status and provide the human interface functions.
 *
 * Copyright 2023-24 AESilky
 *
 * SPDX-License-Identifier: MIT
 */

#include "hid.h"

#include "board.h"
#include "display/display.h"
#include "display/fonts/font.h"
#include "term/term.h"


// ############################################################################
// Constants Definitions
// ############################################################################
//
#define HID_DISPLAY_BG              C16_BLACK
#define HID_SENSBANK_ROW 7
#define HID_SENSBANK_COL 1
#define HID_SENSBANK_CHG_COLOR      C16_MAGENTA
#define HID_SENSBANK_UNCHG_COLOR    C16_LT_BLUE

// ############################################################################
// Function Declarations
// ############################################################################
//


// ############################################################################
// Data
// ############################################################################
//


// ############################################################################
// Message Handlers
// ############################################################################
//


// ############################################################################
// Internal Functions
// ############################################################################
//


// ############################################################################
// Public Functions
// ############################################################################
//
void hid_update_sensbank(sensbank_chg_t sb) {
    // SensBank has 8 sensor bits. Display each as an open box if sensor off
    // or filled box if sensor on. Display as orange is the sensor has changed,
    // display it as blue if it hasn't changed.
    uint8_t csv = sb.bits;
    uint8_t psv = sb.prev_bits;
    uint8_t bs = 0x80;
    for (int i = 0; i < 8; i++) {
        bool senson = (csv & bs) == 0;
        uint8_t ind = senson ? CHKBOX_CHECKED_CHR : CHKBOX_UNCHECKED_CHR;
        colorn16_t fg = (csv & bs) == (psv & bs) ? HID_SENSBANK_UNCHG_COLOR : HID_SENSBANK_CHG_COLOR;
        paint_control_t pc = i == 7 ? Paint : No_Paint;
        disp_char_color(HID_SENSBANK_ROW, HID_SENSBANK_COL + (i * 2), ind, fg, HID_DISPLAY_BG, pc);
        bs = bs >> 1;
    }
}


// ############################################################################
// Initialization and Maintainence Functions
// ############################################################################
//

void hid_start(void) {
    // Setup the screen for the status display and a scroll area for messages.
    disp_scroll_area_define(0, 0);
    disp_text_colors_set(C16_LT_GREEN, C16_BLACK);
    disp_clear(Paint);
    disp_scroll_area_define(10, 5);
    disp_cursor_home();
    //
    // Start the Terminal
    term_start();
}


void hid_module_init(void) {
    static bool _initialized = false;

    if (_initialized) {
        board_panic("hid_module_init already called");
    }
    _initialized = true;

    // Initialize the terminal portion of the HID
        // Init the terminal
    term_module_init();

}
