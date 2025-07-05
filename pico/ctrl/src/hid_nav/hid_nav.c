/**
 * @brief Human Interface Device functionality.
 * @ingroup hid
 *
 * Displays status and provide the human interface functions.
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */

#include "hid_nav.h"

#include "board.h"
#include "curswitch/curswitch.h"
#include "display/display.h"                        // For character/line based operations
#include "display/display_rgb18/display_rgb18.h"    // For pixel/graphics based operations
#include "display/fonts/font.h"
#include "neopix/neopix.h"
#include "rotary_encoder/re_pbsw.h"
#include "rotary_encoder/rotary_encoder.h"
#include "sensbank/sensbank.h"
#include "term/term.h"
#include "touch_panel/touch.h"

#include <stdio.h>

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
static void _show_psa(proc_status_accum_t* psa, int corenum);

// Interrupt handler functions...
static void _gpio_irq_handler(uint gpio, uint32_t events);
static void _input_sw_irq_handler(uint32_t events);

// Message handler functions...
static void _handle_hid_housekeeping(cmt_msg_t* msg);
//
static void _handle_input_sw_debounce(cmt_msg_t* msg);
static void _handle_rotary_change(cmt_msg_t* msg);
static void _handle_sensbank_change(cmt_msg_t* msg);
static void _handle_switch_action(cmt_msg_t* msg);
static void _handle_switch_longpress_delay(cmt_msg_t* msg);


// ############################################################################
// Data
// ############################################################################
//
static switch_id_t _sw_pressed = SW_NONE;
static cmt_msg_t _sw_longpress_msg = { MSG_SW_LONGPRESS_DELAY };
static bool _input_sw_pressed;
static cmt_msg_t _input_sw_debounce_msg = { MSG_INPUT_SW_DEBOUNCE };


// ====================================================================
// Interrupt (irq) handler functions
// ====================================================================

static void _gpio_irq_handler(uint gpio, uint32_t events) {
    switch (gpio) {
    case IRQ_INPUT_SW:
        _input_sw_irq_handler(events);
        break;
    case IRQ_ROTARY_TURN:
        re_turn_irq_handler(gpio, events);
        break;
    }
}

static void _input_sw_irq_handler(uint32_t events) {
    // The GPIO needs to be low for at least 80ms to be considered a button press.
    if (events & GPIO_IRQ_EDGE_FALL) {
        // Delay to see if it is user input or an IR received.
        // Check to see if we have already scheduled a debounce message.
        if (!scheduled_msg_exists(MSG_INPUT_SW_DEBOUNCE)) {
            schedule_msg_in_ms(80, &_input_sw_debounce_msg);
        }
    }
    if (events & GPIO_IRQ_EDGE_RISE) {
        scheduled_msg_cancel(MSG_INPUT_SW_DEBOUNCE);
        if (_input_sw_pressed) {
            _input_sw_pressed = false;
            cmt_msg_t msg;
            cmt_msg_init(&msg, MSG_INPUT_SW_RELEASE);
            postDCSMsg(&msg);
        }
    }
}


// ############################################################################
// Message Handlers
// ############################################################################
//
static void _disp_proc_status(void* data) {
    // Output status every 7 seconds
    cmt_sleep_ms(7000, _disp_proc_status, NULL);
    // Output the current state
    for (int i = 0; i < 2; i++) {
        proc_status_accum_t psa;
        cmt_proc_status_sec(&psa, i);
        // Display the proc status...
        _show_psa(&psa, i);
    }
}

static void _handle_hid_housekeeping(cmt_msg_t* msg) {
    static gfx_point last_touch = { 0,0 };

    // Read the switch banks
    curswitch_trigger_read();
    // Read the touch panel
    const gfx_point* dp = tp_check_display_point();
    if (dp) {
        if (dp->x != last_touch.x || dp->y != last_touch.y) {
            // Store the values and print that we were touched.
            last_touch.x = dp->x;
            last_touch.y = dp->y;
            //
            // Post a message with the touch
            cmt_msg_t msg;
            cmt_msg_init(&msg, MSG_TOUCH_PANEL);
            postHWRTMsgDiscardable(&msg);
            // const gfx_point* pp = tp_last_panel_point();
            // const gfx_rect* bounds = tp_bounds_observed();
            // scr_position_t sp = disp_lc_from_point(dp);
            // char buf[64];
            // snprintf(buf, 63, "T Dx=%3d Dy=%3d Px=%4d Py=%4d", last_touch.x, last_touch.y, pp->x, pp->y);
            // disp_string_color(0, 0, buf, C16_LT_BLUE, C16_BLACK, No_Paint);
            // snprintf(buf, 63, "B: (%4d,%4d , %4d,%4d)", bounds->p1.x, bounds->p1.y, bounds->p2.x, bounds->p2.y);
            // disp_string_color(1, 0, buf, C16_LT_BLUE, C16_BLACK, Paint);
            // snprintf(buf, 63, "SCR: Line:%2d Col:%2d", sp.line, sp.column);
            // disp_string_color(2, 0, buf, C16_LT_BLUE, C16_BLACK, Paint);
        }
    }
}

static void _handle_input_sw_debounce(cmt_msg_t* msg) {
    _input_sw_pressed = user_switch_pressed(); // See if it's still pressed
    if (_input_sw_pressed) {
        cmt_msg_t msg;
        cmt_msg_init(&msg, MSG_INPUT_SW_PRESS);
        postDCSMsg(&msg);
    }
}

static void _handle_rotary_change(cmt_msg_t* msg) {
    // The rotary encoder has been turned.
    int32_t rotary_cnt = re_count();
    debug_printf("RE: p:%5d d:%3hd\n", rotary_cnt, msg->data.value16);
}

static void _handle_sensbank_change(cmt_msg_t* msg) {
    // Handle changes in the sensor bank bits.
    debug_printf("SB Chg: %02x -> %02x\n", msg->data.sensbank_chg.prev_bits, msg->data.sensbank_chg.bits);
    hid_update_sensbank(msg->data.sensbank_chg);
}

static void _handle_switch_action(cmt_msg_t* msg) {
    // Handle switch actions so we can detect a long press
    // and post a message for it.
    //
    // We keep track of one switch in each bank. We assume
    // that only one switch (per bank) can be pressed at
    // a time, so we only keep track of the last one pressed.
    //
    switch_id_t sw_id = msg->data.sw_action.switch_id;
    bool pressed = msg->data.sw_action.pressed;
    if (!pressed) {
        // Clear any long press in progress
        scheduled_msg_cancel(MSG_SW_LONGPRESS_DELAY);
        _sw_pressed = SW_NONE;
    }
    else {
        // Start a delay timer
        switch_action_data_t* sad = &_sw_longpress_msg.data.sw_action;
        _sw_pressed = sw_id;
        sad->switch_id = sw_id;
        sad->pressed = true;
        sad->repeat = false;
        schedule_msg_in_ms(SWITCH_LONGPRESS_MS, &_sw_longpress_msg);
    }
}

static void _handle_switch_longpress_delay(cmt_msg_t* msg) {
    // Handle the long press delay message to see if the switch is still pressed.
    switch_id_t sw_id = msg->data.sw_action.switch_id;
    bool repeat = msg->data.sw_action.repeat;
    cmt_msg_t* slpmsg = NULL;
    if (sw_id == _sw_pressed) {
        // Prepare to post another delay msg.
        slpmsg = &_sw_longpress_msg;
    }
    else {
        sw_id = SW_NONE;
    }
    if (sw_id != SW_NONE) {
        // Yes, the same switch is still pressed
        cmt_msg_t msg;
        cmt_msg_init(&msg, MSG_SW_LONGPRESS);
        msg.data.sw_action.switch_id = sw_id;
        msg.data.sw_action.pressed = true;
        msg.data.sw_action.repeat = repeat;
        postBothMsgDiscardable(&msg);
        // Schedule another delay
        switch_action_data_t* sad = &slpmsg->data.sw_action;
        sad->switch_id = sw_id;
        sad->pressed = true;
        sad->repeat = true;
        uint16_t delay = (repeat ? SWITCH_REPEAT_MS : SWITCH_LONGPRESS_MS);
        schedule_msg_in_ms(delay, slpmsg);
    }
}


// ############################################################################
// Internal Functions
// ############################################################################
//
static void _show_psa(proc_status_accum_t* psa, int corenum) {
    long active = psa->t_active;
    int retrieved = psa->retrieved;
    int msg_id = psa->msg_longest;
    long msg_t = psa->t_msg_longest;
    int interrupt_status = psa->interrupt_status;
    float busy = (float)active / 10000.0f; // Divide by 10,000 rather than 1,000,000 for percent
    float core_temp = onboard_temp_c();
    printf("PSA %d: Active: % 3.2f%%\t At:%ld\tMR:%d\t Temp: %3.1f\t Msg: %03X Msgt: %ld\t Int:%08x\n", corenum, busy, active, retrieved, core_temp, msg_id, msg_t, interrupt_status);
}

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
static void _module_init(void) {
    static bool _initialized = false;

    if (_initialized) {
        board_panic("!!! HID _module_init already called. !!!");
    }
    _initialized = true;

    // Add our message handlers
    cmt_msg_hdlr_add(MSG_INPUT_SW_DEBOUNCE, _handle_input_sw_debounce);
    cmt_msg_hdlr_add(MSG_ROTARY_CHG, _handle_rotary_change);
    cmt_msg_hdlr_add(MSG_SENSBANK_CHG, _handle_sensbank_change);
    cmt_msg_hdlr_add(MSG_SW_ACTION, _handle_switch_action);
    cmt_msg_hdlr_add(MSG_SW_LONGPRESS_DELAY, _handle_switch_longpress_delay);

    re_pbsw_module_init();
    rotary_encoder_module_init();
    gpio_set_irq_enabled_with_callback(IRQ_ROTARY_TURN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &_gpio_irq_handler);
    // gpio_set_irq_enabled(IRQ_rotary_SW, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    // gpio_set_irq_enabled(IRQ_TOUCH, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    // Initialize the display
    disp_module_init();
    // Cursor Switches module.
    curswitch_module_init();
    // Touch Panel initialization
    tp_module_init(5, gfxd_screen_width(), false, gfxd_screen_height(), true, 121, 2520, 122, 2603);

    // Initialize the terminal portion of the HID
    term_module_init();

    neopix_module_init();
}

void start_hid_nav(void) {
    // Initialize modules used by the HID
    _module_init();
    
    // Setup the screen for the status display and a scroll area for messages.
    disp_scroll_area_define(0, 0);
    disp_text_colors_set(C16_LT_GREEN, C16_BLACK);
    disp_clear(Paint);
    disp_scroll_area_define(10, 5);
    disp_cursor_home();
    //
    // Start the Terminal
    term_start();
    //
    // Start the NeoPixel panels
    neopix_start();
    //
    // Output status every 7 seconds
    //cmt_sleep_ms(7000, _disp_proc_status, NULL);
}
