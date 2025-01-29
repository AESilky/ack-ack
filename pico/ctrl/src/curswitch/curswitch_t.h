/**
 * @brief Shared data types for Cursor Switches functionality.
 * @ingroup curswitch
 *
 * Copyright 2024 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef _CURSWITCH_T_H_
#define _CURSWITCH_T_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "system_defs.h"

#include <stdbool.h>

/**
 * @brief Switch IDs
 */
typedef enum _SWITCH_ID_ {
    /** VALUE to indicate no switch pressed */
    SW_NONE     = 0,
    SW_LEFT     = 1,
    SW_RIGHT    = 2,
    SW_UP       = 3,
    SW_DOWN     = 4,
    SW_HOME     = 5,
    SW_ENTER    = 6,
} switch_id_t;
//
#define SW_COUNT        6
#define SW_INDEX_OFFSET (-1) // The Banks are 1-based, so this adjusts to array indexes (don't use for NONE)

/** 
 * @brief The state of a switch
 * @ingroup curswitch
 */
typedef struct _SW_STATE_ {
    /** True if switch is pressed */
    bool pressed;
    /** Millisecond timestamp of last change */ 
    uint32_t ts_ms;
} sw_state_t;

/**
 * @brief Information for a switch action.
 * @ingroup curswitch
 */
typedef struct _sw_action_data_ {
    /** Switch ID */
    switch_id_t switch_id;
    /** Switch pressed. Otherwise, released. */
    bool pressed;
    /** Action is a 'repeat' */
    bool repeat;
} switch_action_data_t;


#ifdef __cplusplus
}
#endif
#endif // _CURSWITCH_H_