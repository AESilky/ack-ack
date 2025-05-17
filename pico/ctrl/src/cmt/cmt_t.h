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


typedef enum MSG_PRI_ {
    MSG_PRI_NORM = 0,
    MSG_PRI_L9,
    MSG_PRI_LP
} msg_priority_t;


typedef enum MSG_ID_ {
    // Common messages (used by both HWOS and DCS/HID)
    MSG_COMMON_NOOP = 0x0000,
    MSG_EXEC,               // General purpose message to use when specifying a handler.
    MSG_CONFIG_CHANGED,
    MSG_CMT_SLEEP,
    MSG_DEBUG_CHANGED,
    MSG_HOUSEKEEPING_RT,    // Housekeeping Repeating - Every 16ms (62.5Hz)
    MSG_INPUT_SW_PRESS,
    MSG_INPUT_SW_RELEASE,
    MSG_SENSBANK_CHG,
    MSG_SWITCH_ACTION,
    MSG_SWITCH_LONGPRESS,
    MSG_TERM_CHAR_RCVD,
    //
    // Hardware-OS (HWOS) messages
    MSG_HWOS_NOOP = 0x0100,
    MSG_HWOS_TEST,
    MSG_INPUT_SW_DEBOUNCE,
    MSG_MAIN_USER_SWITCH_PRESS,
    MSG_RC_DETECTING,   // Radio Control BAUD & Protocol being detected
    MSG_RC_DETECT_DA,   // Radio Control Detect - Data Available
    MSG_RC_DETECTED,    // Radio Control BAUD & Protocol detected
    MSG_RC_RX_ERR,      // Radio Control receiver RX error (Parity +/ Framing)
    MSG_RC_RX_MSG_RDY,  // Radio Control receiver message is ready
    MSG_ROTARY_CHG,
    MSG_SERVO_DATA_RCVD,
    MSG_SERVO_DATA_RX_TO,
    MSG_SERVO_READ_ERROR,
    MSG_SERVO_STATUS_RCVD,
    MSG_STDIO_CHAR_READY,
    MSG_SW_LONGPRESS_DELAY,
    MSG_TOUCH_PANEL,
    MSG_DCS_STARTED,
    //
    // Drive Control System (DCS) and Human Interface Devices (HID) messages
    MSG_DCS_NOOP = 0x0200,
    MSG_DCS_TEST,
    MSG_HWOS_STARTED,
    MSG_DISPLAY_MESSAGE,
} msg_id_t;

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
    int16_t rotary_delta;
    int32_t status;
    char* str;
    rcrx_bp_t rcrx_bp;
    sensbank_chg_t sensbank_chg;
    servo_params_t servo_params;
    switch_action_data_t sw_action;
    uint32_t ts_ms;
    uint64_t ts_us;
};
typedef union MSG_DATA_VALUE_ msg_data_value_t;

/**
 * @brief Structure containing a message ID and message data.
 *
 * @param id The ID (number) of the message.
 * @param data The data for the message.
 * @param priority The message priority.
 * @param hdlr A Handler function to use rather then the one registered (or null).
 * @param n The message number (set by the posting system)
 * @param t The millisecond time msg was posted (set by the posting system)
 */
typedef struct CMT_MSG_ {
    msg_id_t id;
    msg_priority_t priority;
    msg_data_value_t data;
    msg_handler_fn hdlr;
    int32_t n;
    uint32_t t;
} cmt_msg_t;

/**
 * @brief Function prototype for the start function.
 * @ingroup cmt
 */
typedef void (*start_fn)(void);

/**
 * @brief Message handler entry. Used in the message handler list.
 */
typedef struct _MSG_HANDLER_ENTRY {
    int msg_id;
    msg_handler_fn msg_handler;
} msg_handler_entry_t;


#ifdef __cplusplus
    }
#endif
#endif // CMT_T_H_
