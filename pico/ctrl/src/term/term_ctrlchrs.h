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
 * Copyright 2023-24 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef TERMINAL_CTRLCHRS_H_
#define TERMINAL_CTRLCHRS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "pico/types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BS      '\010'  // Backspace
#define BEL     '\007'  // Bell/Alert
#define CBOL    '\r'    // Carrige Return
#define CR      '\r'    // Carrige Return
#define CSI     "\e["   // Control Sequence Introducer
#define DCS     "\eP"   // Device Control String
#define DEL     '\177'  // Delete
#define ENQ     '\005'  // ^E (ENQ)
#define ESC     '\e'    // Escape
#define IND     "\eD"   // Index
#define NEL     "\eE"   // Next Line
#define OSC     "\e]"   // Operating System Command
#define RI      "\eM"   // Reverse Index (UP 1)
#define SCS     "\e("   // Select Character Set (+0 Line Draw, +B ASCII)
#define SS3     "\eO"   // Single Shift 3
#define ST      "\e\\"  // String Terminator


/**
 * @brief Terminal text colors
 * @ingroup term
 */
typedef enum _TERM_CHR_COLOR_NUMS_ {
    TERM_CHR_COLOR_BLACK            =  0,
    TERM_CHR_COLOR_RED              =  1,
    TERM_CHR_COLOR_GREEN            =  2,
    TERM_CHR_COLOR_YELLOW           =  3,
    TERM_CHR_COLOR_BLUE             =  4,
    TERM_CHR_COLOR_MAGENTA          =  5,
    TERM_CHR_COLOR_CYAN             =  6,
    TERM_CHR_COLOR_WHITE            =  7,
    TERM_CHR_COLOR_GRAY             =  8,
    TERM_CHR_COLOR_BR_RED           =  9,
    TERM_CHR_COLOR_BR_GREEN         =  10,
    TERM_CHR_COLOR_BR_YELLOW        =  11,
    TERM_CHR_COLOR_BR_BLUE          =  12,
    TERM_CHR_COLOR_BR_MAGENTA       =  13,
    TERM_CHR_COLOR_BR_CYAN          =  14,
    TERM_CHR_COLOR_BR_WHITE         =  15,
} term_color_t;

/*
 * The following are from the Dec VT-510 programmer's manual.
 *
 * Setting Control Sequence             Final Chars     Mnemonic
 * ==================================== =============== ==========
 * Select Active Status Display         $ g             DECSASD
 * Select Attribute Change Extent       * x             DECSACE
 * Set Character Attribute              " q             DECSCA
 * Set Conformance Level                " p             DECSCL
 * Set Columns Per Page                 $ |             DECSCPP
 * Set Lines Per Page                   t               DECSLPP
 * Set Number of Lines per Screen       * |             DECSNLS
 * Set Status Line Type                 $ ~             DECSSDT
 * Set Left and Right Margins           s               DECSLRM
 * Set Top and Bottom Margins           r               DECSTBM
 * Set Graphic Rendition                m               SGR
 * Select Set-Up Language               p               DECSSL
 * Select Printer Type                  $ s             DECSPRTT
 * Select Refresh Rate                  " t             DECSRFR
 * Select Digital Printed Data Type     ) p             DECSDPT
 * Select ProPrinter Character Set      * p             DECSPPCS
 * Select Communication Speed           * r             DECSCS
 * Select Communication Port            * u             DECSCP
 * Set Scroll Speed                     SP p            DECSSCLS
 * Set Cursor Style                     SP q            DECSCUSR
 * Set Key Click Volume                 SP r            DECSKCV
 * Set Warning Bell Volume              SP t            DECSWBV
 * Set Margin Bell Volume               SP u            DECSMBV
 * Set Lock Key Style                   SP v            DECSLCK
 * Select Flow Control Type             * s             DECSFC
 * Select Disconnect Delay Time         $ q             DECSDDT
 * Set Transmit Rate Limit              " u             DECSTRL
 * Set Port Parameter                   + w             DECSPP
 *
 */


#ifdef __cplusplus
}
#endif
#endif // TERMINAL_CTRLCHRS_H_
