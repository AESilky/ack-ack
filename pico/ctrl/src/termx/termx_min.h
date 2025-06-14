/*
    Extremely minimal XTERM output functionality.

    This is a header-only functionality that performs minimal text color operations
    to a VT-100/XTERM output terminal.

    Copyright 2025 AESilky (SilkyDESIGN)
    SPDX-License-Identifier: MIT
*/
#ifndef TERMX_MIN_H_
#define TERMX_MIN_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#define TERMX_CSI     "\e["   // Control Sequence Introducer
typedef enum _TERMX_CHR_COLOR_NUMS_ {
    TERMX_CHR_COLOR_BLACK            =  0,
    TERMX_CHR_COLOR_RED              =  1,
    TERMX_CHR_COLOR_GREEN            =  2,
    TERMX_CHR_COLOR_YELLOW           =  3,
    TERMX_CHR_COLOR_BLUE             =  4,
    TERMX_CHR_COLOR_MAGENTA          =  5,
    TERMX_CHR_COLOR_CYAN             =  6,
    TERMX_CHR_COLOR_WHITE            =  7,
    TERMX_CHR_COLOR_GRAY             =  8,
    TERMX_CHR_COLOR_BR_RED           =  9,
    TERMX_CHR_COLOR_BR_GREEN         =  10,
    TERMX_CHR_COLOR_BR_YELLOW        =  11,
    TERMX_CHR_COLOR_BR_BLUE          =  12,
    TERMX_CHR_COLOR_BR_MAGENTA       =  13,
    TERMX_CHR_COLOR_BR_CYAN          =  14,
    TERMX_CHR_COLOR_BR_WHITE         =  15,
} termx_color_t;

#define TERMX_START_BLUE_STR "\e[38;5;4m"
#define TERMX_START_GREEN_STR "\e[38;5;2m"
#define TERMX_START_RED_STR "\e[38;5;1m"
#define TERMX_START_WHITE_STR "\e[38;5;15m"
#define TERMX_DEFAULT_COLOR_STR "\e[39;49m"

static inline void termx_color_default() {
    printf("%s39;49m", TERMX_CSI);
}

static inline void termx_color_bg(termx_color_t colorn) {
    printf("%s48;5;%dm", TERMX_CSI, colorn);
}

static inline void termx_color_fg(termx_color_t colorn) {
    printf("%s38;5;%dm", TERMX_CSI, colorn);
}


#ifdef __cplusplus
    }  // extern "C"
#endif
#endif // TERMX_MIN_H_
