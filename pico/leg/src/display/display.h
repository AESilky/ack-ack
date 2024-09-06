/**
 * Copyright 2023-24 AESilky
 *
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef _DISPLAY_H_
#define _DISPLAY_H_
#ifdef __cplusplus
 extern "C" {
#endif

#include <stdbool.h>

/** @brief Bit to OR in to invert a character (display black char on white background) */
#define DISP_CHAR_INVERT_BIT 0x80
/** @brief Mask to AND with a character to remove intert (display white char on black background) */
#define DISP_CHAR_NORMAL_MASK 0x7F

/** @brief Text character data for the full text screen */
extern char disp_full_screen_text[];

/** @brief The number of character lines on the display. */
int disp_info_char_lines();

/** @brief The number of character columns on the display. */
int disp_info_char_cols();

/** @brief Clear the text screen
 *  \ingroup display
 *
 * Clear the current text content and the screen.
 *
 *  \param paint Set true to paint the actual display. Otherwise, only buffers will be cleared.
*/
void disp_clear(bool paint);

/** @brief Display a character on the text screen
 *  \ingroup display
 *
 * Display an ASCII character (plus some special characters).
 * If the top bit is set (c>127) the character is inverse (black on white background).
 *
 * @param row 0-5 With 0 being the top line
 * @param col 0-13 Starting column
 * @param c character to display
 * @param paint Set true to paint the actual display. Otherwise, only buffers will be updated.
 */
void disp_char(unsigned short int row, unsigned short int col, const char c, bool paint);

/** @brief Test the fonts by displaying all of the characters
 *  \ingroup display
 *
 * Display all of the font characters a page at a time. Pause between pages and overlap
 * the range of characters some from page to page.
 *
 */
void disp_font_test(void);

/** @brief Initialize the display
 *  \ingroup display
 *
 * This must be called before using the display.
 *
 */
void disp_module_init(void);

/** @brief Paint the actual display screen
 *  \ingroup display
 *
 * To improve performance and the look of the display, most changes can be made without
 * updating the physical display. Then, once a batch of changes have been made, this
 * is called to move the screen/image buffer onto the display.
 */
void disp_paint(void);

/** @brief Clear the character row.
 *  \ingroup display
 *
 *  \param row The 0-based row to clear.
 *  \param paint True to also paint the screen.
*/
void disp_row_clear(unsigned short int row, bool paint);

/** @brief Paint the portion of the screen containing the given character row.
 *  \ingroup display
 *
 *  This 'paints' the screen from the display buffer. To paint the buffer
 *  from the row data use `disp_row_refresh`.
 *
 *  \param row The 0-based character row to paint.
*/
void disp_row_paint(unsigned short int row);

/** @brief Scroll 2 or more rows up.
 *  \ingroup display
 *
 *  Scroll the character rows up, removing the top row and
 *  clearing the bottom row.
 *
 *  \param row_t The 0-based top row.
 *  \param row_b The 0-based bottom row.
 *  \param paint True to paint the screen after the operation.
*/
void disp_rows_scroll_up(unsigned short int row_t, unsigned short int row_b, bool paint);

/** @brief Display a string
 *  \ingroup display
 *
 * Display a string of ASCII characters (plus some special characters)
 *
 * @param row 1-6 With 1 being the top (status) line
 * @param col 1-14 Starting column
 * @param pString Pointer to the first character of a null-terminated string
 * @param invert True to invert the characters
 * @param paint True to paint the screen after the operation
 */
void disp_string(unsigned short int row, unsigned short int col, const char *pString, bool invert, bool paint);

/** @brief Update the display (graphics) buffer from the row data. Optionally paint the screen
 *  \ingroup display
 *
 *  \param paint True to paint the screen after the operation.
*/
void disp_update(bool paint);

#ifdef __cplusplus
}
#endif
#endif // _DISPLAY_H_

