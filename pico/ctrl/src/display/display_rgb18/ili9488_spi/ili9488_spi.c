/**
 * ILI9488 TFT LCD functionality interface through SPI
 * (This also works for the ILI9486 controller, at least for the functionality we use.)
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
 * Using ILI9488 4-Line Serial Interface II (see datasheet pg 63)
 * 64k Color (16bit) Mode
 */
#include "pico/stdlib.h"

#include "system_defs.h"    // This would need to change for general purpose use
#include "../ili_lcd_spi.h"
#include "ili9488_spi.h"

const uint8_t ili9488_init_cmd_data[] = {
    // CMD, ARGC, ARGD...
    // (if ARGC bit 8 set, delay after sending command)
    // ILI_EC_PWCTL1  , 1, 0x23,              // Grayscale voltage level (GVDD): 3.8v
    // ILI_EC_PWCTL2  , 1, 0x13,              // AVDD=VCIx2 VGH=VCIx6 VGL=-VCIx3
    // ILI_EC_VMCTL1  , 2, 0x3e, 0x28,        // VCOM Voltage: VCH=4.25v VCL:-1.5v
    // ILI_EC_VMCTL2  , 1, 0x86,              // VCOM Offset: Enabled, VMH-58, VML-58
    ILI_MADCTL  , 1, 0x48,              // Memory Access Control: YX=Invert, Portrait, BGR, Vrev, Hnorm
    ILI_CASET   , 4, 0x00,              // Column Addr Set: SC=0 EC=013F (319) for 320 columns
                        0x00,
                        0x01,
                        0x3F,
    ILI_PASET   , 4, 0x00,              // Page Addr Set: SP=0 EP=01DF (479) for 480 rows
                        0x00,
                        0x01,
                        0xDF,
    ILI_VSCRSADD, 2, 0x00, 0x00,        // Vertical Scroll Start: 0x0000
    ILI_PIXFMT  , 1, 0x66,              // Pixel Format: 18 RGB 6,6,6 bits
    // ILI_EC_FRMCTL1 , 2, 0x00, 0x1B,        // Frame Ctl (Normal Mode): fosc/1, rate=70Hz
    // ILI_EC_DFUNCTL , 4, 0x08,              // Function Ctl: Non-Disp Interval Scan, V63/V0, VCH/VCL,
    //                  0x82,              //  LCD=Normally White, ScanG=0-320, ScanS=0-720, ScanMode=T-B, Int=5fms
    //                  0x27,              //  LCD Interval Drive Lines=192
    //                  0x00,              //  PCDIV (clock divider factor)=0
    // ILI_EC_GAMMASET , 1, 0x01,             // Gamma curve 1 selected
    // ILI_EC_GMCTLP1 , 15, 0x0F, 0x31,       // Set Gamma Positive correction
    //     0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
    //     0x37, 0x07, 0x10, 0x03, 0x0E, 0x09,
    //     0x00,
    // ILI_EC_GMCTLN1 , 15, 0x00, 0x0E,       // Set Gamma Negative correction
    //     0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
    //     0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36,
    //     0x0F,
    ILI_SLPOUT  , 0x80,                 // Exit Sleep
    ILI_DISPON  , 0x80,                 // Display on
    0x00                                // End of list
};

