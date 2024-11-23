/**
 * ILI TFT LCD functionality interface through SPI
 *
 * Common to ILI9341 and ILI9488
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 *
 * Using ILI 9341 or 9488 4-Line Serial Interface II
 * 64k Color (16bit) Mode
 */
#include "system_defs.h"
#include "display_rgb18.h"
#include "ili_lcd_spi.h"
#include "ili9341_spi/ili9341_spi.h"
#include "ili9488_spi/ili9488_spi.h"
#include "board.h"
#include "spi_ops.h"

#include "pico/stdlib.h"
#include "pico/malloc.h"

#include <string.h>

#define DISP_OP_CMD 0       // Set the D/C- pin low for command mode
#define DISP_OP_DATA 1      // Set the D/C- pin high for data mode

static void _write_area(const rgb18_t* rgb_pixel_data, uint16_t pixels);
static void _set_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
static void _set_window_fullscreen(void);

static uint16_t _screen_height = 0;
static uint16_t _screen_width = 0;

static rgb18_t *_ili_line_buf = NULL;

// Storage for last used screen window location
static uint16_t _old_x1 = 0xffff, _old_x2 = 0xffff;
static uint16_t _old_y1 = 0xffff, _old_y2 = 0xffff;

/** @brief Flag to track if the screen has been written to since we know it was cleared. */
static bool _screen_dirty = true; // start out assuming dirty

static ili_disp_info_t _ili_disp_info;
static ili_controller_type _ili_controller_type = ILI_CONTROLLER_NONE;

/**
 * Set the chip select for the display.
 *
 */
static void _cs(bool sel) {
    if (sel) {
        spi_display_select();
    }
    else {
        spi_none_select();
    }
}

/**
 * Set the data/command select for the display.
 *
 */
static void _command_mode(bool cmd) {
    if (cmd) {
        gpio_put(SPI_DISP_CD, DISP_OP_CMD);
    }
    else {
        gpio_put(SPI_DISP_CD, DISP_OP_DATA);
    }
}

static void _op_begin() {
    spi_display_begin();
    _cs(true);
}

static void _op_end() {
    _cs(false);
    spi_display_end();
}

/**
 * @brief Read register values from the controller.
 * MUST BE WITHIN `_op_begin` and `_op_end`!!!
 *
 * Send a command indicating what to read and then read a
 * number of data bytes.
*/
static int _read_controller_values(uint8_t cmd, uint8_t* data, size_t count) {
    _command_mode(true);
    spi_display_write8_buf(&cmd, 1);
    _command_mode(false);
    int read = spi_display_read_buf(SPI_LOW_TXD_FOR_READ, data, count);

    return (read);
}

/**
 * @brief Send a single command byte to the controller.
 * MUST BE WITHIN `_op_begin` and `_op_end`!!!
*/
static void _send_command(uint8_t cmd) {
    _command_mode(true);
    spi_display_write8_buf(&cmd, 1);
    _command_mode(false);
}

/**
 * @brief Send a command and 0-n data bytes of data to the controller.
 * MUST BE AFTER `_op_begin`!!! An '_op_end` should follow after the data.
*/
static void _send_command_wd(uint8_t cmd, const uint8_t* data, size_t count) {
    _send_command(cmd);
    spi_display_write8_buf(data, count);
}

/** @brief Set window. MUST BE CALLED WITHIN `_op_begin` and `_op_end`!!! */
static void _set_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    uint16_t x2 = (x + w - 1), y2 = (y + h - 1);
    uint16_t words[2];
    if (x != _old_x1 || x2 != _old_x2) {
        _send_command(ILI_CASET); // Column address set
        words[0] = x;
        words[1] = x2;
        spi_display_write16(words[0]);
        spi_display_write16(words[1]);
        _old_x1 = x;
        _old_x2 = x2;
    }
    if (y != _old_y1 || y2 != _old_y2) {
        _send_command(ILI_PASET); // Page address set
        words[0] = y;
        words[1] = y2;
        spi_display_write16(words[0]);
        spi_display_write16(words[1]);
        _old_y1 = y;
        _old_y2 = y2;
    }
    _send_command(ILI_RAMWR); // Set it up to write to RAM
}

/** @brief Set window to fullscreen. MUST BE CALLED WITHIN `_op_begin` and `_op_end`!!! */
static void _set_window_fullscreen(void) {
    _set_window(0, 0, _screen_width, _screen_height);
}

/**
 * @brief Paint a buffer onto the screen. MUST BE CALLED WITHIN AN OPERATION!
*/
static void _write_area(const rgb18_t* rgb_pixel_data, uint16_t pixels) {
    spi_display_write8_buf((const uint8_t*)rgb_pixel_data, pixels * sizeof(rgb18_t));
}

/**
 * Show all of the colors.
 */
void ili_colors_show() {
    gfxd_screen_clr(RGB18_BLACK, false);
    _op_begin();
    {
        // Do each color 4x4
        _set_window(0, 0, 32 * 4, 4);
        // Reds...
        rgb18_t rgb = RGB18_BLACK;
        for (int row = 0; row < 4; row++) {
            for (int r = 0; r < 64; r++) {
                rgb.r = r << 2;
                for (int col = 0; col < 2; col++) {
                    spi_display_write8_buf((const uint8_t*)&rgb, sizeof(rgb18_t));
                }
            }
        }
        // Greens...
        rgb.r = RGB18_ELM_BLANK;
        _set_window(0, 4, 32 * 4, 4);
        for (int row = 0; row < 4; row++) {
            for (int g = 0; g < 64; g++) {
                rgb.g = g << 2;
                for (int col = 0; col < 2; col++) {
                    spi_display_write8_buf((const uint8_t*)&rgb, sizeof(rgb18_t));
                }
            }
        }
        // Blues...
        rgb.g = RGB18_ELM_BLANK;
        _set_window(0, 8, 32 * 4, 4);
        for (int row = 0; row < 4; row++) {
            for (int b = 0; b < 64; b++) {
                rgb.b = b << 2;
                for (int col = 0; col < 2; col++) {
                    spi_display_write8_buf((const uint8_t*)&rgb, sizeof(rgb18_t));
                }
            }
        }
        // // Colors 0 - 0xFFFF
        // _set_window(0, 12, 320, 228);
        // for (rgb16_t i = 0; i < 0xFFFF; i++) {
        //     spi_display_write16(i);
        // }
    }
    _op_end();
    _screen_dirty = true;
}

void ili_send_command(uint8_t cmd) {
    _op_begin();
    {
        _send_command(cmd);
    }
    _op_end();
}

void ili_send_command_wd(uint8_t cmd, uint8_t* data, size_t count) {
    _op_begin();
    {
        _send_command_wd(cmd, data, count);
    }
    _op_end();
}

rgb18_t* gfxd_get_line_buf() {
    return (_ili_line_buf);
}

/**
 * Read information about the display status and the current configuration.
*/
ili_disp_info_t* ili_disp_info(void) {
    // The info/status reads require that a command be sent,
    // then a 'dummy' byte read, then one or more data reads.
    //
    // The largest read is a 'dummy' + 4 bytes, so use a 5 byte array to read into.
    uint8_t data[6];

    _op_begin();
    {
        // Display Status
        // _read_controller_values(ILI_RDDST, data, 5);
        // _ili_disp_info.status1 = data[1];
        // _ili_disp_info.status2 = data[2];
        // _ili_disp_info.status3 = data[3];
        // _ili_disp_info.status4 = data[4];
        // Power Mode
        _read_controller_values(ILI_RDMODE, data, 2);
        _ili_disp_info.pwr_mode = data[1];
        // MADCTL
        _read_controller_values(ILI_RDMADCTL, data, 2);
        _ili_disp_info.madctl = data[1];
        // Pixel Format
        _read_controller_values(ILI_RDPIXFMT, data, 2);
        _ili_disp_info.pixelfmt = data[1];
        // Image Format
        _read_controller_values(ILI_RDIMGFMT, data, 2);
        _ili_disp_info.imagefmt = data[1];
        // Signal Mode
        _read_controller_values(ILI_RDSIGMODE, data, 2);
        _ili_disp_info.signal_mode = data[1];
        // Selftest result
        _read_controller_values(ILI_RDSELFDIAG, data, 2);
        _ili_disp_info.selftest = data[1];
        // ID 1 (Manufacturer)
        _read_controller_values(ILI_RDID1, data, 2);
        _ili_disp_info.lcd_id1_mfg = data[1];
        // ID 2 (Version)
        _read_controller_values(ILI_RDID2, data, 2);
        _ili_disp_info.lcd_id2_ver = data[1];
        // ID 3 (Driver)
        _read_controller_values(ILI_RDID3, data, 2);
        _ili_disp_info.lcd_id3_drv = data[1];
        // ID 4 (IC)
        // _read_controller_values(ILI_RDID4, data, 5);
        // _ili_disp_info.lcd_id4_ic_ver    = data[2];
        // _ili_disp_info.lcd_id4_ic_model1 = data[3];
        // _ili_disp_info.lcd_id4_ic_model2 = data[4];
    }
    _op_end();

    return (&_ili_disp_info);
}

uint16_t gfxd_screen_height() {
    return _screen_height;
}

void gfxd_screen_on(bool on) {
    _op_begin();
    {
        if (on) {
            _send_command(ILI_DISPON);
        }
        else {
            _send_command(ILI_DISPOFF);
        }
    }
    _op_end();
}

void gfxd_screen_paint(const rgb18_t* rgb_pixel_data, uint16_t pixels) {
    _op_begin();
    {
        _write_area(rgb_pixel_data, pixels);
    }
    _op_end();
    _screen_dirty = true;
}

uint16_t gfxd_screen_width() {
    return _screen_width;
}

void gfxd_scroll_exit(void) {
    _op_begin();
    {
        _send_command(ILI_DISPOFF);
        _send_command(ILI_NORON);
        _send_command(ILI_DISPON);
        _set_window_fullscreen();
    }
    _op_end();
}

void gfxd_scroll_set_area(uint16_t top_fixed_lines, uint16_t bottom_fixed_lines) {
    uint16_t words[3];
    _op_begin();
    {
        _send_command(ILI_VSCRDEF); // Vertical Scrolling Definition
        words[0] = top_fixed_lines;
        words[1] = _screen_height - (top_fixed_lines + bottom_fixed_lines);
        words[2] = bottom_fixed_lines;
        spi_display_write16_buf(words, 3);
        _send_command(ILI_VSCRSADD);
        uint16_t row = top_fixed_lines;
        spi_display_write16(row);
        // Set window within the scroll area
    }
    _op_end();
}

void gfxd_scroll_set_start(uint16_t row) {
    _op_begin();
    {
        _send_command(ILI_VSCRSADD);
        spi_display_write16(row);
    }
    _op_end();
}

void gfxd_window_set_area(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    _op_begin();
    {
        _set_window(x, y, w, h);
    }
    _op_end();
}

void gfxd_window_set_fullscreen(void) {
    _op_begin();
    {
        _set_window(0, 0, _screen_width, _screen_height);
    }
    _op_end();
}

void gfxd_line_paint(uint16_t line, rgb18_t* buf) {
    if (line >= _screen_height) {
        return;
    }
    _op_begin();
    {
        _set_window(0, line, _screen_width, 1);
        _write_area(buf, _screen_width);
    }
    _op_end();
}

void gfxd_screen_clr(rgb18_t color, bool force) {
    if (force || _screen_dirty) {
        rgb18_buf_fill(_ili_line_buf, color, _screen_width);
        _op_begin();
        {
            _set_window_fullscreen();
            for (int i = 0; i < _screen_height; i++) {
                _write_area(_ili_line_buf, _screen_width);
            }
        }
        _op_end();
        _screen_dirty = false;
    }
    else {
        // The screen wasn't dirty, so we didn't clear it,
        // but people expect a call to clear to set the
        // window to the full screen. Check and set if needed.
        if (_old_x1 != 0 || _old_y1 != 0 || _old_x2 != (_screen_width - 1) || _old_y2 != (_screen_height - 1)) {
            gfxd_window_set_fullscreen();
        }
    }
}

void gfxd_screen_clr_c16(colorn16_t color, bool force) {
    gfxd_screen_clr(rgb18_from_color16(color), force);
}

ili_controller_type ili_module_init(void) {
    // Take reset low, then high
    //ZZZ gpio_put(DISPLAY_RESET_OUT, DISPLAY_HW_RESET_OFF);
    //ZZZ sleep_ms(20);
    //ZZZ gpio_put(DISPLAY_RESET_OUT, DISPLAY_HW_RESET_ON);
    //ZZZ sleep_ms(20);
    //ZZZ gpio_put(DISPLAY_RESET_OUT, DISPLAY_HW_RESET_OFF);
    sleep_ms(500);  // Wait for it to come up
    // Do a software reset as well
    _send_command(ILI_SWRESET);
    sleep_ms(100);

    const uint8_t* init_cmd_data;

    // See which controller we have 9341 or 9488  (or none) so we can initialize appropriately.
    bool ZZZ = false; // Temp flag to force 9341 (true) or 9488 (false)
    ili_disp_info_t* info = ili_disp_info();
    if (ZZZ || (info->lcd_id4_ic_model1 == ILI9341_ID_MODEL1 && info->lcd_id4_ic_model2 == ILI9341_ID_MODEL2)) {
        _ili_controller_type = ILI_CONTROLLER_9341;
        init_cmd_data = ili9341_init_cmd_data;
        _screen_height = ILI9341_HEIGHT;
        _screen_width = ILI9341_WIDTH;
        _ili_line_buf = malloc(_screen_width * sizeof(rgb18_t));
    }
    else if (!ZZZ || (info->lcd_id4_ic_model1 == ILI9488_ID_MODEL1 && info->lcd_id4_ic_model2 == ILI9488_ID_MODEL2)) {
        _ili_controller_type = ILI_CONTROLLER_9488;
        init_cmd_data = ili9488_init_cmd_data;
        _screen_height = ILI9488_HEIGHT;
        _screen_width = ILI9488_WIDTH;
        _ili_line_buf = malloc(_screen_width * sizeof(rgb18_t));
    }
    else {
        warn_printf(false, "Cannot determine display controller type (9341 or 9488)");
    }

    if (_ili_controller_type != ILI_CONTROLLER_NONE) {
        uint8_t cmd, x, numArgs;
        _op_begin();
        {
            while ((cmd = *(init_cmd_data++)) > 0) {
                x = *(init_cmd_data++);
                numArgs = x & 0x7F;
                _send_command_wd(cmd, init_cmd_data, numArgs);
                init_cmd_data += numArgs;
                if (x & 0x80)
                    sleep_ms(150);
            }
        }
        _op_end();
    }
    display_backlight_on(true);

    return (_ili_controller_type);
}
