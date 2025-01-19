/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include "system_defs.h"
#include "board.h"
#include "cmt/cmt.h"

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "generated/ws2812.pio.h"

void disp_eye(void* data);
void ws2312_run(void* data);

/**
 * NOTE:
 *  Take into consideration if your WS2812 is a RGB or RGBW variant.
 *
 *  If it is RGBW, you need to set IS_RGBW to true and provide 4 bytes per
 *  pixel (Red, Green, Blue, White) and use urgbw_u32().
 *
 *  If it is RGB, set IS_RGBW to false and provide 3 bytes per pixel (Red,
 *  Green, Blue) and use urgb_u32().
 *
 *  When RGBW is used with urgb_u32(), the White channel will be ignored (off).
 *
 */
#define IS_RGBW false
#define NUM_PIXELS (4*8)

static int t = 0;
static int pattern_indx;
static PIO pio;
static uint sm;

static inline void put_pixel(PIO pio, uint sm, uint32_t pixel_grb) {
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return
        ((uint32_t)(r) << 8) |
        ((uint32_t)(g) << 16) |
        (uint32_t)(b);
}

static inline uint32_t urgbw_u32(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
    return
        ((uint32_t)(r) << 8) |
        ((uint32_t)(g) << 16) |
        ((uint32_t)(w) << 24) |
        (uint32_t)(b);
}

/* Pattern of words of GRB values 8 x 4 */
static uint32_t _eye_pat0[] = {
    // Open eye
    0x000000, 0x4F2214, 0x402214, 0x402214, 0x302010, 0x000000, 0x000000, 0x000000,
    0x4F2817, 0x000000, 0x000000, 0x000000, 0x000000, 0x2A1A0A, 0x000000, 0x000000,
    0x000000, 0x4030F0, 0x4030F0, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x202080, 0x4030F0, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000
};
static uint32_t _eye_pat1[] = {
    // Eyelid top closing #1
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x4F2214, 0x402214, 0x402214, 0x302010, 0x000000, 0x000000, 0x000000,
    0x4F2817, 0x4030F0, 0x4030F0, 0x000000, 0x000000, 0x2A1A0A, 0x000000, 0x000000,
    0x000000, 0x202080, 0x4030F0, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
};
static uint32_t _eye_pat2[] = {
    // Eyelid top closing #2
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x4F2817, 0x4F2214, 0x4F2214, 0x4F2214, 0x302010, 0x2A1A0A, 0x000000, 0x000000,
    0x000000, 0x202080, 0x4030F0, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
};
static uint32_t _eye_pat3[] = {
    // Eye closed
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x4F2214, 0x4F2214, 0x4F2214, 0x302010, 0x2A1A0A, 0x000000, 0x000000,
    0x4F2817, 0x2F0214, 0x2F0214, 0x2F0214, 0x1F0204, 0x000000, 0x000000, 0x000000,
};

static uint32_t* _eye_pattern[] = {_eye_pat0, _eye_pat1, _eye_pat2, _eye_pat3};

static void _disp_mem_pattern(PIO pio, uint sm, uint32_t *data, uint len) {
    for (uint i = 0; i < len; i++) {
        uint32_t grb = *(data + i);
        put_pixel(pio, sm, grb);
    }
}

void pattern_snakes(PIO pio, uint sm, uint len, uint t) {
    for (uint i = 0; i < len; ++i) {
        uint x = (i + (t >> 1)) % 64;
        if (x < 10)
            put_pixel(pio, sm, urgb_u32(0xff, 0, 0));
        else if (x >= 15 && x < 25)
            put_pixel(pio, sm, urgb_u32(0, 0xff, 0));
        else if (x >= 30 && x < 40)
            put_pixel(pio, sm, urgb_u32(0, 0, 0xff));
        else
            put_pixel(pio, sm, 0);
    }
}

void pattern_random(PIO pio, uint sm, uint len, uint t) {
    if (t % 8)
        return;
    for (uint i = 0; i < len; ++i)
        put_pixel(pio, sm, rand());
}

void pattern_sparkle(PIO pio, uint sm, uint len, uint t) {
    if (t % 8)
        return;
    for (uint i = 0; i < len; ++i)
        put_pixel(pio, sm, rand() % 16 ? 0 : 0xffffffff);
}

void pattern_greys(PIO pio, uint sm, uint len, uint t) {
    uint max = 100; // let's not draw too much current!
    t %= max;
    for (uint i = 0; i < len; ++i) {
        put_pixel(pio, sm, t * 0x10101);
        if (++t >= max) t = 0;
    }
}

typedef void (*pattern)(PIO pio, uint sm, uint len, uint t);
const struct {
    pattern pat;
    const char* name;
} pattern_table[] = {
        {pattern_snakes,  "Snakes!"},
        {pattern_random,  "Random data"},
        {pattern_sparkle, "Sparkles"},
        {pattern_greys,   "Greys"},
};

/*
 * Display an eye that is open and then blinks.
 */
typedef struct eye_state_ {
    int blink_state;
    bool opening;
} eye_state_t;
void disp_eye_blink(void* data) {
    int speed = 20;
    eye_state_t* eye_state = (eye_state_t*)data;
    if (eye_state->opening) {
        speed += 80 + (rand() % 200);
        eye_state->blink_state -= 1;
        if (eye_state->blink_state <= 0) {
            disp_eye(NULL);
            return;
        }
    }
    else {
        speed += 10 + (rand() % 50);
        eye_state->blink_state += 1;
        if (eye_state->blink_state > 3) {
            eye_state->opening = true;
            eye_state->blink_state = 3;
        }
    }
    _disp_mem_pattern(pio, sm, _eye_pattern[eye_state->blink_state], 4 * 8);
    cmt_sleep_ms(speed, disp_eye_blink, data);
}
void disp_eye(void* data) {
    // Eye open
    static eye_state_t eye_state_;
    _disp_mem_pattern(pio, sm, _eye_pat0, 4 * 8);
    eye_state_.blink_state = 0;
    eye_state_.opening = false;
    int open_time = 800 + (rand() % 7000);
    cmt_sleep_ms(open_time, disp_eye_blink, &eye_state_);
}

void ws2312_main() {
    pio = PIO_NEOPIX_BLOCK;
    sm = PIO_NEOPIX_SM;
    uint offset;

    // Load the PIO program
    offset = pio_add_program(pio, &ws2812_program);
    if (offset < 0) {
        board_panic("ws2312_main - Unable to load PIO program");
    }

    ws2812_program_init(pio, sm, offset, NEOPIXEL_DRIVE, 800000, IS_RGBW);

    pattern_indx = 0;
    //ws2312_run(&pattern_indx);
    disp_eye(NULL);
}

void ws2312_run(void* data) {
    int i = *(int*)data;
    int pat = rand() % count_of(pattern_table);
    int dir = (rand() >> 30) & 1 ? 1 : -1;
    puts(pattern_table[pat].name);
    puts(dir == 1 ? "(forward)" : "(backward)");
    i++;
    if (i >= 1000) {
        i = 0;
        t += dir;
    }
    pattern_table[pat].pat(pio, sm, NUM_PIXELS, t);
    pattern_indx = i;
    cmt_sleep_ms(50, ws2312_run, &pattern_indx);
}
