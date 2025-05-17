/**
 * hwctrl Cooperative Multi-Tasking.
 *
 * Contains message loop, timer, and other CMT enablement functions.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#ifndef _CMT_H_
#define _CMT_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "cmt_t.h"

#include "multicore.h"


// Define functional names for the 'Core' message queue functions (Camel-case to help flag as aliases).
#define postHWCtrlMsg( pmsg )                   post_to_core0( pmsg )
#define postHWCtrlMsgDiscardable( pmsg )        post_to_core0_nowait( pmsg )
#define postDCSMsg( pmsg )                      post_to_core1( pmsg )
#define postDCSMsgDiscardable( pmsg )           post_to_core1_nowait( pmsg )
#define postBothMsgDiscardable( pmsg )          post_to_cores_nowait( pmsg )

typedef struct _PROC_STATUS_ACCUM_ {
    volatile uint64_t ts_psa;                       // Timestamp of last PS Accumulator/sec update
    volatile uint64_t t_active;
    volatile uint32_t retrieved;
    volatile uint32_t interrupt_status;
    volatile msg_id_t msg_longest;
    volatile uint64_t t_msg_longest;
} proc_status_accum_t;

typedef struct _MSG_LOOP_CNTX {
    uint8_t corenum;                                // The core number the loop is running on
    const msg_handler_entry_t** handler_entries;    // NULL terminated list of message handler entries
} msg_loop_cntx_t;

/**
 * @brief Initialize a CMT Message with Normal Priority so that it is ready to be posted.
 * @ingroup cmt
 *
 * This initializes the message with the ID and Normal Priority. It also NULLs out
 * the Handler Function pointer (correct for typical messages).
 *
 * @param msg Pointer to the Message to initialize
 * @param id Message ID
 */
extern void cmt_msg_init(cmt_msg_t* msg, msg_id_t id);

/**
 * @brief Initialize a CMT Message with a priority so that it is ready to be posted.
 * @ingroup cmt
 *
 * This initializes the message with the ID and Priority. It also NULLs out
 * the Handler Function pointer (correct for typical messages).
 *
 * @param msg Pointer to the Message to initialize
 * @param id Message ID
 * @param priority Priority
 */
extern void cmt_msg_init2(cmt_msg_t* msg, msg_id_t id, msg_priority_t priority);

/**
 * @brief Initialize a CMT Message with a priority so that it is ready to be posted.
 * @ingroup cmt
 *
 * This initializes the message with the ID and Priority, and also a Handler Function pointer.
 *
 * @param msg Pointer to the Message to initialize
 * @param id Message ID
 * @param priority Priority
 * @param hdlr Message handler function that will be used, rather than looking one up
 */
extern void cmt_msg_init3(cmt_msg_t* msg, msg_id_t id, msg_priority_t priority, msg_handler_fn hdlr);

/**
 * @brief Remove the (forced) message handler set on a message.
 *
 * This should be used when re-posting a message that has been handled by a
 * specific message handler (set in `init`) so that the handler will be
 * looked up by the message processor.
 *
 * @param msg The message to remove the handler from
 */
extern void cmt_msg_rm_hdlr(cmt_msg_t* msg);

/**
 * @brief Indicates if the Core-0 message loop has been started.
 * @ingroup cmt
 *
 * @return true The Core-0 message loop has been started.
 * @return false The Core-0 message loop has not been started yet.
 */
extern bool cmt_message_loop_0_running();

/**
 * @brief Indicates if the Core-1 message loop has been started.
 * @ingroup cmt
 *
 * @return true The Core-1 message loop has been started.
 * @return false The Core-1 message loop has not been started yet.
 */
extern bool cmt_message_loop_1_running();

/**
 * @brief Indicates if both the Core-0 and Core-1 message loops have been started.
 * @ingroup cmt
 *
 * @return true The message loops have been started.
 * @return false The message loops have not been started yet.
 */
extern bool cmt_message_loops_running();

/**
 * @brief Handle a Scheduled Message timer Tick.
 *
 * @param msg (not used)
 */
extern void cmt_handle_sleep(cmt_msg_t* msg);

/**
 * @brief Get the last Process Status Accumulator per second values.
 *
 * @param psas Pointer to Process Status Accumulator structure to fill with values.
 * @param corenum The core number (0|1) to get the process status values for.
 */
extern void cmt_proc_status_sec(proc_status_accum_t* psas, uint8_t corenum);

/**
 * @brief The number of scheduled messages waiting.
 *
 * @return int Number of scheduled messages.
 */
extern int cmt_sched_msg_waiting();

/**
 * @brief Get the ID's of the scheduled messages waiting.
 *
 * @param max The maximum number of ID's to return
 * @param buf Buffer (of uint16's) to hold the values
 *
 * @return True is any messages are waiting
 */
extern bool cmt_sched_msg_waiting_ids(int max, uint16_t *buf);

/**
 * @brief Sleep for milliseconds and call a function.
 * @ingroup cmt
 *
 * @param ms The time in milliseconds from now.
 * @param sleep_fn The function to call when the time expires.
 * @param user_data A pointer to user data that the 'sleep_fn' will be called with.
 */
extern void cmt_sleep_ms(int32_t ms, cmt_sleep_fn sleep_fn, void* user_data);

/**
 * @brief Schedule a message to post to Core-0 in the future.
 *
 * Use this when it is needed to future post to a core other than the one currently
 * being run on.
 *
 * @param ms The time in milliseconds from now.
 * @param msg The cmt_msg_t message to post when the time period elapses.
 */
extern void schedule_core0_msg_in_ms(int32_t ms, const cmt_msg_t* msg);

/**
 * @brief Schedule a message to post to Core-1 in the future.
 *
 * Use this when it is needed to future post to a core other than the one currently
 * being run on.
 *
 * @param ms The time in milliseconds from now.
 * @param msg The cmt_msg_t message to post when the time period elapses.
 */
extern void schedule_core1_msg_in_ms(int32_t ms, const cmt_msg_t* msg);

/**
 * @brief Schedule a message to post in the future.
 *
 * @param ms The time in milliseconds from now.
 * @param msg The cmt_msg_t message to post when the time period elapses.
 */
extern void schedule_msg_in_ms(int32_t ms, const cmt_msg_t* msg);

/**
 * @brief Cancel scheduled message(s) for a message ID.
 * @ingroup cmt
 *
 * This will attempt to cancel the scheduled message. It is possible that the time might have already
 * passed and the message was posted.
 *
 * @param sched_msg_id The ID of the message that was scheduled.
 */
extern void scheduled_msg_cancel(msg_id_t sched_msg_id);

/**
 * @brief Indicate if a scheduled message exists.
 * @ingroup cmt
 *
 * Typically, this is used to keep from adding a scheduled message if one already exists.
 *
 * @param sched_msg_id The ID of the message to check for.
 * @return True if there is a scheduled message for the ID.
 */
extern bool scheduled_message_exists(msg_id_t sched_msg_id);

/**
 * @brief Enter into a message processing loop.
 * @ingroup cmt
 *
 * Enter into a message processing loop using a loop context.
 * This function will not return.
 *
 * @param loop_context Loop context for processing.
 * @param fstart Function to be called once the message loop is started.
 */
extern void message_loop(const msg_loop_cntx_t* loop_context, start_fn fstart);

/**
 * @brief Initialize the Cooperative Multi-Tasking system.
 * @ingroup cmt
 */
extern void cmt_module_init();

#ifdef __cplusplus
    }
#endif
#endif // _CMT_H_
