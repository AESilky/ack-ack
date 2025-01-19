/**
 * @brief Terminal Functionality - Interactive Terminal (minimal)
 * @ingroup terminal
 *
 * To run a getty on the tty port in Linux, use:
 * `systemctl start serial-getty@ttyACMn.service` [n=0,1,...]
 * To make it persist through reboots:
 * `systemctl enable serial-getty@ttyACMn.service`
 *
 * To kill a logged in session, use:
 * `ps -ft ttyACM1`
 * This will produce output similar to:
 * ```
 * UID        PID  PPID  C STIME TTY          TIME CMD
 * Usa        772  2701  0 15:26 ttyACM1  00:00:00 bash
 * ```
 * Use the PID to kill the process for the TTY to be killed:
 * `kill 772`
 * It is possible that *SIGKILL* will be needed:
 * `kill -9 772`
 *
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#include "term.h"
#include "term_ctrlchrs.h"
#include "tkbd.h"

#undef putc     // Use the function, not the macro
#undef putchar  // Use the function, not the macro

#include "cmt/cmt.h"
#include "curswitch/curswitch_t.h"
#include "display/display.h"
#include "touch_panel/touch.h"

#include "pico/printf.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"

#include <stdio.h>
#include <stdlib.h>


// ############################################################################
// Structure and Enum Definitions and Function Prototypes (internal)
// ############################################################################
//


// ############################################################################
// Function Declarations
// ############################################################################
//
static void _handle_switch_action(cmt_msg_t* msg);
static void _handle_touch(cmt_msg_t* msg);


// ############################################################################
// Data
// ############################################################################
//
#define INPUT_BUF_SIZE_        4096 // Make this a power of 2
#define BURST_MAX_SIZE_         100 // The maximum input burst to process at once

static char _input_buf[INPUT_BUF_SIZE_];
static bool _input_buf_overflow = false;
static uint16_t _input_buf_in = 0;
static uint16_t _input_buf_out = 0;

static msg_handler_fn _term_notify_on_input; // Holds a function pointer for a message when input is available


// ############################################################################
// Message Handlers
// ############################################################################
//

const msg_handler_entry_t term_switch_action_handler_entry = { MSG_SWITCH_ACTION, _handle_switch_action };
const msg_handler_entry_t term_touch_handler_entry = { MSG_TOUCH_PANEL, _handle_touch };


static void _handle_switch_action(cmt_msg_t* msg) {
    //
    // Handle switch actions so we can send 'canned' sequences to the host.
    //
    switch_id_t sw_id = msg->data.sw_action.switch_id;
    bool pressed = msg->data.sw_action.pressed;
    if (pressed) {
        switch (sw_id) {
        case SW_LEFT:
            stdio_puts_raw("ackr\r");
            break;
        case SW_RIGHT:
            stdio_puts_raw("asecret1\r");
            break;
        case SW_HOME:
            stdio_putchar_raw('\r');
            break;
        case SW_DOWN:
            stdio_puts_raw("ls -al\r");
            break;
        case SW_UP:
            stdio_puts_raw("cd ..\r");
            break;
        case SW_ENTER:
            stdio_puts_raw("cd ~\r");
            break;
        default:
            break;
        }
    }
}

static void _handle_touch(cmt_msg_t* msg) {
    const gfx_point* dp = tp_last_display_point();
    if (dp) {
        scr_position_t sp = disp_lc_from_point(dp);
        uint8_t kv = tkbd_get_csk(sp.column, sp.line);
        if ((kv & KBD_SPECIAL_KEY_FLAG) == 0) {
            // It's a normal character
            //  See if we are in Control mode
            if (tkbd_substate_get() == KSS_CONTROL) {
                // Yes. Turn the character into a control character
                kv = kv & 0x1F;
                tkbd_substate_set(KSS_NORMAL);
            }
            stdio_putchar_raw(kv);
        }
        else {
            // It's a special key
            switch (kv) {
                case KSK_BS:
                    stdio_putchar_raw(BS);
                    break;
                case KSK_CR:
                    stdio_putchar_raw(CR);
                    break;
                case KSK_PUNCTUATION:
                    tkbd_state_set(KS_PUNCTUATION);
                    break;
                case KSK_CTRL:
                    tkbd_substate_set(KSS_CONTROL);
                    break;
                case KSK_SHIFT:
                    tkbd_substate_set(KSS_SHIFT);
                    break;
                case KSK_SP:
                    // Space is special, just because it spans multiple key-caps
                    stdio_putchar_raw(' ');
                    break;
                default:
                    // Probably a tap outside of the keyboard area.
                    break;
            }
        }
    }
}

// ############################################################################
// Internal Functions
// ############################################################################
//

static void _post_msg_if_chars_available() {
    if (term_input_available()) {
        cmt_msg_t msg;
        cmt_msg_init(&msg, MSG_TERM_CHAR_RCVD);
        msg.hdlr = _term_notify_on_input;  // Load a handler (or NULL)
        // Post the message.
        postHWCtrlMsg(&msg);
    }
}

static bool _recv_buf_full(void) {
    bool full = ((_input_buf_in + 1) % INPUT_BUF_SIZE_) == _input_buf_out;
    return (full);
}

/**
 * @brief Callback function that is registered with the STDIO handler to be notified when characters become available.
 *
 * @param param Value that is passed to us that we registered with.
 */
static void _stdio_chars_available(void* param) {
    // Read the character
    int ci;
    int burst = 0;
    while (burst++ < BURST_MAX_SIZE_ && !_recv_buf_full() && (ci = getchar_timeout_us(0)) >= 0) {
        // Store it, then continue reading
        _input_buf[_input_buf_in] = (char)ci;
        _input_buf_in = (_input_buf_in + 1) % INPUT_BUF_SIZE_;
    }
    if (_recv_buf_full()) {
        // Flag that we are full.
        _input_buf_overflow = true;
    }
    _post_msg_if_chars_available();
}

static void _stdio_drain(void) {
    int ci;
    while ((ci = getchar_timeout_us(0)) >= 0);
}

/**
 * @brief Pulls the available data from the input buffer and displays it.
 *
 * This is a CMT Message handler, as it is called in response to a
 * MSG_TERM_CHARS_AVALABLE. There isn't anything important in the message.
 */
static void _rcv_disp(cmt_msg_t *msg) {
    int c;
    int burst = 0;
    while (burst++ < BURST_MAX_SIZE_ && (c = term_getc()) >= 0) {
        if (c < ' ') {
            switch (c) {
                case CBOL:
                    disp_cursor_bol();
                    break;
                case '\n':
                    disp_print_crlf(0, Paint);
                    break;
                default:
                    // It is another CTRL character
                    // We'll need to handle these... ZZZ
                    break;
            }
        }
        else {
            disp_printc((char)c, Paint);
        }
    }
}

// ############################################################################
// Public Functions
// ############################################################################
//

int term_getc(void) {
    if (!term_input_available()) {
        return (-1);
    }
    int c = _input_buf[_input_buf_out];
    _input_buf_out = (_input_buf_out + 1) % INPUT_BUF_SIZE_;

    return (c);
}

bool term_input_available() {
    return (_input_buf_in != _input_buf_out);
}

void term_input_buf_clear(void) {
    _input_buf_in = _input_buf_out = 0;
    _input_buf_overflow = false;
}

bool term_input_overflow() {
    bool retval = _input_buf_overflow;
    _input_buf_overflow = false;

    return (retval);
}

void term_register_notify_on_input(msg_handler_fn notify_fn) {
    _term_notify_on_input = notify_fn;
    // If the function is not NULL, post a message if anything is currently available.
    if (notify_fn) {
        _post_msg_if_chars_available();
    }
}



// ############################################################################
// Initialization and Maintainence Functions
// ############################################################################
//

void term_start() {
    // Set the display to make wrap nicer (for now)
    disp_print_wrap_len_set(0);
    uint16_t kb_line_top = disp_info_lines() - KB_LINES;
    tkbd_module_init(kb_line_top, 0, KS_LETTERS_LC, KSS_NORMAL);
    disp_cursor_show(true);
    //
    _stdio_drain();
    // Input handler...
    stdio_set_chars_available_callback(_stdio_chars_available, NULL);   // We can pass a parameter if we want
    // Register our 'received -> display' function to start
    term_register_notify_on_input(_rcv_disp);
    //
    stdio_putchar_raw('\r');
}

void term_module_init() {
}
