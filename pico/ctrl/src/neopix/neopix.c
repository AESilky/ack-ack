/**
 * @brief Neopixel Panel Display Driver.
 * @ingroup neopixel
 *
 * Provides graphics display on two 4x8 Neopixel panels.
 *
 * This could be made more generic, but the 4x8 panel is readily available
 * from Adafruit as a 'FeatherWing'. Restricting to the 4x8 panel and RGB
 * helps keep things much simpler.
 *
 * The idea is to provide two 'eyes' that can show expressions. Of course,
 * anything that can fit on two 4x8 panels can be displayed. This module
 * uses two consecutive 'frame buffers' and the DMA to update the two display panels.
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#include "neopix.h"

#include "system_defs.h"
#include "board.h"
#include "cmt/cmt.h"

#include "pico/stdlib.h"
#include "hardware/dma.h" //DMA is used to move the frame buffer to the PIO
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "generated/ws2812.pio.h"

#include <stdlib.h>

static bool _initialized = false;
static int _dma_fbuf;
static int _dma_copy;

static void _framebuf_to_disp();
static void _disp_eye(void* data);

static uint32_t __attribute__((aligned(256))) _frame_buf[NEOPIX_FRAME_BUF_ELEMENTS];

/* Pattern of words of GRB values 8 x 4 */
static uint32_t __attribute__((aligned(256))) _eye_pat0[] = {
    // Open eye
    0x00000000, 0x4F221400, 0x40221400, 0x40221400, 0x30201000, 0x00000000, 0x00000000, 0x00000000,
    0x4F281700, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x2A1A0A00, 0x00000000, 0x00000000,
    0x00000000, 0x20108000, 0x20108000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x10104000, 0x20108000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
};
static uint32_t __attribute__((aligned(256))) _eye_pat1[] = {
    // Eyelid top closing #1
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x4F221400, 0x40221400, 0x40221400, 0x30201000, 0x00000000, 0x00000000, 0x00000000,
    0x4F281700, 0x20108000, 0x20108000, 0x00000000, 0x00000000, 0x2A1A0A00, 0x00000000, 0x00000000,
    0x00000000, 0x10104000, 0x20108000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
};
static uint32_t __attribute__((aligned(256))) _eye_pat2[] = {
    // Eyelid top closing #2
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x4F281700, 0x4F221400, 0x4F221400, 0x4F221400, 0x30201000, 0x2A1A0A00, 0x00000000, 0x00000000,
    0x00000000, 0x10104000, 0x3F020000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
};
static uint32_t __attribute__((aligned(256))) _eye_pat3[] = {
    // Eye closed
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x4F221400, 0x4F221400, 0x4F221400, 0x30201000, 0x2A1A0A00, 0x00000000, 0x00000000,
    0x4F281700, 0x3F020000, 0x3F020000, 0x2F010000, 0x1F000000, 0x00000000, 0x00000000, 0x00000000,
};

static uint32_t* _eye_pattern[] = { _eye_pat0, _eye_pat1, _eye_pat2, _eye_pat3 };


/**
 * @brief Copy a block of memory into the Frame Buffer. Optionally display it.
 * @ingroup neopix
 *
 * Copy a two panel image into the active frame buffer to be displayed.
 * Optionally, trigger the FrameBuffer DMA to display the frames. The source
 * buffer MUST be 2 x 32 words in length.
 *
 * @param src The two panel image to move into the frame buffer.
 * @param display True to cause the image to be displayed.
 */
static void _copy_to_framebuf(const uint32_t* src) {
    dma_channel_set_write_addr(_dma_copy, &_frame_buf, false);
    dma_channel_set_read_addr(_dma_fbuf, &_frame_buf, false);
    dma_channel_transfer_from_buffer_now(_dma_copy, src, NEOPIX_FRAME_BUF_ELEMENTS);
}

static void _framebuf_to_disp() {
    dma_channel_transfer_from_buffer_now(_dma_fbuf, &_frame_buf, NEOPIX_FRAME_BUF_ELEMENTS);
}


/*
 * Display an eye that is open and then blinks.
 */
typedef struct eye_state_ {
    int blink_state;
    bool opening;
} eye_state_t;

void _disp_eye_blink(void* data) {
    int speed = 20;
    eye_state_t* eye_state = (eye_state_t*)data;
    if (eye_state->opening) {
        speed += (20 + (rand() % 100));
        eye_state->blink_state -= 1;
        if (eye_state->blink_state <= 0) {
            _disp_eye(NULL);
            return;
        }
    }
    else {
        speed += (rand() % 50);
        eye_state->blink_state += 1;
        if (eye_state->blink_state > 3) {
            eye_state->opening = true;
            eye_state->blink_state = 3;
            speed *= 2;
        }
    }
    _copy_to_framebuf(_eye_pattern[eye_state->blink_state]);
    cmt_sleep_ms(speed, _disp_eye_blink, data);
}
void _disp_eye(void* data) {
    // Eye open
    static eye_state_t eye_state_;
    static bool move_eye_right = true;
    eye_state_.opening = false;
    eye_state_.blink_state = 0;
    // Possibly, move the eye a bit...
    if (rand() % 3 == 0) {
        // Move the 'eye' a bit, or move it back;
        if (move_eye_right) {
            *(_eye_pat0+19) = *(_eye_pat0+17);
            *(_eye_pat0+17) = 0;
            *(_eye_pat0 + 27) = *(_eye_pat0 + 25);
            *(_eye_pat0 + 25) = 0;
            move_eye_right = false;
        }
        else {
            *(_eye_pat0+17) = *(_eye_pat0+19);
            *(_eye_pat0+19) = 0;
            *(_eye_pat0 + 25) = *(_eye_pat0 + 27);
            *(_eye_pat0 + 27) = 0;
            move_eye_right = true;
        }
    }
    _copy_to_framebuf(_eye_pat0);
    int open_time = 800 + (rand() % 7000);
    cmt_sleep_ms(open_time, _disp_eye_blink, &eye_state_);
}


void neopix_start(void) {
    _disp_eye(NULL);
}

void neopix_module_init(void) {
    if (_initialized) {
        board_panic("neopix module already initialized!");
    }
    _initialized = true;
    uint offset;

    // Load the PIO program
    offset = pio_add_program(PIO_NEOPIX_BLOCK, &ws2812_program);
    if (offset < 0) {
        board_panic("ws2312_main - Unable to load PIO program");
    }
    // Initialize the PIO that feeds the data to the Neopixel.
    ws2812_program_init(PIO_NEOPIX_BLOCK, PIO_NEOPIX_SM, offset, NEOPIXEL_DRIVE, 800000, false);
    // Initialize the DMA that moves data from the frame buffers to the PIO,
    // and from a source buffer to the frame buffer.
    _dma_fbuf = dma_claim_unused_channel(true);
    _dma_copy = dma_claim_unused_channel(true);
    //
    // Init the Frame Buffer DMA to write the frame buffer to the PIO
    dma_channel_config c1 = dma_channel_get_default_config(_dma_fbuf); //Get configurations for the frame-buffer channel
    channel_config_set_transfer_data_size(&c1, DMA_SIZE_32); //Set frame-buffer channel data transfer size to 8 bits
    channel_config_set_read_increment(&c1, true); // Frame-buffer channel read increment to true (advance through buffer)
    channel_config_set_write_increment(&c1, false); // Frame-buffer channel write increment to false (write to PIO)
    channel_config_set_dreq(&c1, PIO_NEOPIX_DREQ); //Set the transfer request signal to the PIO-SM tx-fifo empty.
    channel_config_set_ring(&c1, false, 8); //Set read address wrapping to 8 bits (256 bytes - (2 * (4x8 * 4)))
    //
    // Init the Frame Buffer copy DMA
    dma_channel_config c2 = dma_channel_get_default_config(_dma_copy); //Get configurations for the frame-copy channel
    channel_config_set_transfer_data_size(&c2, DMA_SIZE_32); //Set transfer size to 32 bits
    channel_config_set_read_increment(&c2, true); // Frame-buffer channel read increment to true (advance through source)
    channel_config_set_write_increment(&c2, true); // Frame-buffer channel write increment to true (advance through frame buffer)
    channel_config_set_chain_to(&c2, _dma_fbuf);  // Once the copy to the frame-buf is done, trigger the frame-buf DMA
    //
    // Configure frame-buffer channel to write to the PIO driving the panel
    dma_channel_configure(_dma_fbuf, &c1,
        &PIO_NEOPIX_BLOCK->txf[PIO_NEOPIX_SM],      // Destination
        _frame_buf,                                 // Memory buffer to read from
        NEOPIX_FRAME_BUF_ELEMENTS,                  // Number of pixels to transfer in one block
        false);                                     // Don't start yet
    //
    // Configure buffer transfer to write from the source to the frame-buffer
    dma_channel_configure(_dma_copy, &c2,
        _frame_buf,                                 // Destination
        _frame_buf,                                 // (updated for each transfer)
        NEOPIX_FRAME_BUF_ELEMENTS,                  // Number of pixels to transfer in one block
        false);                                     // Don't start yet
}


