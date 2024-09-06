/**
 * Copyright 2023-24 AESilky
 * Portions copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * Some portions from Raspberry Pi Pico example code to talk to an SSD1306-based OLED display
 * https://github.com/raspberrypi/pico-examples/tree/master/i2c/oled_i2c
 *
 * (SSD1306 Datasheet: https://www.digikey.com/htmldatasheets/production/2047793/0/0/1/SSD1306.pdf)
 */
#include "system_defs.h"    // Include before our header so I2C_PORT is defined.
#include "oled1306_i2c.h"

#include <stdio.h>


/*! @brief Memory area for the screen data pixel-bytes */
uint8_t oled_disp_buf[OLED_BUF_LEN];

/*! @brief Memory area for the screen data pixel-bytes */
render_area_t display_full_area = {start_col: 0, end_col : OLED_WIDTH - 1, start_page : 0, end_page : OLED_NUM_PAGES - 1};

void oled_disp_fill(uint8_t buf[], uint8_t oled_disp_fill) {
    // oled_disp_fill entire buffer with the same byte
    for (int i = 0; i < OLED_BUF_LEN; i++) {
        buf[i] = oled_disp_fill;
    }
};

void oled_disp_fill_page(uint8_t *buf, uint8_t fill, uint8_t page) {
    // fill entire page with the same byte
    memset(buf + (page * OLED_WIDTH), fill, OLED_WIDTH);
};

void calc_render_area_buflen(render_area_t *area) {
    // calculate how long the flattened buffer will be for a render area
    area->buflen = (area->end_col - area->start_col + 1) * (area->end_page - area->start_page + 1);
}

void oled_send_cmd(uint8_t cmd) {
    // I2C write process expects a control byte followed by data
    // this "data" can be a command or data to follow up a command

    // Co = 1, D/C = 0 => the driver expects a command
    uint8_t buf[2] = {0x80, cmd};
    i2c_write_blocking(I2C_PORT, (OLED_ADDR & OLED_WRITE_MODE), buf, 2, false);
}

void oled_write_buf(uint8_t buf[], int buflen) {
    // in horizontal addressing mode, the column address pointer auto-increments
    // and then wraps around to the next page, so we can send the entire frame
    // buffer in one gooooooo!

    // copy our frame buffer into a new buffer because we need to add the control byte
    // to the beginning

    // TODO find a more memory-efficient way to do this..
    // maybe break the data transfer into pages?
    uint8_t *temp_buf = malloc(buflen + 1);

    for (int i = 1; i < buflen + 1; i++) {
        temp_buf[i] = buf[i - 1];
    }
    // Co = 0, D/C = 1 => the driver expects data to be written to RAM
    temp_buf[0] = 0x40;
    i2c_write_blocking(I2C_PORT, (OLED_ADDR & OLED_WRITE_MODE), temp_buf, buflen + 1, false);

    free(temp_buf);
}

void oled_module_init() {
    // some of these commands are not strictly necessary as the reset
    // process defaults to some of these but they are shown here
    // to demonstrate what the initialization sequence looks like

    // some configuration values are recommended by the board manufacturer

    oled_send_cmd(OLED_SET_DISP | 0x00);            // set display off

    /* memory mapping */
    oled_send_cmd(OLED_SET_MEM_ADDR_MODE);          // set memory address mode
    oled_send_cmd(0x00);                            // horizontal addressing mode

    /* resolution and layout */
    oled_send_cmd(OLED_SET_DISP_START_LINE);        // set display start line to 0

    oled_send_cmd(OLED_SET_MUX_RATIO);              // set multiplex ratio
    oled_send_cmd(OLED_HEIGHT - 1);                 // our display is 64 pixels high

    oled_send_cmd(OLED_SET_SEG_REMAP | 0x01);       // set segment re-map (A0/A1)
                                                    // A0 = column address 0 is mapped to SEG0
                                                    // A1 = column address 127 is mapped to SEG0

    oled_send_cmd(OLED_SET_COM_OUT_DIR | 0x08);     // set COM (common) output scan direction (C0/C8)
                                                    // C0 = scan from top down, COM0 to COM[N-1]
                                                    // C8 = scan from bottom up, COM[N-1] to COM0

    oled_send_cmd(OLED_SET_DISP_OFFSET);            // set display offset
    oled_send_cmd(0x00);                            // no offset

    oled_send_cmd(OLED_SET_COM_PIN_CFG);            // set COM (common) pins hardware configuration
    oled_send_cmd(0x10 | 0x02);                     // 00xx0010
                                                    // xx:00 = sequential rows, no left/right remap
                                                    // xx:01 = interleave rows, no left/right remap
                                                    // xx:10 = sequential rows, left/right remap
                                                    // xx:11 = interleave rows, left/right remap

    /* timing and driving scheme */
    oled_send_cmd(OLED_SET_DISP_CLK_DIV);           // set display clock divide ratio
    oled_send_cmd(0x80);                            // div ratio of 1, standard freq

    oled_send_cmd(OLED_SET_PRECHARGE);              // set pre-charge period
    oled_send_cmd(0xF1);                            // Vcc internally generated on our board

    oled_send_cmd(OLED_SET_VCOM_DESEL);             // set VCOMH deselect level
    oled_send_cmd(0x30);                            // 0.83xVcc

    /* display */
    oled_send_cmd(OLED_SET_CONTRAST);               // set contrast control (0-255)
    oled_send_cmd(0xA0);

    oled_send_cmd(OLED_SET_ENTIRE_ON);              // set entire display on to follow RAM content

    oled_send_cmd(OLED_SET_NORM_INV);               // set normal (not inverted) display

    oled_send_cmd(OLED_SET_CHARGE_PUMP);            // set charge pump
    oled_send_cmd(0x14);                            // Vcc internally generated on our board

    oled_send_cmd(OLED_SET_SCROLL | 0x00);          // deactivate horizontal scrolling if set
        // this is necessary as memory writes will corrupt if scrolling is enabled

    oled_send_cmd(OLED_SET_DISP | 0x01);            // turn display on

    // initialize render area for entire display (128 pixels by 8 pages)
    calc_render_area_buflen(&display_full_area);

    // zero the entire display
    oled_disp_fill(oled_disp_buf, 0x00);
    oled_disp_render(oled_disp_buf, &display_full_area);

    // intro sequence: flash the screen twice
    for (int i = 0; i < 2; i++) {
        oled_send_cmd(0xA5); // ignore RAM, all pixels on
        sleep_ms(100);
        oled_send_cmd(0xA4); // go back to following RAM
        sleep_ms(100);
    }
}

void oled_disp_render(uint8_t *buf, render_area_t *area) {
    // update a portion of the display with a render area
    oled_send_cmd(OLED_SET_COL_ADDR);
    oled_send_cmd(area->start_col);
    oled_send_cmd(area->end_col);

    oled_send_cmd(OLED_SET_PAGE_ADDR);
    oled_send_cmd(area->start_page);
    oled_send_cmd(area->end_page);

    oled_write_buf(buf, area->buflen);
}

/*! @brief Scroll display horizontally
 *  \ingroup oled1306_i2c
 *
 *  Scroll the full display from right to left.
 */
void oled_disp_scroll_horz(void) {
    oled_send_cmd(OLED_SET_HORIZ_SCROLL | 0x00);
    oled_send_cmd(0x00); // dummy byte
    oled_send_cmd(0x00); // start page 0
    oled_send_cmd(0x00); // time interval
    oled_send_cmd(0x07); // end page 7
    oled_send_cmd(0x00); // dummy byte
    oled_send_cmd(0xFF); // dummy byte

    oled_send_cmd(OLED_SET_SCROLL | 0x01);
}

// ################################################################################################
// print_xxx convenience methods for printing out a buffer to be rendered
// mostly useful for debugging images, patterns, etc

void oled_disp_print_buf_page(uint8_t buf[], uint8_t page) {
    // disp_prints one page of a full length (128x4) buffer
    for (int j = 0; j < OLED_PAGE_HEIGHT; j++) {
        for (int k = 0; k < OLED_WIDTH; k++) {
            printf("%u", (buf[page * OLED_WIDTH + k] >> j) & 0x01);
        }
        printf("\n");
    }
}

void oled_disp_print_buf_pages(uint8_t buf[]) {
    // disp_prints all pages of a full length buffer
    for (int i = 0; i < OLED_NUM_PAGES; i++) {
        printf("--page %d--\n", i);
        oled_disp_print_buf_page(buf, i);
    }
}

void oled_disp_print_buf_area(uint8_t *buf, render_area_t *area) {
    // print a render area of generic size
    int area_width = area->end_col - area->start_col + 1;
    int area_height = area->end_page - area->start_page + 1; // in pages, not pixels
    for (int i = 0; i < area_height; i++) {
        for (int j = 0; j < OLED_PAGE_HEIGHT; j++) {
            for (int k = 0; k < area_width; k++) {
                printf("%u", (buf[i * area_width + k] >> j) & 0x01);
            }
            printf("\n");
        }
    }
}
