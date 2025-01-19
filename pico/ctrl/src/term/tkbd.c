/**
 * @brief Terminal Touch Keyboard functionality (very simple).
 * @ingroup terminal
 *
 * This implements a touch keyboard of rows. It provides Upper & Lower
 * alpha, Numbers, Punctuation, and Control Chars.
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */

#include "tkbd.h"

#include "display/display.h"


#define KB_LINES 5
#define KB_COLUMNS 30
#define KB_LAYOUT_ALPHA_OFF 1
#define KB_LAYOUT_CONTROLS_OFF 4
#define KB_LAYOUT_DIGITS_OFF 0
#define KB_LAYOUT_PUNCTUATION_OFF 0

#define TKBD_SPECIAL_KEY_FLAG 0x80;
#define TKBD_SPECIAL_KEY_INDX 0x7F;

// ############################################################################
// Internal Structures/Types
// ############################################################################
//

/**
 * @brief Structure for a non-alphanumeric 'special' key.
 */
typedef struct SPECIAL_KEY_ {
    const uint8_t key;
    const uint8_t cap;
} key_value_t;

typedef struct KEY_ROW_ {
    const uint16_t start_col;
    const key_value_t* keys;
    const int key_count;
} key_row_t;

typedef struct KEY_BANK_ {
    const uint16_t start_row;
    const key_row_t* rows;
    const int row_count;
} key_bank_t;


static const key_value_t _digit_kr1[] = { { '1' },{ '2' }, { '3' }, { '4' }, { '5' }, { '6' }, { '7' }, { '8' }, { '9' }, { '0' } };
static const key_row_t _digits_r1 = {
    0,
    _digit_kr1,
    10
};
static const key_row_t _digits_rows[] = {_digits_r1};
static const key_bank_t _digits_bank = {
    0,
    _digits_rows,
    1
};

static const key_value_t _alpha_lc_kr1[] = { {'q'}, {'w'}, {'e'}, {'r'}, {'t'}, {'y'}, {'u'}, {'i'}, {'o'}, {'p'} };
static const key_value_t _alpha_lc_kr2[] = { {'a'}, {'s'}, {'d'}, {'f'}, {'g'}, {'h'}, {'j'}, {'k'}, {'l'} };
static const key_value_t _alpha_lc_kr3[] = { {'z'}, {'x'}, {'c'}, {'v'}, {'b'}, {'n'}, {'m'}, {'-'} };
static const key_row_t _alpha_lc_r1 = {
    0,
    _alpha_lc_kr1,
    10,
};
static const key_row_t _alpha_lc_r2 = {
    1,
    _alpha_lc_kr2,
    9,
};
static const key_row_t _alpha_lc_r3 = {
    3,
    _alpha_lc_kr3,
    8,
};
static const key_row_t _alpha_lc_rows[] = {_alpha_lc_r1, _alpha_lc_r2, _alpha_lc_r3};
static const key_bank_t _alpha_lc_bank = {
    1,
    _alpha_lc_rows,
    3
};

static const key_value_t kv_none = { KSK_NONE };
//
static const key_value_t kv_bs = { KSK_BS, '\032' };
static const key_value_t kv_cr = { KSK_CR, '\034' };
static const key_value_t kv_control = { KSK_CTRL, '\030' };
static const key_value_t kv_punctuation = { KSK_PUNCTUATION, '\177' };
static const key_value_t kv_shift = { KSK_SHIFT, '\030' };
static const key_value_t kv_sp1 = { KSK_SP, '\024' }; // [
static const key_value_t kv_sp2 = { KSK_SP, ' ' }; // SP
static const key_value_t kv_sp3 = { KSK_SP, '\025' }; // ]

static const key_value_t _controls_kr1[] = {
        kv_shift,
        kv_punctuation,
        kv_control,
        {','},
        kv_sp1,
        kv_sp2,
        kv_sp3,
        {'.'},
        kv_bs,
        kv_cr
    };
static const key_row_t _controls_r1 = {
    0,
    _controls_kr1,
    10,
};
static const key_row_t _controls_rows[] = { _controls_r1 };
static const key_bank_t _controls_bank = {
    4,
    _controls_rows,
    1
};


static uint16_t _kb_line_top;
static uint16_t _kb_col_left;
static kbd_state_t _kb_state;
static kbd_substate_t _kb_substate;


// ############################################################################
// Internal Functions
// ############################################################################
//

static key_value_t _get_row_key(const key_row_t* kr, uint16_t col) {
    // Each key takes up three columns
    uint16_t acol = col - kr->start_col;
    if (acol >= 0) {
        uint16_t kcol = acol / 3;
        if (kcol < kr->key_count) {
            return (kr->keys[kcol]);
        }
    }
    return (kv_none);
}

static const key_value_t _get_key_value(uint16_t col, uint16_t row) {
    switch (_kb_state) {
        case KS_LETTERS_LC:
            // digits and lowercase letters
            if (row == 0) {
                return (_get_row_key(&_digits_r1, col));
            }
            else if (row < (_alpha_lc_bank.start_row + _alpha_lc_bank.row_count)) {
                return (_get_row_key(&_alpha_lc_bank.rows[row - _alpha_lc_bank.start_row], col));
            }
            else if (row < (_controls_bank.start_row + _controls_bank.row_count)) {
                return (_get_row_key(&_controls_bank.rows[row - _controls_bank.start_row], col));
            }
            break;
        default:
            break;
    }
    return (kv_none);
}

static void _kb_draw_bank(const key_bank_t* bank) {
    text_color_pair_t cp;
    disp_text_colors_get(&cp);
    disp_text_colors_set(C16_BLACK, C16_BLACK);
    for (uint16_t r = 0; r < bank->row_count; r++) {
        uint16_t arow = _kb_line_top + bank->start_row + r;
        key_row_t kr = bank->rows[r];
        char line[(3 * kr.key_count) + 1];
        disp_line_clear(arow, No_Paint);
        // Build up the line
        const key_value_t* kvs = kr.keys;
        for (uint16_t c = 0; c < kr.key_count; c++) {
            uint16_t i1, i2, i3;
            i1 = c * 3;
            i2 = i1 + 1;
            i3 = i1 + 2;
            key_value_t kv = kvs[c];
            uint8_t key = kv.key;
            uint8_t cap;
            if ((key & KBD_SPECIAL_KEY_FLAG) == 0) {
                cap = kv.key;
            }
            else {
                // Special key
                cap = kv.cap;
            }
            line[i1] = line[i3] = ' ';
            line[i2] = cap;
            line[i3+1] = '\000';
        }
        disp_string_color(arow, _kb_col_left + kr.start_col, line, C16_BLACK, C16_WHITE, Paint);
    }
    disp_text_colors_set(cp.fg, cp.bg);
}

static void _kb_draw_digits(void) {
    _kb_draw_bank(&_digits_bank);
}

static void _kb_draw_lcletters(void) {
    _kb_draw_bank(&_alpha_lc_bank);
}

static void _kb_draw_ucletters(void) {
    _kb_draw_bank(&_alpha_lc_bank);
}

static void _kb_draw_punctuation(void) {
    _kb_draw_bank(&_alpha_lc_bank);
}

/**
 * @brief Draw the 'Shift', 'Backspace', and bottom line (Punc/Alpha, comma, sp, period, enter)
 */
static void _kb_draw_controls(void) {
    _kb_draw_bank(&_controls_bank);
}


// ############################################################################
// Public Functions
// ############################################################################
//

kbd_state_t tkbd_state_get(void) {
    return (_kb_state);
}

void tkbd_state_set(kbd_state_t kbs) {
    _kb_state = kbs;
    tkbd_redraw();
}

kbd_substate_t tkbd_substate_get(void) {
    return (_kb_substate);
}

void tkbd_substate_set(kbd_substate_t kbss) {
    _kb_substate = kbss;
    tkbd_redraw();
}

uint8_t tkbd_get_csk(uint16_t col, uint16_t row) {
    // Check the row and column for inside of the keyboard
    if (row >= _kb_line_top && row < (_kb_line_top + KB_LINES) && col >= _kb_col_left && col < (_kb_col_left + KB_COLUMNS)) {
        // Get the key value
        const key_value_t kv = _get_key_value(col - _kb_col_left, row - _kb_line_top);
        return (kv.key);
    }
    return (kv_none.key);
}

void tkbd_redraw(void) {
    // Erase the keyboard area
    text_color_pair_t cp;
    disp_text_colors_get(&cp);
    disp_text_colors_set(C16_BLACK, C16_BLACK);
    uint16_t line = _kb_line_top;
    for (int i = 0; i < KB_LINES; i++) {
        disp_line_clear(line + i, Paint);
    }
    disp_text_colors_set(cp.fg, cp.bg);
    switch (_kb_state) {
    case KS_LETTERS_LC:
        _kb_draw_digits();
        _kb_draw_lcletters();
        break;
    case KS_LETTERS_UC:
        _kb_draw_digits();
        _kb_draw_ucletters();
        break;
    case KS_PUNCTUATION:
        _kb_draw_punctuation();
        break;
    }
    _kb_draw_controls();
}


// ############################################################################
// Initialization and Maintainence Functions
// ############################################################################
//

void tkbd_module_init(uint16_t start_row, uint16_t start_col, kbd_state_t state, kbd_substate_t substate) {
    _kb_line_top = start_row;
    _kb_col_left = start_col;
    _kb_state = state;
    _kb_substate = substate;

    tkbd_redraw();
}
