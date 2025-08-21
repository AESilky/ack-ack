/**
 * hwctrl Cooperative Multi-Tasking.
 *
 * Contains message loop, timer, and other CMT enablement functions.
 *
 * This is the include file for the 'types' used by CMT.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#ifndef CMT_T_H_
#define CMT_T_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "pico/types.h"

#include <stdbool.h>
#include <stdint.h>

// Includes for types used in the Message Data
#include "curswitch/curswitch_t.h"
#include "rcrx/rcrx_t.h"
#include "sensbank/sensbank_t.h"
#include "servo/servo_t.h"

/** @brief Special identifier to specify a handler for both cores. */
#define MSG_HDLR_CORE_BOTH ((uint)-1)

// Keep the total number of messages under 256 to allow indexing into handlers.
typedef enum MSG_ID_ {
    // Common messages 0x00 - 0x5F (used by both HWRT and DCS/HID)
    MSG_NOOP = 0x00,
    MSG_LOOP_STARTED,
    MSG_HWRT_STARTED,
    MSG_DCS_STARTED,
    MSG_HID_STARTED,
    MSG_PERIODIC_RT,        // Periodic Repeating Time - Every 16ms (62.5Hz)
    MSG_CMT_SLEEP,
    MSG_EXEC,               // General purpose message that can be used when specifying a handler.
    MSG_CONFIG_CHANGED,
    MSG_DEBUG_CHANGED,
    MSG_RC_FAILSAFE_CHG,    // The 'FailSafe' state of the Radio Control has changed
    MSG_RC_RECEIVED,        // A Radio Control message has been received and processed (ready for use)
    MSG_SENSBANK_CHG,       // Data has Previous and Current sensor bit values
    MSG_SW_ACTION,
    MSG_SW_LONGPRESS,
    MSG_SW_LONGPRESS_DELAY,
    MSG_TERM_CHAR_RCVD,
    //
    // Hardware-Runtime (HWRT) messages 0x60 - 0xBF
    MSG_HWRT_NOOP = 0x60,
    MSG_HWRT_TEST,
    MSG_ALERT_SW_PRESS, // The 'ALERT' (Red) Switch was pressed
    MSG_RC_DETECTING,   // Radio Control BAUD & Protocol being detected
    MSG_RC_DETECT_DA,   // Radio Control Detect - Data Available
    MSG_RC_DETECTED,    // Radio Control BAUD & Protocol detected
    MSG_RC_RX_RAW_ERR,      // Radio Control receiver RX error (Parity +/ Framing)
    MSG_RC_RX_RAW_MSG_RCVD, // Radio Control receiver message had been received
    MSG_RC_RX_RAW_MSG_RDY,  // Radio Control receiver message is ready
    MSG_SERVO_DATA_RCVD,
    MSG_SERVO_DATA_RX_TO,
    MSG_SERVO_READ_ERROR,
    MSG_SERVO_STATUS_RCVD,
    MSG_STDIO_CHAR_READY,
    MSG_STOP_SW_PRESS,
    MSG_STOP_SW_DEBOUNCE,
    MSG_STOP_SW_RELEASE,
    MSG_TOUCH_PANEL,
    //
    // Drive Control System (DCS), Human Interface Devices (HID), and Navigation (NAV) messages 0xC0 - 0xFF
    MSG_DCS_NOOP = 0xC0,
    MSG_HID_NOOP = 0xC0,
    MSG_DCS_TEST,
    MSG_DIRECT_CTRL_CHG,
    MSG_FORWARD_ROTATE_REVERSE_CHG,
    MSG_INPUT_SW_PRESS,
    MSG_INPUT_SW_DEBOUNCE,
    MSG_INPUT_SW_RELEASE,
    MSG_ROTARY_CHG,
    MSG_ROTARY_SW_PRESS,
    MSG_DISPLAY_MESSAGE,
    MSG_UB_GREEN_PR,
    MSG_UB_YELLOW_PR
} msg_id_t;
#define MSG_ID_CNT (0x100)

/**
 * @brief Function prototype for a sleep function.
 * @ingroup cmt
 */
typedef void (*cmt_sleep_fn)(void* user_data);

typedef struct cmt_sleep_data_ {
    cmt_sleep_fn sleep_fn;
    void* user_data;
} cmt_sleep_data_t;

// Declare the CMT Message structure so that we can declare the handler function.
struct CMT_MSG_;

/**
 * @brief Function prototype for a message handler.
 * @ingroup cmt
 *
 * @param msg The message to handle.
 */
typedef void (*msg_handler_fn)(struct CMT_MSG_* msg);

#define NULL_MSG_HDLR ((msg_handler_fn)0)

/**
 * @brief Message data.
 *
 * Union that can hold the data needed by the messages.
 */
union MSG_DATA_VALUE_ {
    char c;
    bool bv;
    bool debug;
    cmt_sleep_data_t cmt_sleep;
    int16_t value16;
    uint16_t value16u;
    int32_t status;
    uint32_t value32u;
    char* str;
    rcrx_bp_t rcrx_bp;
    sensbank_cah_t sensbank_chg;
    servo_params_t servo_params;
    switch_action_data_t sw_action;
    uint32_t ts_ms;
    uint64_t ts_us;
};
typedef union MSG_DATA_VALUE_ msg_data_value_t;

/**
 * @brief Structure containing a message ID and message data.
 *
 * It is suggested that the CMT methods be used for initializing a cmt_msg_t
 * object for use.
 *
 * @param data The data for the message.
 * @param hdlr A Handler function to use rather then the one registered (or null).
 * @param id The ID (number) of the message.
 * @param abort Controls whether the next registered handler should be run.
 * @param n The message number (set by the posting system)
 * @param t The millisecond time msg was posted (set by the posting system)
 */
typedef struct CMT_MSG_ {
    msg_id_t id;
    bool abort;
    msg_data_value_t data;
    msg_handler_fn hdlr;
    uint32_t n;
    uint32_t t;
} cmt_msg_t;

static inline void cmt_msg_init_ctrl(cmt_msg_t* msg, msg_id_t id, msg_handler_fn hdlr, bool abort);

/**
 * @brief Initialize a CMT Message for 'EXEC'ing a specified handler (only).
 * @ingroup cmt
 *
 * This is a shortcut of using a cmt_init_... method specifying MSG_EXEC as the message with
 * a specific handler and setting the 'abort' flag true to only run the specified handler.
 *
 * @param msg Pointer to the Message to initialize
 * @param hdlr Message handler function that will be used first
 */
static inline void cmt_exec_init(cmt_msg_t* msg, msg_handler_fn exec_hdlr) {
    cmt_msg_init_ctrl(msg, MSG_EXEC, exec_hdlr, true);
}

/**
 * @brief Initialize a CMT Message with Normal Priority so that it is ready to be posted.
 * @ingroup cmt
 *
 * This initializes the message with the ID. It also NULLs out
 * the Handler Function pointer (correct for typical messages).
 *
 * @param msg Pointer to the Message to initialize
 * @param id Message ID
 */
static inline void cmt_msg_init(cmt_msg_t* msg, msg_id_t id) {
    msg->id = id;
    msg->hdlr = NULL_MSG_HDLR;
    msg->abort = false;
    msg->n = 0;
    msg->t = 0;
}

/**
 * @brief Initialize a CMT Message with a handler so that it is ready to be posted.
 * @ingroup cmt
 *
 * This initializes the message with the ID and a Handler Function. This handler will
 * be used first to handle the message when the message is pulled from the queue.
 * After the handler set on the message finishes processing, registered handlers will
 * be called unless processing is ended by clearing the `run_next_hdlr` flag.
 *
 * If it is desired that only the set Handler Function called, the `cmt_msg_init_ctrl`
 * method can be used to specify both the handler and the `run_next_hdlr` flag.
 *
 * @see cmt_msg_end_handling(cmt_msg_t* msg)
 * @see cmt_mdg_init_ctrl(cmt_msg_t* msg, bool run_next_hdlr)
 *
 * @param msg Pointer to the Message to initialize
 * @param id Message ID
 * @param hdlr Message handler function that will be used first
 */
static inline void cmt_msg_init2(cmt_msg_t* msg, msg_id_t id, msg_handler_fn hdlr) {
    msg->id = id;
    msg->hdlr = hdlr;
    msg->abort = false;
    msg->n = 0;
    msg->t = 0;
}

/**
 * @brief Initialize a CMT Message with a handler and control of the abort flag so
 * that it is ready to be posted.
 * @ingroup cmt
 *
 * This is the same as `cmt_msg_init2` with the addition of setting the `abort`
 * flag. Normally this flag is initialized to `false`, but this method allows setting it
 * initially to `true`. If the flag is true when the message is posted, only the handler
 * set on the message will be used for processing (even if other handlers are registered).
 *
 * @param msg Pointer to the Message to initialize
 * @param id Message ID
 * @param hdlr Message handler function that will be used first
 * @param abort False to continue with registered handlers. True to not look up additional handlers.
 */
static inline void cmt_msg_init_ctrl(cmt_msg_t* msg, msg_id_t id, msg_handler_fn hdlr, bool abort) {
    msg->id = id;
    msg->hdlr = hdlr;
    msg->abort = abort;
    msg->n = 0;
    msg->t = 0;
}

/**
 * @brief Remove the (forced) message handler set on a message.
 *
 * This should be used when re-posting a message that has been handled by a
 * specific (forced) message handler (set in `init`) so that the handler will be
 * looked up by the message processor.
 *
 * @param msg The message to remove the handler from
 */
static inline void cmt_msg_rm_set_hdlr(cmt_msg_t* msg) {
    msg->hdlr = NULL;
    msg->abort = false;
}

/**
 * @brief Indicate that no additional message handlers should be called to process
 * this message.
 *
 * This can be set before posting a message to limit the processing to only the
 * handler set on the message (which will always be used)
 *
 * @param msg
 */
static inline void cmt_msg_abort_handling(cmt_msg_t* msg) {
    msg->abort = true;
}



// Define functional names for the 'Core' message queue functions (Camel-case to help flag as aliases).
#define postHWRTMsg( pmsg )                     post_to_core0( pmsg )
#define postHWRTMsgDiscardable( pmsg )          post_to_core0_nowait( pmsg )
#define postDCSMsg( pmsg )                      post_to_core1( pmsg )
#define postDCSMsgDiscardable( pmsg )           post_to_core1_nowait( pmsg )
#define postHIDMsg( pmsg )                      post_to_core1( pmsg )
#define postHIDMsgDiscardable( pmsg )           post_to_core1_nowait( pmsg )
#define postBothMsgDiscardable( pmsg )          post_to_cores_nowait( pmsg )



#ifdef __cplusplus
    }
#endif
#endif // CMT_T_H_
