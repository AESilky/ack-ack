/**
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FONT_H_
#define _FONT_H_
#ifdef __cplusplus
extern "C" {
#endif


#include <stdbool.h>
#include <stdint.h>

// 'Special' characters

#define SPACE_CHR                       0x20u   // \040

#define MU_CHR                          0x00u   // \000
#define WIFI_NOT_CONNECTED_CHR          0x01u   // \001
#define WIFI_CONNECTED_CHR              0x02U   // \002
#define DISK_WRITE_CHR                  0x03u   // \003
#define CHKBOX_UNCHECKED_CHR            0x04u   // \004
#define CHKBOX_CHECKED_CHR              0x05u   // \005
#define RADIO_BTN_NOT_SELECTED_CHR      0x06u   // \006
#define RADIO_BTN_SELECTED_CHR          0x07u   // \007
#define CHECK_CHR                       0x08u   // \010
#define TRASH_CAN_CHR                   0x09u   // \011
#define GEAR_LG_L_CHR                   0x0Au   // \012
#define GEAR_LG_R_CHR                   0x0Bu   // \013
#define MENU_LG_L_CHR                   0x0Cu   // \014
#define MENU_LG_R_CHR                   0x0Du   // \015
#define LOOP_CLOSED_CHR                 0x0Eu   // \016
#define LOOP_OPEN_CHR                   0x0Fu   // \017
#define CLOSER_CLOSED_LG_L_CHR          0x10u   // \020
#define CLOSER_CLOSED_LG_R_CHR          0x11u   // \021
#define CLOSER_OPEN_LG_L_CHR            0x12u   // \022
#define CLOSER_OPEN_LG_R_CHR            0x13u   // \023
#define CONNECTED_NOT_LG_L_CHR          0x14u   // \024
#define CONNECTED_NOT_LG_R_CHR          0x15u   // \025
#define CONNECTED_LG_L_CHR              0x16u   // \026
#define CONNECTED_LG_R_CHR              0x17u   // \027
#define ARROW_UP_CHR                    0x18u   // \030
#define ARROW_DOWN_CHR                  0x19u   // \031
#define ARROW_LEFT_CHR                  0x1Au   // \032
#define ARROW_RIGHT_CHR                 0x1Bu   // \033
#define BLANK1_CHR                      0x1Cu   // \034
#define BLANK2_CHR                      0x1Du   // \035
#define BLANK3_CHR                      0x1Eu   // \036
#define BLANK4_CHR                      0x1Fu   // \037

#define PARAGRAPH_CHR                   0x7Fu   // \177

/**
 * @brief Information about a font.
 * @ingroup display
 */
typedef struct font_info_ {
    const char *name;
    const int8_t width;
    const int8_t height;
    const int8_t bytes_per_glyph_line;
    const int8_t suggested_cursor_line;
    const uint32_t bitmask;
    const bool has_lowercase;
    const uint8_t *glyphs;
} font_info_t;

#ifdef __cplusplus
}
#endif
#endif // _FONT_H
