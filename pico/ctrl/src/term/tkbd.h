/**
 * @brief Terminal Touch Keyboard functionality (very simple).
 * @ingroup terminal
 *
 * This implements a touch keyboard of rows. It provides Upper & Lower
 * alpha, Numbers, Punctuation, and Control Chars.
 *
 * The 'keys' are drawn in rows with each key taking one row height and
 * three columns in width. It requires 5 rows and 30 columns. It is placed
 * on the screen using parameters passed in the init methods.
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef TKBD_H_
#define TKBD_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/** @brief Number of lines in the full keyboard */
#define KB_LINES 5

#define KBD_SPECIAL_KEY_FLAG 0x80
typedef enum KBD_SPECIAL_KEY_ {
    // 0x00 - 0x7F reserved for normal characters.
    KSK_NONE = (KBD_SPECIAL_KEY_FLAG | 0),
    KSK_BS = (KBD_SPECIAL_KEY_FLAG | 1),
    KSK_CR = (KBD_SPECIAL_KEY_FLAG | 2),
    KSK_CTRL = (KBD_SPECIAL_KEY_FLAG | 3),
    KSK_PUNCTUATION = (KBD_SPECIAL_KEY_FLAG | 4),
    KSK_SHIFT = (KBD_SPECIAL_KEY_FLAG | 5),
    KSK_SP = (KBD_SPECIAL_KEY_FLAG | 6),
} kbd_special_key_t;

typedef enum KBD_STATE_ {
    KS_LETTERS_LC = 0,
    KS_LETTERS_UC,
    KS_PUNCTUATION,
} kbd_state_t;

typedef enum KBD_SUBSTATE_ {
    KSS_NORMAL = 0,
    KSS_SHIFT,
    KSS_CONTROL,
} kbd_substate_t;


/**
 * @brief Get the current keyboard state.
 * @ingroup term
 *
 * @return kbd_state_t Current state
 */
extern kbd_state_t tkbd_state_get(void);

/**
 * @brief Set the current keyboard state.
 * @ingroup term
 *
 * This changes the state and redraws the keyboard if the state changed.
 *
 * @param state The state
 */
extern void tkbd_state_set(kbd_state_t state);

/**
 * @brief Get the current keyboard sub-state.
 * @ingroup term
 *
 * @return kbd_substate_t Current sub-state
 */
extern kbd_substate_t tkbd_substate_get(void);

/**
 * @brief Set the current keyboard sub-state.
 * @ingroup term
 *
 * This changes the sub-state.
 *
 * @param substate The sub-state
 */
extern void tkbd_substate_set(kbd_substate_t substate);

/**
 * @brief Get the character or special key value for the row and column.
 * @ingroup term
 *
 * Using the row and column and accounting for the sub-state (control, caps-lock)
 * this returns either the character (ASCII) or a special-key value. Special keys
 * are indicated by having the upper-bit set.
 *
 * If the row or column are outside of the keyboard area, the special key `NONE`
 * is returned.
 *
 * @param col The absolute column value (can be outside of the keyboard)
 * @param row The absolute row value (can be outside of the keyboard)
 * @return uint_t The character or special key
 */
extern uint8_t tkbd_get_csk(uint16_t col, uint16_t row);

extern void tkbd_redraw(void);

extern void tkbd_module_init(uint16_t start_row, uint16_t start_col, kbd_state_t state, kbd_substate_t substate);

#ifdef __cplusplus
}
#endif
#endif // TKBD_H_
