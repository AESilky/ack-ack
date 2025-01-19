/**
 * @brief Terminal functionality (very simple).
 * @ingroup terminal
 *
 * This implements a simple interactive terminal. This can be used for
 * output from the board and for connection to a host for CLI operations.
 *
 * The terminal identifies itself as 'xterm' but it is far from an
 * actual xterm terminal.
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef TERMINAL_H_
#define TERMINAL_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "cmt/cmt.h"

#include "pico/types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// ############################################################################
// Structure and Enum Definitions and Function Prototypes
// ############################################################################
//


// ############################################################################
// Public Methods
// ############################################################################
//

/**
 * @brief Get a character from the terminal without blocking.
 * @ingroup term
 *
 * Once the terminal has been initialized, this should be used rather than
 * getchar/getchar_timeout_us as the terminal support installs a handler
 * and implements an input buffer.
 *
 * @see getchar()
 * @see getchar_timeout_us(us)
 * @see term_input_available()
 *
 * @return int The input character or -1 if no character is available.
 */
extern int term_getc(void);

/**
 * @brief Return status of available input.
 * @ingroup term
 *
 * @return true If there is input data available (in the input buffer).
 * @return false If there isn't currently input data available.
 */
extern bool term_input_available(void);

/**
 * @brief Clears the input buffer.
 * @ingroup term
 *
 */
extern void term_input_buf_clear(void);

/**
 * @brief Indicates if input data was lost (the buffer was full when input was received).
 * @ingroup term
 *
 * Once set, this status will remain true until this function is called, or the input buffer is cleared.
 *
 * @see term_input_but_clear()
 *
 * @return true Input was lost since the last time this was called.
 * @return false No input has been lost since the last time this was called.
 */
extern bool term_input_overflow(void);

/**
 * @brief Register a function to be called when input data becomes available.
 * @ingroup term
 *
 * The function will be called when input data becomes available from the terminal,
 * including if data is currently available.
 *
 * @param fn A CMT Message Handler function to be called, or `NULL` to remove a registered function.
 */
extern void term_register_notify_on_input(msg_handler_fn notify_fn);


// ====================================================================
// Initialization and Startup functions
// ====================================================================


/**
 * @brief Start the Interactive Terminal
 */
extern void term_start(void);

/**
 * @brief Initialize the Interactive Terminal Module
 */
extern void term_module_init(void);

#ifdef __cplusplus
}
#endif
#endif // TERMINAL_H_
