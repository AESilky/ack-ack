/**
 * @brief Test, demo, and debugging routines.
 * @ingroup test
 *
 * Many of these aren't actually tests. Rather, they tend to be routines
 * that display patterns, send things to the terminal, get rotary control
 * values, etc., that can be helpful for seeing, demo'ing, and debugging
 * functionality.
 *
 * Copyright 2023-24 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef _TEST_H_
#define _TEST_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Test creating a config structure and then free'ing it.
 * @ingroup test
 */
int test_config_new_free();

/**
 * @brief Test printing an error to the terminal.
 * @ingroup test
 */
void test_error_printf();

/**
 * @brief Use `strdatetime` to format a datetime_t a number of different ways and print them.
 * @ingroup test
 *
 */
void test_strdatetime();

/**
 * @brief Register a `term_notify_on_input` function and read input when called.
 * @ingroup test
 *
 * @param timeout Maximum time in milliseconds to wait for input.
 *
 * @return char The character read or '\000' if nothing was read within `n` seconds.
 */
char test_term_notify_on_input(uint32_t timeout);

/**
 * @brief Set a top fixed area and a middle scroll area, leaving a bottom fixed area.
 *        Put some text in the top and bottom by positioning the cursor, then scroll
 *        some lines of text.
 * @ingroup test
 *
 */
void test_term_scroll_area();

/**
 * @brief Set the screen & page size to 132 x 48 and print a diagonal.
 * @ingroup test
 *
 */
void test_term_screen_page_size();

#ifdef __cplusplus
}
#endif
#endif // _TEST_H_
