/**
 * @brief Display functionality.
 * @ingroup display
 *
 * This defines the display/screen functionality in a generic way. The
 * `display_xxx` files contain implementations that are specific to a
 * particular display/screen device.
 *
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef _DISPLAY_I_H_
#define _DISPLAY_I_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "display.h"
#include "display_rgb18/display_rgb18.h"
#include "fonts/font.h"

/**
 * @brief Context for a screen.
 * @ingroup display
 *
 * Defines an active screen.
 */
typedef struct scr_context_ {
    uint16_t cols;                      // Number of text columns
    uint16_t lines;                     // Number of text lines
    colorn16_t color_fg_default;        // Default foreground color
    colorn16_t color_bg_default;        // Default background color
    uint16_t fixed_area_top_size;       // The top fixed area line count
    uint16_t fixed_area_bottom_size;    // The bottom fixedarea line count
    uint16_t scroll_size;               // This is `lines` - `the fixed size`, but stored for performance
    uint16_t scroll_start;              // Start line of the scroll area in the text buffer
    scr_position_t cursor_pos;          // The cursor position
    bool show_cursor;                   // Cursor visibility control
    rgb18_t cursor_color;               // The color of the cursor when it's shown
    const font_info_t* font_info;       // Font info for the selected font
    uint8_t* full_screen_text;          // Buffer for a full screen of characters
    colorbyte_t* full_screen_color;     // Buffer for a full screen of colors
    bool* dirty_text_lines;             // bool array to track lines modified since paint
    rgb18_t *render_buf;                 // buffer large enough to render one line of characters into
} scr_context_t;

/**
 * @brief Test if there are screen contexts available on the stack.
 * @ingroup display
 *
 * @return true If screen contexts are available.
 * @return false If no contexts are available.
 */
bool _has_scr_context();

/**
 * @brief Peek the top screen context.
 * @ingroup display
 *
 * @return scr_context_t* The top context or NULL if none exist.
 */
scr_context_t* _peek_scr_context();

/**
 * @brief Pop the top screen context from the stack.
 * @ingroup display
 *
 * @return scr_context_t* The top context or NULL if none exist.
 */
scr_context_t* _pop_scr_context();

/**
 * @brief Push a screen context on the stack.
 * @ingroup display
 *
 * @param sc The screen context to push.
 * @return true If the context could be pushed.
 * @return false If the stack is full.
 */
bool _push_scr_context(scr_context_t* sc);

#ifdef __cplusplus
}
#endif
#endif // _DISPLAY_I_H_

