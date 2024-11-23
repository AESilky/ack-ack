/**
 * Copyright 2023-24 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#include "pico/stdlib.h"
#include "pico/malloc.h"
#include <malloc.h>

#include "system_defs.h"
#include "display_rgb18.h"
#include "../display_i.h"
#include "../fonts/font.h"
#include "../fonts/font_10_16.h"
#include "ili_lcd_spi.h"
#include "board.h"
#include "debug_support.h"
#include "string.h"

// RGB-18 Color Constants that are used for the Color16 values.
//
const rgb18_t RGB18_BLACK = { 0x00,0x00,0x00 };      //  0 :   0,   0,   0
const rgb18_t RGB18_BLUE = { 0x00,0x00,0xA8 };       //  1 :   0,   0, 170
const rgb18_t RGB18_GREEN = { 0x38,0x44,0x00 };      //  2 :  78, 145,   0
const rgb18_t RGB18_CYAN = { 0x00,0xE8,0xFC };       //  3 :   0, 250, 255
const rgb18_t RGB18_RED = { 0xA8,0x00,0x00 };        //  4 : 170,   0,   0
const rgb18_t RGB18_MAGENTA = { 0xFC,0x00,0xFC };    //  5 : 255,  64, 255
const rgb18_t RGB18_BROWN = { 0x90,0x44,0x00 };      //  6 : 100,  17,   0
const rgb18_t RGB18_WHITE = { 0x40,0x40,0x40 };      //  7 : 208, 208, 208
const rgb18_t RGB18_GREY = { 0xE4,0xE4,0x68 };       //  8 : 121, 121,  90
const rgb18_t RGB18_LT_BLUE = { 0x00,0x90,0xFC };    //  9 :   0, 100, 255
const rgb18_t RGB18_LT_GREEN = { 0x00,0xE4,0x00 };   // 10 :   0, 249,   0
const rgb18_t RGB18_LT_CYAN = { 0xCC,0xF4,0xFC };    // 11 : 115, 253, 255
const rgb18_t RGB18_ORANGE = { 0xFC,0x2C,0x08 };     // 12 : 255,  75,   2
const rgb18_t RGB18_LT_MAGENTA = { 0xFC,0x28,0x60 }; // 13 : 255, 138, 218
const rgb18_t RGB18_YELLOW = { 0xFC,0x4C,0x00 };     // 14 : 255, 147,   0
const rgb18_t RGB18_BR_WHITE = { 0xFC,0xFC,0xFC };      // 15 : 255, 255, 255

static void _disp_char(uint16_t aline, uint16_t col, char c, paint_control_t paint);
static void _disp_char_colorbyte(uint16_t aline, uint16_t col, char c, uint8_t color, paint_control_t paint);
static void _disp_line_clear(uint16_t aline, paint_control_t paint);
static void _disp_line_paint(uint16_t aline);
static uint16_t _translate_cursor_line(uint16_t curline);
static uint16_t _translate_line(uint16_t line);

/*! @brief Map of RGB-18 values indexed by Color16 numbers. */
static rgb18_t _color16_map[16];

// /** @brief Map of Color24 (RGB) values indexed by Color16 numbers. */
// static const rgb16_t _color16_map[] = {
//     ILI_BLACK;
//     ILI_BLUE;
//     ILI_GREEN;
//     ILI_CYAN;
//     ILI_RED;
//     ILI_MAGENTA;
//     ILI_BROWN;
//     ILI_WHITE;
//     ILI_GREY;
//     ILI_LT_BLUE;
//     ILI_LT_GREEN;
//     ILI_LT_CYAN;
//     ILI_ORANGE;
//     ILI_LT_MAGENTA;
//     ILI_YELLOW;
//     ILI_BR_WHITE
// };

/** @brief Flag indicating if the display is available and ready. */
static bool _display_ready = false;

/** @brief The current/active screen context */
static scr_context_t* _scr_ctx = NULL;

/** @brief The number of characters to scan (back) looking for a wrap break-point character */
static uint16_t _wrap_len;

// ======================================================================================
// Internal functions
// ======================================================================================

/**
 * NOTE: This does not perform text line translation, nor bounds check.
 */
static void _disp_char(uint16_t line, uint16_t col, char c, paint_control_t paint) {
    _disp_char_colorbyte(line, col, c, colorbyte(_scr_ctx->color_fg_default, _scr_ctx->color_bg_default), paint);
}

/**
 * NOTE: This does not perform text line translation, nor bounds check.
 */
static uint8_t _disp_char_color_get(uint16_t aline, uint16_t col) {
    return (*(_scr_ctx->full_screen_color + (aline * _scr_ctx->cols) + col));
}

/**
 * NOTE: This does not perform text line translation, nor bounds check.
 */
static char _disp_char_get(uint16_t aline, uint16_t col) {
    return (*(_scr_ctx->full_screen_text + (aline * _scr_ctx->cols) + col));
}


/*
 * Display an ASCII character (plus some special characters)
 * If the top bit is set (c>127) the character is inverse (black on white background).
 *
 * Getting the glyph data for the font character is similar to `disp_line_paint`;
 * but the difference is that here we are rendering one single complete charcter.
 *
 * NOTE: This does not perform text line translation, nor bounds check.
 *
 */
static void _disp_char_colorbyte(uint16_t aline, uint16_t col, char c, uint8_t color, paint_control_t paint) {
    *(_scr_ctx->full_screen_text + (aline * _scr_ctx->cols) + col) = c;
    *(_scr_ctx->full_screen_color + (aline * _scr_ctx->cols) + col) = color;
    if (paint) {
        // Actually render the characher glyph onto the screen.
        bool invert = c & DISP_CHAR_INVERT_BIT;
        unsigned char cl = c & 0x7F;
        uint8_t fg = (invert ? bg_from_cb(color) : fg_from_cb(color));
        uint8_t bg = (invert ? fg_from_cb(color) : bg_from_cb(color));
        rgb18_t fgrgb = rgb18_from_color16(fg);
        rgb18_t bgrgb = rgb18_from_color16(bg);
        const font_info_t* fi = _scr_ctx->font_info;
        int8_t font_height = fi->height;
        int8_t font_width = fi->width;
        int8_t bpgl = fi->bytes_per_glyph_line;
        rgb18_t* rbuf = _scr_ctx->render_buf;
        uint16_t glyphindex = (cl * font_height * bpgl);
        // See if we need to show the cursor
        bool show_a_cursor = (_scr_ctx->show_cursor && col == _scr_ctx->cursor_pos.column && aline == _translate_cursor_line(_scr_ctx->cursor_pos.line));
        rgb18_t cursor_color = _scr_ctx->cursor_color;
        for (int glyph_line = 0; glyph_line < font_height; glyph_line++) {
            if (show_a_cursor && glyph_line == _scr_ctx->font_info->suggested_cursor_line) {
                for (int c = 0; c < font_width; c++) {
                    *rbuf++ = cursor_color;
                }
            }
            else {
                // Get each glyph row for the character (each line of the font char height).
                uint32_t cgr = 0;
                uint32_t mask = 1u << (font_width - 1u);
                for (int byte = 0; byte < bpgl; byte++) {
                    cgr |= (fi->glyphs[glyphindex + byte + (glyph_line * bpgl)]) << (8u * byte);
                }
                for (int c = 0; c < font_width; c++) {
                    // For each glyph column bit that is set, set the forground color in the character buffer.
                    if (cgr & mask) {
                        *rbuf = fgrgb;
                    }
                    else {
                        *rbuf = bgrgb;
                    }
                    mask >>= 1u;
                    rbuf++;
                }
            }
        }
        // Set a window into the right part of the screen
        uint16_t x = col * font_width;
        uint16_t y = aline * font_height;
        uint16_t w = font_width;
        uint16_t h = font_height;
        gfxd_window_set_area(x, y, w, h);
        gfxd_screen_paint(_scr_ctx->render_buf, (font_width * font_height));
    }
    else {
        _scr_ctx->dirty_text_lines[aline] = true;
    }
}

/*
 * Clear from a column to the end of line of text.
 *
 * NOTE: This does not perform text line translation, nor bounds check.
 */
static void _disp_eol_clear(uint16_t aline, uint16_t col, paint_control_t paint) {
    memset((_scr_ctx->full_screen_text + (aline * _scr_ctx->cols) + col), SPACE_CHR, (_scr_ctx->cols - col));
    memset((_scr_ctx->full_screen_color + (aline * _scr_ctx->cols) + col), colorbyte(_scr_ctx->color_fg_default, _scr_ctx->color_bg_default), (_scr_ctx->cols - col));
    if (paint) {
        _disp_line_paint(aline);
    }
    else {
        _scr_ctx->dirty_text_lines[aline] = true;
    }
}

/*
 * Clear a line of text.
 *
 * NOTE: This does not perform text line translation, nor bounds check.
 */
static void _disp_line_clear(uint16_t aline, paint_control_t paint) {
    memset((_scr_ctx->full_screen_text + (aline * _scr_ctx->cols)), SPACE_CHR, _scr_ctx->cols);
    memset((_scr_ctx->full_screen_color + (aline * _scr_ctx->cols)), colorbyte(_scr_ctx->color_fg_default, _scr_ctx->color_bg_default), _scr_ctx->cols);
    if (paint) {
        _disp_line_paint(aline);
    }
    else {
        _scr_ctx->dirty_text_lines[aline] = true;
    }
}

/*
 * Update the portion of the screen containing the given character line.
 *
 * Getting the glyph data for the font character is similar to `disp_char_colorbyte`;
 * but the difference is that here we are rendering a complete line of characters one
 * glyph line at a time, and repeating that for all of the glyph lines.
 *
 * NOTE: This does not perform text line translation, nor bounds check.
 */
static void _disp_line_paint(uint16_t aline) {
    const font_info_t* fi = _scr_ctx->font_info;
    int8_t font_height = fi->height;
    int8_t font_width = fi->width;
    int8_t bpgl = fi->bytes_per_glyph_line;
    bool show_cursor = (_scr_ctx->show_cursor && aline == _translate_cursor_line(_scr_ctx->cursor_pos.line));
    int8_t cursor_show_row = fi->suggested_cursor_line;
    uint16_t screen_line = aline * font_height;
    rgb18_t* rbuf = _scr_ctx->render_buf;
    for (int glyph_line = 0; glyph_line < font_height; glyph_line++) {
        for (uint16_t textcol = 0; textcol < _scr_ctx->cols; textcol++) {
            uint16_t index = (aline * _scr_ctx->cols) + textcol;
            unsigned char c = _scr_ctx->full_screen_text[index];
            bool invert = c & DISP_CHAR_INVERT_BIT;
            unsigned char cl = c & 0x7F;
            uint8_t color = _scr_ctx->full_screen_color[index];
            uint8_t fg = (invert ? bg_from_cb(color) : fg_from_cb(color));
            uint8_t bg = (invert ? fg_from_cb(color) : bg_from_cb(color));
            rgb18_t fgrgb = rgb18_from_color16(fg);
            rgb18_t bgrgb = rgb18_from_color16(bg);
            // Get the glyph row for the character (a line of the font char height).
            uint16_t glyphindex = (cl * font_height * bpgl);
            uint32_t cgr = 0;
            for (int byte = 0; byte < bpgl; byte++) {
                cgr |= (fi->glyphs[glyphindex + byte + (glyph_line * bpgl)]) << (8u * byte);
            }
            for (uint32_t mask = (1u << (font_width - 1u)); mask; mask >>= 1u) {
                if (show_cursor && textcol == _scr_ctx->cursor_pos.column && glyph_line == cursor_show_row) {
                    // Draw a cursor line
                    *rbuf++ = _scr_ctx->cursor_color;
                }
                else if (cgr & mask) {
                    // set the fg color
                    *rbuf++ = fgrgb;
                }
                else {
                    // set the bg color
                    *rbuf++ = bgrgb;
                }
            }
        }
    }
    // Write the pixel screen row to the display
    gfxd_window_set_area(0, screen_line, _scr_ctx->cols * font_width, font_height);
    gfxd_screen_paint(_scr_ctx->render_buf, _scr_ctx->cols * font_width * font_height);
}

/**
 * @brief Get the absolute text line index for the current cursor position, accounting for scroll.
 *
 * @return uint16_t The absolute text line number for the cursor
 */
static uint16_t _translate_cursor_line(uint16_t curline) {
    // Cursor line is relative to the scroll area. For example, cursor line 1 is the
    // 2nd line within the scroll area.
    uint16_t aline = curline + _scr_ctx->scroll_start;
    if (aline >= _scr_ctx->lines - _scr_ctx->fixed_area_bottom_size) {
        aline -= _scr_ctx->scroll_size;
    }

    return (aline);
}

/**
 * @brief Get the absolute text line index for the requested line, accounting for scroll.
 *
 * @return uint16_t The absolute text line number
 */
static uint16_t _translate_line(uint16_t line) {
    // A bit more work is needed than with the cursor, since `line` can be outside
    // of the scroll area and is from the top of the screen (absolute line 0).
    if (line < _scr_ctx->fixed_area_top_size || line >= (_scr_ctx->lines - _scr_ctx->fixed_area_bottom_size)) {
        return (line);
    }
    // The line is within the scroll area, so adjust it and translate it the same as the cursor.
    line -= _scr_ctx->fixed_area_top_size;
    return (_translate_cursor_line(line));
}

// ======================================================================================
// Public functions
// ======================================================================================

inline rgb18_t rgb18_from_color16(colorn16_t c16) {
    return (_color16_map[c16 & 0x0f]);
}

inline scr_position_t disp_cursor_get(void) {
    return _scr_ctx->cursor_pos;
}

void disp_cursor_home(void) {
    disp_cursor_set(0, 0);
}

void disp_cursor_show(bool show) {
    _scr_ctx->show_cursor = show;
}

void disp_cursor_set(uint16_t line, uint16_t col) {
    disp_cursor_set_sp((scr_position_t){line, col});
}

void disp_cursor_set_sp(scr_position_t pos) {
    if (pos.line >= _scr_ctx->scroll_size || pos.column >= _scr_ctx->cols) {
        return;
    }
    _scr_ctx->cursor_pos = pos;
}

/*! @brief Create 'color-byte number' from forground & background color numbers */
inline colorbyte_t colorbyte(colorn16_t fg, colorn16_t bg) {
    return ((bg << 4u) | fg);
}

/*! @brief Get forground color number from color-byte number */
inline colorn16_t fg_from_cb(colorbyte_t cb) {
    return (cb & 0x0f);
}

/*! @brief Get background color number from color-byte number */
inline colorn16_t bg_from_cb(colorbyte_t cb) {
    return ((cb & 0xf0) >> 4u);
}

void disp_c16_color_chart() {
    disp_clear(true);
    // VGA Color Test
    disp_text_colors_set(C16_BR_WHITE, C16_BLACK);
    for (uint8_t i = 0; i < 8; i++) {
        char c = '0' + i;
        disp_char(4, (2 * i) + 5, c, true);
        disp_char_colorbyte(5, (2 * i) + 5, DISP_CHAR_INVERT_BIT | SPACE_CHR, i, true);
    }
    for (uint8_t i = 8; i < 16; i++) {
        char c = '0' + i;
        if (c > '9') c += 7;
        disp_char(7, (2 * (i - 8)) + 5, c, true);
        disp_char_colorbyte(8, (2 * (i - 8)) + 5, DISP_CHAR_INVERT_BIT | SPACE_CHR, i, true);
    }
}

/*
 * Clear the current text content and the screen.
*/
void disp_clear(paint_control_t paint) {
    size_t chars = _scr_ctx->lines * _scr_ctx->cols;
    memset(_scr_ctx->full_screen_text, SPACE_CHR, chars);
    memset(_scr_ctx->full_screen_color, colorbyte(_scr_ctx->color_fg_default, _scr_ctx->color_bg_default), chars);
    memset(_scr_ctx->dirty_text_lines, false, _scr_ctx->lines);
    disp_cursor_home();
    if (paint) {
        display_backlight_on(false);    // Turning off the backlight helps this from being distracting
        gfxd_screen_clr_c16(_scr_ctx->color_bg_default, false);
        display_backlight_on(true);
    }
}

void disp_char(uint16_t line, uint16_t col, char c, paint_control_t paint) {
    if (line >= _scr_ctx->lines || col >= _scr_ctx->cols) {
        return;  // Invalid line or column
    }
    uint16_t aline = _translate_line(line); // Adjust for scroll
    _disp_char(aline, col, c, paint);
}

void disp_char_color(uint16_t line, uint16_t col, char c, colorn16_t fg, colorn16_t bg, paint_control_t paint) {
    disp_char_colorbyte(line, col, c, colorbyte(fg, bg), paint);
}

/*
 * Display an ASCII character (plus some special characters)
 * If the top bit is set (c>127) the character is inverse (black on white background).
 *
 * Getting the glyph data for the font character is similar to `disp_line_paint`;
 * but the difference is that here we are rendering one single complete charcter.
 *
 */
void disp_char_colorbyte(uint16_t line, uint16_t col, char c, colorbyte_t color, paint_control_t paint) {
    if (line >= _scr_ctx->lines || col >= _scr_ctx->cols) {
        return;  // Invalid line or column
    }
    uint16_t aline = _translate_line(line); // Adjust for scroll
    _disp_char_colorbyte(aline, col, c, color, paint);
}

/*
 * Display all of the font characters. Wrap the characters until the full
 * screen is filled.
 */
void disp_font_test(void) {
    disp_clear(true);
    // test font display 1
    char c = 0;
    int16_t lines = _scr_ctx->lines;
    int16_t cols = _scr_ctx->cols;
    for (unsigned int line = 0; line < lines; line++) {
        for (unsigned int col = 0; col < cols; col++) {
            _disp_char(line, col, c, true);
            c++;
        }
    }
}

uint16_t disp_info_columns() {
    return (_scr_ctx->cols);
}

uint16_t disp_info_lines() {
    return (_scr_ctx->lines);
}

uint16_t disp_info_fixed_top_lines() {
    return (_scr_ctx->fixed_area_top_size);
}

uint16_t disp_info_fixed_bottom_lines() {
    return (_scr_ctx->fixed_area_bottom_size);
}

uint16_t disp_info_scroll_lines() {
    return (_scr_ctx->scroll_size);
}

scr_position_t disp_lc_from_point(const gfx_point* p) {
    uint16_t x = _max(p->x, 0);
    x = _min(x, gfxd_screen_width());
    uint16_t y = _max(p->y, 0);
    y = _min(y, gfxd_screen_height());
    const font_info_t* fi = &font_10_16;

    uint16_t col = x / fi->width;
    uint16_t line = y / fi->height;
    scr_position_t pos;
    pos.line = line;
    pos.column = col;

    return (pos);
}

bool disp_ready(void) {
    return _display_ready;
}


/*! @brief Initialize the display
 *  \ingroup display
 *
 * This must be called before using the display, but should only be called once.
 */
void disp_module_init(void) {
    int i = 0;

    _color16_map[i++] = RGB18_BLACK;
    _color16_map[i++] = RGB18_BLUE;
    _color16_map[i++] = RGB18_GREEN;
    _color16_map[i++] = RGB18_CYAN;
    _color16_map[i++] = RGB18_RED;
    _color16_map[i++] = RGB18_MAGENTA;
    _color16_map[i++] = RGB18_BROWN;
    _color16_map[i++] = RGB18_WHITE;
    _color16_map[i++] = RGB18_GREY;
    _color16_map[i++] = RGB18_LT_BLUE;
    _color16_map[i++] = RGB18_LT_GREEN;
    _color16_map[i++] = RGB18_LT_CYAN;
    _color16_map[i++] = RGB18_ORANGE;
    _color16_map[i++] = RGB18_LT_MAGENTA;
    _color16_map[i++] = RGB18_YELLOW;
    _color16_map[i++] = RGB18_BR_WHITE;


    // run through the complete initialization process

    ili_controller_type ctrl_type = ili_module_init();
    ili_disp_info_t* disp_info = ili_disp_info();

    // If in debug mode, print info about the display...
    if (debug_mode_enabled()) {
        debug_printf(true, "Display ID1:         %02hhx\n", disp_info->lcd_id1_mfg);
        debug_printf(true, "Display ID2:         %02hhx\n", disp_info->lcd_id2_ver);
        debug_printf(true, "Display ID3:         %02hhx\n", disp_info->lcd_id3_drv);
        debug_printf(true, "Display Status 1:    %02hhx\n", disp_info->status1);
        debug_printf(true, "Display Status 2:    %02hhx\n", disp_info->status2);
        debug_printf(true, "Display Status 3:    %02hhx\n", disp_info->status3);
        debug_printf(true, "Display Status 4:    %02hhx\n", disp_info->status4);
        debug_printf(true, "Display PWR Mode:    %02hhx\n", disp_info->pwr_mode);
        debug_printf(true, "Display MADCTL:      %02hhx\n", disp_info->madctl);
        debug_printf(true, "Display Pixel Fmt:   %02hhx\n", disp_info->pixelfmt);
        debug_printf(true, "Display Image Fmt:   %02hhx\n", disp_info->imagefmt);
        debug_printf(true, "Display Signal Mode: %02hhx\n", disp_info->signal_mode);
        debug_printf(true, "Display Selftest:    %02hhx\n", disp_info->selftest);
    }

    if (_scr_ctx != NULL) {
        warn_printf(false, "`disp_module_init` called multiple times!\n");
        return;
    }

    disp_screen_new();

    if (ctrl_type != ILI_CONTROLLER_NONE) {
        _display_ready = true;
    }
}

/*
 * Clear a line of text.
 */
void disp_line_clear(uint16_t line, paint_control_t paint) {
    if (line >= _scr_ctx->lines) {
        return;  // Invalid line or column
    }
    uint16_t aline = _translate_line(line); // Adjust for scroll
    _disp_line_clear(aline, paint);
}

/*
 * Update the portion of the screen containing the given character line.
 *
 * Getting the glyph data for the font character is similar to `disp_char_colorbyte`;
 * but the difference is that here we are rendering a complete line of characters one
 * glyph line at a time, and repeating that for all of the glyph lines.
 */
void disp_line_paint(uint16_t line) {
    if (line >= _scr_ctx->lines) {
        return; // invalid line number
    }
    line = _translate_line(line);
    _disp_line_paint(line);
}

/*
 * Paint the physical screen from the text.
 */
void disp_paint(void) {
    int16_t lines = _scr_ctx->lines;
    uint16_t aline;
    bool some_lines_dirty = false;
    for (uint16_t i = 0; i < lines && !some_lines_dirty; i++) {
        some_lines_dirty |= _scr_ctx->dirty_text_lines[i];
    }
    if (some_lines_dirty) {
        for (uint16_t line = 0; line < lines; line++) {
            aline = _translate_line(line);
            if (_scr_ctx->dirty_text_lines[aline]) {
                _disp_line_paint(aline);
                _scr_ctx->dirty_text_lines[aline] = false; // The text line has been painted, mark it 'not dirty'
            }
        }
    }
}

void disp_print_crlf(int16_t add_lines, paint_control_t paint) {
    int16_t total_scroll_lines = add_lines;
    uint16_t ss = _scr_ctx->scroll_start;  // Scroll start line
    uint16_t scroll_lines = _scr_ctx->scroll_size;  // Screen scroll lines
    uint16_t scroll_cap = _scr_ctx->lines - _scr_ctx->fixed_area_bottom_size - 1;
    uint16_t cursor_cap = scroll_lines - 1;
    scr_position_t new_cp = { _scr_ctx->cursor_pos.line + 1, 0 };
    uint16_t aline;
    if (new_cp.line > cursor_cap) {
        total_scroll_lines += (new_cp.line - cursor_cap);
        new_cp.line = cursor_cap;
    }
    if (total_scroll_lines > scroll_lines) {
        total_scroll_lines = scroll_lines;
    }
    if (total_scroll_lines > 0) {
        // Advance the Scroll-Start
        ss++;
        if (ss > scroll_cap) {
            ss = _scr_ctx->fixed_area_top_size;
        }
        _scr_ctx->scroll_start = ss;
        // blank out what will be the cursor line
        aline = _translate_cursor_line(new_cp.line);
        _disp_line_clear(aline, paint);
        gfxd_scroll_set_start(ss * _scr_ctx->font_info->height);
    }
    else {
        // blank out what will be the cursor line
        aline = _translate_cursor_line(new_cp.line);
        _disp_line_clear(aline, paint);
    }
    _scr_ctx->cursor_pos = new_cp;
}

void disp_print_erase_eol(paint_control_t paint) {
    // blank out what will be the cursor line
    uint16_t aline = _translate_cursor_line(_scr_ctx->cursor_pos.line);
    _disp_eol_clear(aline, _scr_ctx->cursor_pos.column, paint);
}

uint16_t disp_print_wrap_len_get() {
    return _wrap_len;
}

void disp_print_wrap_len_set(uint16_t len) {
    // Limit the value to the one less than the screen width.
    _wrap_len = (len < _scr_ctx->cols ? len : (_scr_ctx->cols - 1));
}

void disp_printc(char c, paint_control_t paint) {
    // Since the line doesn't wrap right when the last column is written to;
    // we need to check the current cursor position against the current margins and
    // wrap/scroll if needed before printing the character.
    //
    // Also, this allows printing a NL ('\n') character, so nothing special is
    // done for it.
    //
    uint16_t aline = _translate_cursor_line(_scr_ctx->cursor_pos.line);
    if (_scr_ctx->cursor_pos.column >= _scr_ctx->cols) {
        // Need to go to next line (CRLF)
        // Check for re-wrapping existing text
        bool nl_done = false;
        if (SPACE_CHR != c && _wrap_len > 0) {
            // Yes - see if we can find a space within `_wrap_len` characters back.
            // Plan for finding one, therefore reprinting characters from it to eol...
            char rc[_wrap_len];
            uint8_t rclr[_wrap_len];
            int break_flag = 0;
            char sc;
            int i = 0;
            for (; i < _wrap_len; i++) {
                sc = _disp_char_get(aline, (_scr_ctx->cursor_pos.column - (i+1)));
                rc[i] = sc;
                rclr[i] = _disp_char_color_get(aline, (_scr_ctx->cursor_pos.column - (i+1)));
                if (SPACE_CHR == sc) {
                    break; // break in place of
                }
                switch (sc) {
                    case '$': case '(': case '*': case '+': case '-': case '<':
                    case '=': case '>': case '@': case '[': case '{':
                        break_flag = -1; // break before
                        break;
                }
                if (break_flag == 0 && (sc < '0' || ':' == sc || ';' == sc || '?' == sc || ']' == sc || '}' == sc)) {
                        break_flag = 1; //break after
                }
                if (break_flag != 0) {
                    break;
                }
            }
            if (i < _wrap_len) {
                // Found a place that we can break;
                disp_cursor_set(_scr_ctx->cursor_pos.line, _scr_ctx->cursor_pos.column - (i + 1));
                _disp_eol_clear(aline, _scr_ctx->cursor_pos.column, No_Paint);
                if (break_flag <= 0) {
                    // Break before or in place of the character we found
                    disp_print_crlf(0, No_Paint);
                    aline = _translate_cursor_line(_scr_ctx->cursor_pos.line);
                }
                if (SPACE_CHR != rc[i]) {
                    // Print the character we found if it isn't a space
                    _disp_char_colorbyte(aline, _scr_ctx->cursor_pos.column++, rc[i], rclr[i], No_Paint);
                }
                if (break_flag > 0) {
                    // Break after the character we found
                    disp_print_crlf(0, No_Paint);
                    aline = _translate_cursor_line(_scr_ctx->cursor_pos.line);
                }
                // print the characters we stored before finding the break point
                for (int j = (i-1); j >= 0; j--) {
                    _disp_char_colorbyte(aline, _scr_ctx->cursor_pos.column++, rc[j], rclr[j], No_Paint);
                }
                nl_done = true;
            }
        }
        if (!nl_done) {
            disp_print_crlf(0, paint);
            if (SPACE_CHR == c) {
                // If we needed to do a new line and this character is a space, don't print it.
                return;
            }
            aline = _translate_cursor_line(_scr_ctx->cursor_pos.line);
        }
    }
    _disp_char(aline, _scr_ctx->cursor_pos.column++, c, paint);
}

void disp_prints(char* str, paint_control_t paint) {
    unsigned char c;
    while ((c = *str++) != 0) {
        if (c == '\n') {
            disp_print_crlf(0, false);
        }
        else {
            disp_printc(c, false);
        }
    }
    if (paint) {
        disp_paint();
    }
}

void disp_string(uint16_t line, uint16_t col, const char *pString, bool invert, paint_control_t paint) {
    if (line >= _scr_ctx->lines || col >= _scr_ctx->cols) {
        return;  // Invalid line or column
    }
    for (unsigned char c = *pString; c != 0; c = *(++pString)) {
        if (invert) {
            c = c ^ DISP_CHAR_INVERT_BIT;
        }
        disp_char(line, col, c, false);
        col++;
        if (col == _scr_ctx->cols) {
            col = 0;
            line++;
            if (line == _scr_ctx->lines) {
                line = 0;
            }
        }
    }
    if (paint) {
        disp_paint();
    }
}

void disp_string_color(uint16_t line, uint16_t col, const char* pString, colorn16_t fg, colorn16_t bg, paint_control_t paint){
    if (line >= _scr_ctx->lines || col >= _scr_ctx->cols) {
        return;  // Invalid line or column
    }
    for (unsigned char c = *pString; c != 0; c = *(++pString)) {
        disp_char_color(line, col, c, fg, bg, false);
        col++;
        if (col == _scr_ctx->cols) {
            col = 0;
            line++;
            if (line == _scr_ctx->lines) {
                line = 0;
            }
        }
    }
    if (paint) {
        disp_paint();
    }

}

void disp_text_colors_cp_set(text_color_pair_t* cp) {
    // force them to 0-15
    _scr_ctx->color_fg_default = cp->fg & 0x0f;
    _scr_ctx->color_bg_default = cp->bg & 0x0f;
}

void disp_text_colors_get(text_color_pair_t* cp) {
    cp->fg = _scr_ctx->color_fg_default;
    cp->bg = _scr_ctx->color_bg_default;
}

void disp_text_colors_set(colorn16_t fg, colorn16_t bg) {
    // force them to 0-15
    _scr_ctx->color_fg_default = fg & 0x0f;
    _scr_ctx->color_bg_default = bg & 0x0f;
}

void disp_update(paint_control_t paint) {
    // Mark all lines as 'dirty' so they will be re-rendered during a `paint` operation.
    memset(_scr_ctx->dirty_text_lines, true, _scr_ctx->lines * sizeof(bool));
    if (paint) {
        disp_paint();
    }
}

void disp_screen_close() {
    if (!_has_scr_context()) {
        warn_printf(false, "Display - Trying to close main screen context. Ignoring `disp_screen_close()` call.");
        return;
    }
    // Free the buffers from the current context...
    free(_scr_ctx->full_screen_text);
    free(_scr_ctx->full_screen_color);
    free(_scr_ctx->dirty_text_lines);
    free(_scr_ctx->render_buf);
    // Now free the current context
    free(_scr_ctx);

    // Get the top context and make it current
    _scr_ctx = _pop_scr_context();
    // This will re-configure the ILI for correct scrolling
    disp_scroll_area_define(_scr_ctx->fixed_area_top_size, _scr_ctx->fixed_area_bottom_size);
    disp_update(Paint);
}

bool disp_screen_new() {
    // First thing is to see if we can push the current screen context if there is one
    if (_scr_ctx) {
        if (!_push_scr_context(_scr_ctx)) {
            return false;
        }
    }
    scr_context_t* scr_context = malloc(sizeof(scr_context_t));
    if (scr_context == NULL) {
        error_printf(false, "Display - Could not allocate a screen context.");
        return false;
    }
    // For now, we only have one font - get its info
    const font_info_t* fi = &font_10_16;
    info_printf(true, "Display font: %s.\n", fi->name);
    scr_context->font_info = fi;
    scr_context->color_bg_default = C16_BLACK;
    scr_context->color_fg_default = C16_WHITE;
    // Figure out how many lines and columns we have
    int16_t cols = gfxd_screen_width() / fi->width;
    int16_t lines = gfxd_screen_height() / fi->height;
    info_printf(true, "Display size: %hdx%hd (cols x lines)\n", cols, lines);
    size_t chars = lines * cols;
    scr_context->cols = cols;
    scr_context->lines = lines;
    // Allocate buffers
    scr_context->full_screen_text = (uint8_t*)malloc(chars);
    scr_context->full_screen_color = (colorbyte_t*)malloc(chars);
    scr_context->dirty_text_lines = (bool*)calloc(lines, sizeof(bool));
    scr_context->render_buf = (rgb18_t*)malloc(fi->width * fi->height * cols * sizeof(rgb18_t));
    // Default scroll area to the full screen
    scr_context->fixed_area_top_size = 0;
    scr_context->fixed_area_bottom_size = 0;
    scr_context->scroll_size = lines;
    scr_context->scroll_start = 0;
    scr_context->cursor_pos = (scr_position_t){ 0, 0 };
    scr_context->show_cursor = false; // Start with the cursor off (typical for dialogs)
    scr_context->cursor_color = (rgb18_t){0x00,0x64,0x00};    // Custom Green so it doesn't match any of the 16 (0 45 0)
    // Set this as the current context before calling other 'screen' functions.
    _scr_ctx = scr_context;
    disp_scroll_area_define(0, 0);   // This will configure the ILI for scrolling
    disp_clear(Paint);

    return (true);
}

void disp_scroll_area_define(uint16_t top_fixed_size, uint16_t bottom_fixed_size) {
    uint16_t screen_lines = _scr_ctx->lines;
    uint16_t fixed_lines = top_fixed_size + bottom_fixed_size;
    int16_t scroll_lines = screen_lines - fixed_lines;
    if (scroll_lines < 0) {
        error_printf(false, "Display - Attempting to set fixed regions of screen larger than total screen lines.");
        return;
    }
    else if (scroll_lines == 0) {
        // To keep the `print...` functions working, while avoiding a bunch of special
        // case code, we treat this setting the same as full screen scroll.
        // If we find a use case that truly requires the whole screen to be fixed, 
        // we will address this.
        top_fixed_size = 0;
        bottom_fixed_size = 0;
    }
    _scr_ctx->scroll_start = top_fixed_size;
    _scr_ctx->fixed_area_top_size = top_fixed_size;
    _scr_ctx->fixed_area_bottom_size = bottom_fixed_size;
    _scr_ctx->scroll_size = screen_lines - (top_fixed_size + bottom_fixed_size);
    gfxd_scroll_set_area(top_fixed_size * _scr_ctx->font_info->height, bottom_fixed_size * _scr_ctx->font_info->height);
    gfxd_scroll_set_start(_scr_ctx->scroll_start);
    disp_cursor_home();
}

void rgb18_buf_fill(rgb18_t* rgb_buf, rgb18_t rgb, size_t size) {
    for (int i=0; i<size; i++) {
        rgb18_t* f = rgb_buf++;
        f->r = rgb.r;
        f->g = rgb.g;
        f->b = rgb.b;
    }
}

