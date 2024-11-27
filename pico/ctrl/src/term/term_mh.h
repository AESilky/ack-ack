/**
 * @brief Terminal functionality - Message Handlers.
 * @ingroup terminal
 *
 * This implements a simple interactive terminal. This can be used for
 * output from the board and for connection to a host for CLI operations.
 *
 * The terminal identifies itself as 'xterm' but it is far from an
 * actual xterm terminal.
 *
 * Copyright 2023-24 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef TERMINAL_MH_H_
#define TERMINAL_MH_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "cmt/cmt.h"

extern const msg_handler_entry_t term_switch_action_handler_entry;

extern const msg_handler_entry_t term_touch_handler_entry;

#ifdef __cplusplus
}
#endif
#endif // TERMINAL_H_
