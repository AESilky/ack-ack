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

typedef struct cmt_sm_counts_ {
    uint16_t total;
    uint16_t sleeps;
    uint16_t core0;
    uint16_t core1;
} cmt_sm_counts_t;

typedef struct _PROC_STATUS_ACCUM_ {
    volatile uint64_t ts_psa;                       // Timestamp of last PS Accumulator/sec update
    volatile uint64_t t_active;
    volatile uint32_t retrieved;
    volatile uint32_t interrupt_status;
    volatile msg_id_t msg_longest;
    volatile uint64_t t_msg_longest;
} proc_status_accum_t;

/**
 * @brief Add (register) a handler for a message.
 *
 * This adds (registers) a handler to be called when a message is being processed. The
 * handler is registered for the core calling this function.
 *
 * @see cmt_msg_hdlr_add_for_core to add a handler for both (or different) cores (Non-typical usage).
 *
 * @param id The message ID
 * @param hdlr The handler function
 */
extern void cmt_msg_hdlr_add(msg_id_t id, msg_handler_fn hdlr);

/**
 * @brief Add (register) a handler for a message for both (or different) cores.
 *
 * This adds (registers) a handler to be called when a message is being processed. The
 * handler is registered for the core specified.
 *
 * @see cmt_msg_hdlr_add to add a handler for the calling core (Typical usage).
 * @see MSG_HDLR_CORE_BOTH for the ID to use to specify both cores.
 *
 * @param id The message ID
 * @param hdlr The handler function
 * @param corenum The corenum number
 */
extern void cmt_msg_hdlr_add_for_core(msg_id_t id, msg_handler_fn hdlr, uint corenum);

/**
 * @brief Remove (un-register) a handler for a message.
 *
 * This removes (un-registers) the handler for a message for the core calling this function.
 *
 * @see cmt_msg_hdlr_rm_for_core to remove a handler for both (or different) cores (Non-typical)
 *
 * @param id The message ID
 * @param hdlr The handler function
 */
extern void cmt_msg_hdlr_rm(msg_id_t id, msg_handler_fn hdlr);

/**
 * @brief Remove (un-register) a handler for a message for both (or different) cores.
 *
 * This removes (un-registers) the handler for a message for the core calling this function.
 *
 * @see cmt_msg_hdlr_rm to remove a handler for the calling core (Typical)
 * @see MSG_HDLR_CORE_BOTH for the ID to use to specify both cores.
 *
 * @param id The message ID
 * @param hdlr The handler function
 * @param corenum The corenum number
 */
extern void cmt_msg_hdlr_rm_for_core(msg_id_t id, msg_handler_fn hdlr, uint corenum);

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
 * @brief Get the last Process Status Accumulator per second values.
 *
 * @param psas Pointer to Process Status Accumulator structure to fill with values.
 * @param corenum The core number (0|1) to get the process status values for.
 */
extern void cmt_proc_status_sec(proc_status_accum_t* psas, uint8_t corenum);

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
 * @brief Cancel scheduled message for a message ID.
 * @ingroup cmt
 *
 * This will attempt to cancel the scheduled message. It is possible that the time might have already
 * passed and the message was posted.
 *
 * If the message was scheduled with a handler specified, then `scheduled_msg_cancel2` must be used
 * to cancel it. That includes the handler. This method will not cancel a message that specified a
 * handler.
 *
 * @param sched_msg_id The ID of the message that was scheduled.
 * @return int32_t Number of milliseconds remaining.
 */
extern int32_t scheduled_msg_cancel(msg_id_t sched_msg_id);

/**
 * @brief Cancel scheduled message for a message ID and specified handler.
 * @ingroup cmt
 *
 * This is similar to `scheduled_msg_cancel` except that the scheduled message is also
 * checked for the specific handler.
 *
 * This is typically used to cancel a 'MSG_EXEC' that has a handler function specified,
 * as there could be multiple 'MSG_EXEC' messages scheduled, but a specific one needs
 * to be cancelled.
 *
 * @param sched_msg_id The ID of the message that was scheduled.
 * @param hdlr The handler function that was specified for the scheduled message.
 * @return int32_t Number of milliseconds remaining.
 */
extern int32_t scheduled_msg_cancel2(msg_id_t sched_msg_id, msg_handler_fn hdlr);

/**
 * @brief Indicate if a scheduled message exists.
 * @ingroup cmt
 *
 * Typically, this is used to keep from adding a scheduled message if one already exists.
 *
 * @param sched_msg_id The ID of the message to check for.
 * @return True if there is a scheduled message for the ID.
 */
extern bool scheduled_msg_exists(msg_id_t sched_msg_id);

/**
 * @brief The number of scheduled messages waiting.
 *
 * @return cmt_sm_counts_t Counts of the number of scheduled messages,
 *      including the number of sleeps and the number for each core.
 */
extern cmt_sm_counts_t scheduled_msgs_waiting();

/**
 * @brief Enter into a message processing loop.
 * @ingroup cmt
 *
 * Enter into a message processing loop using a loop context.
 * This function will not return.
 *
 * @param fstart Function to be called once the message loop is started (by posting a message).
 */
extern void message_loop(msg_handler_fn fstart);

/**
 * @brief Initialize the Cooperative Multi-Tasking system.
 * @ingroup cmt
 */
extern void cmt_module_init();

#ifdef __cplusplus
    }
#endif
#endif // _CMT_H_
