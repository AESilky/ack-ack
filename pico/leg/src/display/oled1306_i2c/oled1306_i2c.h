/**
 * SPDX-FileCopyrightText: Copyright 2023-24 AESilky
 * Portions copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _OLED1306_I2C_H_
#define _OLED1306_I2C_H_
#ifdef __cplusplus
 extern "C" {
#endif

/**
 * Some portions from Raspberry Pi Pico example code to talk to an SSD1306-based OLED display
 * https://github.com/raspberrypi/pico-examples/tree/master/i2c/oled_i2c
 *
 * (SSD1306 Datasheet: https://www.digikey.com/htmldatasheets/production/2047793/0/0/1/SSD1306.pdf)
 */
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"

// I2C values if not set outside...
#ifndef I2C_PORT
#define I2C_PORT i2c_default
#endif

// commands (see datasheet)
#define OLED_SET_CONTRAST _u(0x81)
#define OLED_SET_ENTIRE_ON _u(0xA4)
#define OLED_SET_NORM_INV _u(0xA6)
#define OLED_SET_DISP _u(0xAE)
#define OLED_SET_MEM_ADDR_MODE _u(0x20)
#define OLED_SET_COL_ADDR _u(0x21)
#define OLED_SET_PAGE_ADDR _u(0x22)
#define OLED_SET_DISP_START_LINE _u(0x40)
#define OLED_SET_SEG_REMAP _u(0xA0)
#define OLED_SET_MUX_RATIO _u(0xA8)
#define OLED_SET_COM_OUT_DIR _u(0xC0)
#define OLED_SET_DISP_OFFSET _u(0xD3)
#define OLED_SET_COM_PIN_CFG _u(0xDA)
#define OLED_SET_DISP_CLK_DIV _u(0xD5)
#define OLED_SET_PRECHARGE _u(0xD9)
#define OLED_SET_VCOM_DESEL _u(0xDB)
#define OLED_SET_CHARGE_PUMP _u(0x8D)
#define OLED_SET_HORIZ_SCROLL _u(0x26)
#define OLED_SET_SCROLL _u(0x2E)
//
#define OLED_ADDR _u(0x3C)
#define OLED_HEIGHT _u(64)
#define OLED_WIDTH _u(128)
#define OLED_PAGE_HEIGHT _u(8)
#define OLED_NUM_PAGES OLED_HEIGHT / OLED_PAGE_HEIGHT
#define OLED_BUF_LEN (OLED_NUM_PAGES * OLED_WIDTH)
//
#define OLED_WRITE_MODE _u(0xFE)
#define OLED_READ_MODE _u(0xFF)

typedef struct _render_area {
    uint8_t start_col;
    uint8_t end_col;
    uint8_t start_page;
    uint8_t end_page;
    int buflen;
} render_area_t;

/*! @brief Memory area for the screen data pixel-bytes */
extern uint8_t oled_disp_buf[];
/*! @brief Render area for the full oled display screen */
extern render_area_t display_full_area;

void oled_disp_fill(uint8_t buf[], uint8_t fill);

void oled_disp_fill_page(uint8_t *buf, uint8_t fill, uint8_t page);

void calc_render_area_buflen(render_area_t *area);

void oled_send_cmd(uint8_t cmd);

void oled_write_buf(uint8_t buf[], int buflen);

void oled_module_init();

void oled_disp_render(uint8_t *buf, render_area_t *area);

/*! @brief Scroll display horizontally
 *  \ingroup oled1306_i2c
 *
 *  Scroll the full display from right to left.
 */
void oled_disp_scroll_horz(void);

// ################################################################################################
// print_xxx convenience methods for printing out a buffer to be rendered
// mostly useful for debugging images, patterns, etc

void oled_disp_print_buf_page(uint8_t buf[], uint8_t page);

void oled_disp_print_buf_pages(uint8_t buf[]);

void oled_disp_print_buf_area(uint8_t *buf, render_area_t *area);

#ifdef __cplusplus
}
#endif
#endif // _OLED1306_I2C_H_
