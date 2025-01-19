/**
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#include "touch.h"

#include "cmt/cmt.h"
#include "gfx/gfx.h"
#include "board.h"
#include "spi_ops.h"

#include "pico/float.h"
#include "pico/time.h"
#include "pico/stdlib.h"

#include <stdlib.h>

/**
 * @brief Message used to signal a panel touch.
 */
static cmt_msg_t _msg_touch;

/**
 * @brief The configuration to be used for subsequent reads.
 */
static tp_config_t _config;

/**
 * @brief Bounds (x/y min/max) observed from the panel
 */
static gfx_rect _bounds;

/**
 * @brief Display point last measured.
 */
static gfx_point _display_point;

/**
 * @brief Touch panel (raw) point last measured.
 */
static gfx_point _panel_point;

/**
 * @brief Touch force value last measured.
 */
static uint32_t _touch_force = 0;

/**
 * Set the chip select for the touch panel.
 *
 */
static void _cs(bool sel) {
    if (sel) {
        spi_touch_select();
    }
    else {
        spi_none_select();
    }
}


static void _op_begin() {
    spi_touch_begin();
    _cs(true);
}

static void _op_end() {
    _cs(false);
    spi_touch_end();
}


const gfx_rect* tp_bounds_observed() {
    return &_bounds;
}

const gfx_point* tp_check_display_point() {
    gfx_point *retval = NULL;
    const gfx_point *pp = tp_check_panel_point();

    // If the panel is being touched, calculate the display point from the panel (raw) point.
    if (pp) {
        int px = _max(pp->x - _config.x_min, 0);
        int py = _max(pp->y - _config.y_min, 0);
        int ax = float2int(int2float(px) / _config.fx);
        int ay = float2int(int2float(py) / _config.fy);
        int x = _min(ax, _config.display_width);
        int y = _min(ay, _config.display_height);
        if (_config.invert_x) {
            x = _config.display_width - x;
        }
        if (_config.invert_y) {
            y = _config.display_height - y;
        }
        _display_point.x = x;
        _display_point.y = y;
        retval = &_display_point;
    }

    return (retval);
}

const gfx_point* tp_check_panel_point() {
    gfx_point* retval = NULL;

    int x = tp_read_adc12_trimmed_mean(TP_ADC_SEL_X);
    int y = tp_read_adc12_trimmed_mean(TP_ADC_SEL_Y);
    _panel_point.x = x;
    _panel_point.y = y;
    if (x > 0 && y > 0) {
        retval = &_panel_point;
        // Add the point into our observed bounds
        gfx_bounds_add_point(&_bounds, &_panel_point);
    }

    return (retval);
}

/*
 * From the XPT2046 Datasheet:
 *
 * Rf = Rx * (Xpos/4096) * ((F2/F1)-1)
 * The force will be proportional to the inverse of the resistance.
 */
uint32_t tp_check_touch_force() {
    int fl;
    float f, r, f1, f2, x;

    x = int2float(tp_read_adc12(TP_ADC_SEL_X));
    f1 = int2float(tp_read_adc12(TP_ADC_SEL_F1));
    f2 = int2float(tp_read_adc12(TP_ADC_SEL_F2));

    r = (x/4096.0) * ((f2/f1) - 1.0);
    f = 1.0 / r;
    fl = labs(float2int(f - 20000.0));
    _touch_force = fl;

    return (_touch_force);
}

const tp_config_t* tp_config() {
    return &_config;
}

const gfx_point* tp_last_display_point() {
    return (&_display_point);
}

const gfx_point* tp_last_panel_point() {
    return (&_panel_point);
}

uint32_t tp_last_touch_force() {
    return (_touch_force);
}

uint8_t tp_read_adc8(tsc_adc_sel_t adc) {
    tsc_adc_sel_t addr = adc & TP_CTRL_BITS_ADC_SEL; // Assure that we only have the address bits
    // Generate the full command from our config and the address
    uint8_t cmd = TP_CMD | addr | TP_RESOLUTION_8BIT | TP_REF_DIFFERENTIAL | TP_PD_ON_W_IRQ;
    uint8_t adcbyte;

    // Send the command to the touch controller and read the value.
    _op_begin();
    {
        spi_touch_write8(cmd);
        spi_touch_read_buf(SPI_LOW_TXD_FOR_READ, &adcbyte, 1);
    }
    _op_end();

    return (adcbyte);
}

uint16_t tp_read_adc12(tsc_adc_sel_t adc) {
    tsc_adc_sel_t addr = adc & TP_CTRL_BITS_ADC_SEL; // Assure that we only have the address bits
    uint8_t cmd = TP_CMD | addr | TP_RESOLUTION_12BIT | TP_REF_DIFFERENTIAL | TP_PD_ON_W_IRQ;
    uint8_t adcbytes[2];

    // Send the command to the touch controller and read the value.
    _op_begin();
    {
        spi_touch_write8(cmd);
        spi_touch_read_buf(SPI_LOW_TXD_FOR_READ, adcbytes, 2);
    }
    _op_end();

    uint16_t adcval = ((adcbytes[0] << 8) | adcbytes[1]);
    adcval >>= 4; // Bottom 12 bits are valid

    return (adcval);
}

uint8_t tp_read_adc8_trimmed_mean(tsc_adc_sel_t adc) {
    int samples = _config.smpl_size; // `smpl_size` is forced to be >=3 when set
    int retval = 0;
    int accum = 0;
    int high = 0;
    int low = ~0;

    for (int i = 0; i < samples; i++) {
        uint8_t v = tp_read_adc8(adc);
        accum += v;
        high = (high > v ? high : v);
        low = (low < v ? low : v);
    }
    accum -= (high + low);
    float mean = (int2float(accum) / int2float(samples - 2));
    retval = float2int(mean);

    return (retval);
}

uint16_t tp_read_adc12_trimmed_mean(tsc_adc_sel_t adc) {
    int samples = _config.smpl_size; // `smpl_size` is forced to be >=3 when set
    int retval = 0;
    int accum = 0;
    int high = 0;
    int low = ~0;

    for (int i = 0; i < samples; i++) {
        uint16_t v = tp_read_adc12(adc);
        accum += v;
        high = (high > v ? high : v);
        low = (low < v ? low : v);
    }
    accum -= (high + low);
    float mean = (int2float(accum) / int2float(samples - 2));
    retval = float2int(mean);

    return (retval);
}

//=================================================================================================

void tp_irq_handler(uint gpio, uint32_t events) {
    if (events & GPIO_IRQ_EDGE_FALL) {
        const gfx_point* p = tp_check_display_point();
        if (p) {
            // Read the force so that the last force value is updated.
            uint16_t f = tp_check_touch_force();
            debug_printf("Touch Force: %u\n", f);
            // Post a message with the display point
            // ZZZ _msg_touch.data.touch_point = p;
            postDCSMsg(&_msg_touch);
        }
    }
}

void tp_module_init(int sample_size, uint16_t display_width, bool invert_x, uint16_t display_height, bool invert_y, uint16_t panel_min_x, uint16_t panel_max_x, uint16_t panel_min_y, uint16_t panel_max_y) {
    _config.smpl_size = (sample_size > 3 ? sample_size : 3); // 3 minimum to allow for a trimmed mean
    _config.display_width = display_width;
    _config.invert_x = invert_x;
    _config.display_height = display_height;
    _config.invert_y = invert_y;
    _config.x_min = panel_min_x;
    _config.x_max = panel_max_x;
    _config.y_min = panel_min_y;
    _config.y_max = panel_max_y;

    // Start the Bounds at a reasonable point to expand on.
    int xd = panel_max_x - panel_min_x;
    int yd = panel_max_y - panel_min_y;
    _bounds.p1.x = xd / 2;
    _bounds.p1.y = yd / 2;
    _bounds.p2.x = xd / 2;
    _bounds.p2.y = yd / 2;
    // Calculate the X and Y factors
    float fx = int2float(xd) / int2float(display_width);
    float fy = int2float(yd) / int2float(display_height);
    _config.fx = fx;
    _config.fy = fy;

    cmt_msg_init(&_msg_touch, MSG_TOUCH_PANEL);

    // Perform a 'throw away' read to get the controller set up for
    // subsequent operations.
    tp_read_adc12(TP_ADC_SEL_F1);
}
