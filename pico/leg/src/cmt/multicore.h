/**
 * leg Multicore common.
 *
 * Containes the data structures and routines to handle multicore functionality.
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#ifndef _MK_MULTICORE_H_
#define _MK_MULTICORE_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "cmt.h"

/**
 * @file multicore.h
 * @defgroup multicore multicore
 * Common multicore structures, data, and functionality.
 *
 * NOTE: Fifo use
 * leg uses the Fifo for base (core (ha!)) level communication between
 * the cores for the supervisor/system functionality. The Pico SDK warns about
 * using the Fifo, as it is used by the Pico runtime to support multicore operation.
 *
 * leg acknowledges the warnings, and will take care to assure that the Fifo and
 * the leg code_seq is in an appropriate state when operations are performed that will
 * cause the Pico SDK/runtime to use them.
 *
 * For general purpose application communication between the functionality running on
 * the two cores queues will be used.
 *
 * @addtogroup multicore
 * @include multicore.c
 *
*/

/**
 * @brief Get a message for Core 0 (from the Core 0 queue). Block until a message can be read.
 *
 * @param msg Pointer to a buffer for the message.
 */
void get_core0_msg_blocking(cmt_msg_t *msg);

/**
 * @brief Get a message for Core 0 (from the Core 0 queue) if available, but don't wait for one.
 *
 * @param msg Pointer to a buffer for the message.
 * @return true If a message was retrieved.
 * @return false If no message was available.
 */
bool get_core0_msg_nowait(cmt_msg_t* msg);

/**
 * @brief Get a message for Core 1 (from the Core 1 queue). Block until a message can be read.
 *
 * @param msg Pointer to a buffer for the message.
 */
void get_core1_msg_blocking(cmt_msg_t* msg);

/**
 * @brief Get a message for Core 1 (from the Core 1 queue) if available, but don't wait.
 *
 * @param msg Pointer to a buffer for the message.
 * @return true If a message was retrieved.
 * @return false If no message was available.
 */
bool get_core1_msg_nowait(cmt_msg_t* msg);

/**
 * @brief Post a message by ID to Core 0 (using the Core 0 queue). Block until it can be posted.
 * @ingroup multicore
 *
 * Generally used for necessary operational information/instructions. This posts a message that
 * doesn't require message data using the message ID.
 *
 * @param msg_id The message ID of the message to post.
 * @param priority The priority of the message (NORMAL or HIGH)
 */
void post_by_id_to_core0_blocking(msg_id_t msg_id, msg_priority_t priority);

/**
 * @brief Post a message by ID to Core 1 (using the Core 1 queue). Block until it can be posted.
 * @ingroup multicore
 *
 * Generally used for necessary operational information/instructions. This posts a message that
 * doesn't require message data using the message ID.
 *
 * @param msg_id The message ID of the message to post.
 * @param priority The priority of the message (NORMAL or HIGH)
 */
void post_by_id_to_core1_blocking(msg_id_t msg_id, msg_priority_t priority);

/**
 * @brief Post a message by ID to Core 0 (using the Core 0 queue). Do not wait if it can't be posted.
 * @ingroup multicore
 *
 * Generally used for informational status. Especially information that
 * is updated on an ongoing basis. This posts a message that doesn't require message data
 * using the message ID.
 *
 * @param msg_id The message ID of the message to post.
 * @returns true if message was posted.
 */
bool post_by_id_to_core0_nowait(msg_id_t msg_id);

/**
 * @brief Post a message by ID to Core 1 (using the Core 1 queue). Do not wait if it can't be posted.
 * @ingroup multicore
 *
 * Generally used for informational status. Especially information that
 * is updated on an ongoing basis. This posts a message that doesn't require message data
 * using the message ID.
 *
 * @param msg_id The message ID of the message to post.
 * @returns true if message was posted.
 */
bool post_by_id_to_core1_nowait(msg_id_t msg_id);

/**
 * @brief Post a message by ID to both Core 0 and Core 1 (using the Core 0 and Core 1 queues).
 * @ingroup multicore
 *
 * Generally used for necessary operational information/instructions.
 *
 * @note Since this is posting the same message to both cores, it should not be used for messages
 *       that contain allocated resources, as both core's message handlers would try to free them.
 *
 * @param msg_id The message ID of the message to post.
 * @param priority The priority of the message (NORMAL or HIGH)
 */
void post_by_id_to_cores_blocking(msg_id_t msg_id, msg_priority_t priority);

/**
 * @brief Post a message by ID to both Core 0 and Core 1 (using the Core 0 and Core 1 queues). Do not
 *        wait if it can't be posted.
 * @ingroup multicore
 *
 * Generally used for informational status. Especially information that
 * is updated on an ongoing basis.
 *
 * @note Since this is posting the same message to both cores, it should not be used for messages
 *       that contain allocated resources, as both core's message handlers would try to free them.
 *
 * @param msg_id The message ID of the message to post.
 * @return 0 Could not post to either. 1 Posted to Core 0. 2 Posted to Core 1. 3 Posted to both cores.
 */
uint16_t post_by_id_to_cores_nowait(msg_id_t msg_id);

/**
 * @brief Post a message to Core 0 (using the Core 0 queue). Block until it can be posted.
 * @ingroup multicore
 *
 * Generally used for necessary operational information/instructions.
 *
 * @param msg The message to post.
 * @param priority The priority of the message (NORMAL or HIGH)
 */
void post_to_core0_blocking(const cmt_msg_t* msg, msg_priority_t priority);

/**
 * @brief Post a message to Core 0 (using the Core 0 queue). Do not wait if it can't be posted.
 * @ingroup multicore
 *
 * Generally used for informational status. Especially information that
 * is updated on an ongoing basis.
 *
 * @param msg The message to post.
 * @returns true if message was posted.
 */
bool post_to_core0_nowait(const cmt_msg_t* msg);

/**
 * @brief Post a message by ID to Core 1 (using the Core 1 queue). Block until it can be posted.
 * @ingroup multicore
 *
 * Generally used for necessary operational information/instructions. This posts a message that
 * doesn't require message data using the message ID.
 *
 * @param msg_id The message ID of the message to post.
 * @param priority The priority of the message (NORMAL or HIGH)
 */
void post_msg_to_core1_blocking(msg_id_t msg_id, msg_priority_t priority);

/**
 * @brief Post a message to Core 1 (using the Core 1 queue).
 * @ingroup multicore
 *
 * Generally used for necessary operational information/instructions.
 *
 * @param msg The message to post.
 * @param priority The priority of the message (NORMAL or HIGH)
 */
void post_to_core1_blocking(const cmt_msg_t* msg, msg_priority_t priority);

/**
 * @brief Post a message to Core 1 (using the Core 1 queue). Do not wait if it can't be posted.
 * @ingroup multicore
 *
 * Generally used for informational status. Especially information that
 * is updated on an ongoing basis.
 *
 * @param msg The message to post.
 * @returns true if message was posted.
 */
bool post_to_core1_nowait(const cmt_msg_t* msg);

/**
 * @brief Post a message to both Core 0 and Core 1 (using the Core 0 and Core 1 queues).
 * @ingroup multicore
 *
 * Generally used for necessary operational information/instructions.
 *
 * @note Since this is posting the same message to both cores, it should not be used for messages
 *       that contain allocated resources, as both core's message handlers would try to free them.
 *
 * @param msg The message to post.
 * @param priority The priority of the message (NORMAL or HIGH)
 */
void post_to_cores_blocking(const cmt_msg_t* msg, msg_priority_t priority);

/**
 * @brief Post a message to both Core 0 and Core 1 (using the Core 0 and Core 1 queues). Do not
 *        wait if it can't be posted.
 * @ingroup multicore
 *
 * Generally used for informational status. Especially information that
 * is updated on an ongoing basis.
 *
 * @note Since this is posting the same message to both cores, it should not be used for messages
 *       that contain allocated resources, as both core's message handlers would try to free them.
 *
 * @param msg The message to post.
 * @return 0 Could not post to either. 1 Posted to Core 0. 2 Posted to Core 1. 3 Posted to both cores.
 */
uint16_t post_to_cores_nowait(const cmt_msg_t* msg);

/**
 * @brief Initialize the multicore environment to be ready to run message loops on the two cores.
 * @ingroup multicore
 *
 * This gets everything ready, but it doesn't actually start up the message loops.
 */
void multicore_module_init();

#ifdef __cplusplus
    }
#endif
#endif // _MK_MULTICORE_H_
