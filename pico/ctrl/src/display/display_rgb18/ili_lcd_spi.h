/**
 * ILI Color LCD functionality interface through SPI
 *
 * Common to ILI9341 and ILI9488
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
 */
#ifndef _ILI_LCD_SPI_H_
#define _ILI_LCD_SPI_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "../display.h"

// 16-bit Color Mode to Basic 16 Colors (similar to PC CGA/EGA/VGA)
//
// 16-bit is R5G6B5 (RGB-16)
// 16-bit from RGB = (R/8)<<11 + (G/4)<<5 + (B/8)
// 16-bit from R5G6B5 = R5*2048 + G6*32 + B5
//
//                                      NUM :   R    G    B  R5 G6 B5
//                                      --- : ---  ---  ---  -- -- --
// #define ILI_BLACK       0x0000    //  0 :   0,   0,   0   0  0  0
// #define ILI_BLUE        0x0011    //  1 :   0,   0, 136   0  0 17
// #define ILI_GREEN       0x4C80    //  2 :  78, 145,   0   9 36  0
// #define ILI_CYAN        0x079E    //  3 :   0, 240, 240   0 60 30
// #define ILI_RED         0xE000    //  4 : 224,   0,   0  28  0  0
// #define ILI_MAGENTA     0xFA1F    //  5 : 255,  64, 255  31 16 31
// #define ILI_BROWN       0x6080    //  6 : 100,  17,   0  12  4  0
// #define ILI_WHITE       0xB5D2    //  7 : 180, 190, 150  22 46 18
// #define ILI_GREY        0x6B49    //  8 : 104, 104,  72  13 26  9
// #define ILI_LT_BLUE     0x033F    //  9 :   0, 100, 255   0 25 31
// #define ILI_LT_GREEN    0x07E0    // 10 :   0, 255,   0   0 63  0
// #define ILI_LT_CYAN     0x77FF    // 11 : 115, 253, 255  14 63 31
// #define ILI_ORANGE      0xFA40    // 12 : 255,  75,   2  31 18  0
// #define ILI_LT_MAGENTA  0xFC5B    // 13 : 255, 138, 218  31 34 27
// #define ILI_YELLOW      0xFFEA    // 12 : 255, 255,  85  31 63 10
// #define ILI_BR_WHITE    0xFFFF    // 15 : 255, 255, 255  31 63 31

// typedef unsigned short rgb16_t; // R5G6B5


// Command descriptions start on page 83 (9341) / 141 (9488) of the datasheet

#define ILI_NOP         0x00    // -  No-op
#define ILI_SWRESET     0x01    // -  Software reset
#define ILI_RDDID       0x04    // -  Read display identification information (3)
#define ILI_RDERRDSI    0x05    // -  Read number of errors on DSI
#define ILI_RDDST       0x09    // -  Read Display Status (2)

#define ILI_RDMODE      0x0A    // -  Read Display Power Mode (1)
#define ILI_RDMADCTL    0x0B    // -  Read Display MADCTL (1)
#define ILI_RDPIXFMT    0x0C    // -  Read Display Pixel Format (1)
#define ILI_RDIMGFMT    0x0D    // -  Read Display Image Format (1)
#define ILI_RDSIGMODE   0x0E    // -  Read Display Signal Mode (1)
#define ILI_RDSELFDIAG  0x0F    // -  Read Display Self-Diagnostic Result (1)

#define ILI_SLPIN       0x10    // -  Enter Sleep Mode
#define ILI_SLPOUT      0x11    // -  Sleep Out
#define ILI_PTLON       0x12    // -  Partial Mode ON
#define ILI_NORON       0x13    // -  Normal Display Mode ON

#define ILI_INVOFF      0x20    // -  Display Inversion OFF
#define ILI_INVON       0x21    // -  Display Inversion ON
//#define ILI_GAMMASET    0x26    // -  Gamma Set
#define ILI_DISPOFF     0x28    // -  Display OFF
#define ILI_DISPON      0x29    // -  Display ON

#define ILI_CASET       0x2A    // -  Column Address Set
#define ILI_PASET       0x2B    // -  Page Address Set
#define ILI_RAMWR       0x2C    // -  Memory Write
//#define ILI_CLRSET      0x2D    // -  Color Set
#define ILI_RAMRD       0x2E    // -  Memory Read

#define ILI_PTLAR       0x30    // -  Partial Area
#define ILI_VSCRDEF     0x33    // -  Vertical Scrolling Definition
#define ILI_TEARELOFF   0x34    // -  Tearing Effect Line OFF
#define ILI_TEARELON    0x35    // -  Tearing Effect Line ON
#define ILI_MADCTL      0x36    // -  Memory Access Control
#define ILI_VSCRSADD    0x37    // -  Vertical Scrolling Start Address
#define ILI_IDLEMODEOFF 0x38    // -  Idle Mode OFF
#define ILI_IDLEMODEON  0x39    // -  Idle Mode ON
#define ILI_PIXFMT      0x3A    // -  COLMOD: Pixel Format Set
#define ILI_MEMWRCONT   0x3C    // -  Memory Write Continue
#define ILI_MEMRDCONT   0x3E    // -  Memory Read Continue

#define ILI_DISPBRT     0x51    // -  Display Brightness Write
#define ILI_RDDISPBRT   0x52    // -  Read Display Brightness

#define ILI_RDID1       0xDA    // -  Read Display Manufacturer ID (1)
#define ILI_RDID2       0xDB    // -  Read Display Version ID (1)
#define ILI_RDID3       0xDC    // -  Read Display Module/Driver ID (1)

// //////////////////////////////////////////////////////////////////////////////////
// Extended Command Set
//
#define ILI_EC_FRMCTL1     0xB1    // -  Frame Rate Control (In Normal Mode/Full Colors)
#define ILI_EC_FRMCTL2     0xB2    // -  Frame Rate Control (In Idle Mode/8 colors)
#define ILI_EC_FRMCTL3     0xB3    // -  Frame Rate control (In Partial Mode/Full Colors)
#define ILI_EC_INVCTL      0xB4    // -  Display Inversion Control
#define ILI_EC_DFUNCTL     0xB6    // -  Display Function Control

#define ILI_EC_PWCTL1      0xC0    // -  Power Control 1
#define ILI_EC_PWCTL2      0xC1    // -  Power Control 2
#define ILI_EC_PWCTL3      0xC2    // -  Power Control 3
#define ILI_EC_PWCTL4      0xC3    // -  Power Control 4
#define ILI_EC_PWCTL5      0xC4    // -  Power Control 5
#define ILI_EC_VMCTL1      0xC5    // -  VCOM Control 1
#define ILI_EC_VMCTL2      0xC7    // -  VCOM Control 2

#define ILI_EC_RDID4       0xD3    // -  Read Display IC (Ver, Model-A, Model-B) (3)

#define ILI_EC_GMCTLP1     0xE0    // -  Positive Gamma Correction
#define ILI_EC_GMCTLN1     0xE1    // -  Negative Gamma Correction
#define ILI_EC_PWCTL6      0xFC    // -  Power Control

/**
 * @brief ILI Controller type.
 * @ingroup display
 */
typedef enum _ili_controller_type {
    ILI_CONTROLLER_NONE = 0,
    ILI_CONTROLLER_9341 = 9341,
    ILI_CONTROLLER_9486 = 9486,
    ILI_CONTROLLER_9488 = 9488,
} ili_controller_type;

/**
 * @brief ILI Display information.
 * @ingroup display
 */
typedef struct _ili_disp_info_ {
    // Display Status (CMD 0x09)
    uint8_t status1;
    uint8_t status2;
    uint8_t status3;
    uint8_t status4;
    // Power Mode (CMD 0x0A)
    uint8_t pwr_mode;
    // MADCTL (CMD 0x0B)
    uint8_t madctl;
    // PIXEL Format (CMD 0x0C)
    uint8_t pixelfmt;
    // Image Format (CMD 0x0D)
    uint8_t imagefmt;
    // Signal Mode (CMD 0x0E)
    uint8_t signal_mode;
    // Self-Diagnostic Result (CMD 0x0F)
    uint8_t selftest;
    // ID1 (MFG CMD 0xDA)
    uint8_t lcd_id1_mfg;
    // ID2 (Version CMD 0xDB)
    uint8_t lcd_id2_ver;
    // ID3 (Driver CMD 0xDB)
    uint8_t lcd_id3_drv;
    // ID4 (IC Ver,Model-l,Model-2 CMD 0xD3)
    uint8_t lcd_id4_ic_ver;
    uint8_t lcd_id4_ic_model1;
    uint8_t lcd_id4_ic_model2;
} ili_disp_info_t;

/**
 * @brief Get information about the display hardare & configuration.
 * @ingroup display
 */
extern ili_disp_info_t* ili_disp_info(void);

/**
 * @brief Initialize the display.
 * @ingroup display
 */
extern ili_controller_type ili_module_init(void);


/**
 * @brief Send a command byte to the controller.
 * @ingroup display
 *
 * Sends a single byte command to the controller.
 * Care should be taken to avoid putting the controller into
 * an invalid/unknown state.
 *
 * @see ILI9341 datasheet section 8 and ILI9488 section 5 for command descriptions.
 *
 * @param cmd The command byte to send (@see ILI_xxx defines)
 */
extern void ili_send_command(uint8_t cmd);

/**
 * @brief Send a command byte and argument data to the controller.
 * @ingroup display
 *
 * Sends a command and additional argument data to the controller.
 * Care should be taken to avoid putting the controller into
 * an invalid/unknown state.

 * @see ILI9341 datasheet section 8 and ILI9488 section 5 for command descriptions.
 *
 * @param cmd The command byte to send (@see ILI_xxx defines)
 * @param data A pointer to a byte buffer of argument data
 * @param count The number of data bytes to send from the buffer
 */
extern void ili_send_command_wd(uint8_t cmd, uint8_t* data, size_t count);

/**
 * @brief Display all of the colors in the palette
 * @ingroup display
 *
 * Run through all of the colors. First do red, then green, then blue.
 * Second, run through all 64k colors, going from 0 to 65,535.
 */
extern void ili_colors_show();


#ifdef __cplusplus
    }
#endif
#endif // _ILI_LCD_SPI_H_