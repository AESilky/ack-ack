/**
 * @brief Touch panel functionality.
 * @ingroup touch_panel
 *
 * This defines the touch-panel functionality for a panel that uses the
 * XPT2046/TI-ADS7843 touch controller.
 *
 * Examples are the QDTech-TFT-LCD display modules.
 *
 * Copyright 2023-24 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef _TOUCH_H_
#define _TOUCH_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "gfx/gfx.h"

#include "pico/types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


#define TP_CTRL_BITS_CMD            0x80 // CONTROL BYTE bit 7 indicates a command.
#define TP_CTRL_BITS_ADC_SEL        0x70 // CONTROL BYTE bits 6-4 address the ADC multiplexer
#define TP_CTRL_BITS_RESOLUTION     0x08 // CONTROL BYTE bit 3 controls the measurement resolution (8/12 bit).
#define TP_CTRL_BITS_REF_TYPE       0x04 // CONTROL BYTE bit 2 controls the reference type (single/double ended)
#define TP_CTRL_BITS_PWRDWN_MODE    0x03 // CONTROL BYTE bits 1-0 set the power-down mode.

/**
 * @brief Touch Panel Controller CONTROL BYTE.
 * @ingroup touch_panel
 *
 * The CONTROL BYTE consists of 5 fields
 * COMMAND        | Bit 7   | 1=Command, 0=NOP
 * ADC ADDRESS    | Bit 6:4 | 000=NONE, 001=Y, 010=IN3(VBAT), 011=Z1, 100=Z2, 101=X, 110=IN4(AUX), 111=res
 * RESOLUTION     | Bit 3   | 1=8 bit, 0=12 bit
 * REFERENCE TYPE | Bit 2   | 1=Single-Ended, 0=Differential
 * PWR DOWN MODE  | Bit 1:0 | 00=PD between measurements with IRQ enabled, 01=As 0 w/IRQ disabled, 10=res, 11=No PD
 */
typedef uint8_t tp_ctrl_t;

/**
 * @brief Touch Panel Controller CONTROL BYTE bit 7 indicates a command.
 * @ingroup touch_panel
 */
#define TP_CMD 0x80

/**
 * @brief Touch Panel Controller CONTROL BYTE bits 6-4 address the ADC multiplexer
 * @ingroup touch_panel
 */
typedef enum _tsc_adc_sel_ {
    TP_ADC_SEL_NONE = 0x00,  // No ADC
    TP_ADC_SEL_X    = 0x50,  // X Measurement
    TP_ADC_SEL_Y    = 0x10,  // Y Measurement
    TP_ADC_SEL_F1   = 0x30,  // Touch Force 1
    TP_ADC_SEL_F2   = 0x40,  // Touch Force 2
} tsc_adc_sel_t;

/**
 * @brief Touch Panel Controller CONTROL BYTE bit 3 controls the measurement resolution.
 * (8 bit or 12 bit)
 * @ingroup touch_panel
 *
 */
typedef enum _tsc_resolution_ {
    TP_RESOLUTION_12BIT = 0x00,
    TP_RESOLUTION_8BIT  = 0x08,
} tsc_resolution_t;

/**
 * @brief Touch Panel Controller CONTROL BYTE bit 2 controls the reference type.
 * (single-ended or differential)
 * @ingroup touch_panel
 */
typedef enum _tsc_ref_type_ {
    TP_REF_SINGLE_ENDED = 0x04,
    TP_REF_DIFFERENTIAL = 0x00,
} tsc_ref_type_t;

/**
 * @brief Touch Panel Controller CONTROL BYTE bits 1-0 set the power-down mode.
 * @ingroup touch_panel
 */
typedef enum _tsc_pwrdwn_mode_ {
    TP_PD_OFF         = 0x03,    // No power-down between conversions.
    TP_PD_ON_W_IRQ    = 0x00,    // Power-down between conversions with the pen interrupt enabled.
    TP_PD_ON_WO_IRQ   = 0x01,    // Power-down between conversions with the pen interrupt disabled.
} tsc_pwrdwn_mode_t;

typedef struct _tp_config {
    int smpl_size;
    uint16_t display_height;
    uint16_t display_width;
    long x_factor;
    int16_t x_offset;
    long y_factor;
    int16_t y_offset;
} tp_config_t;

/**
 * @brief Configuration used for reads from the touch panel controller.
 * @ingroup touch_panel
 *
 * This controls the type, resolution, and other parameters used to read a value
 * from the touch panel.
 *
 * This **MUST** be called before the first read.
 *
 * This can also be called at any later point, and the new config will be used
 * for subsequent reads.
 *
 * @param sample_size The number of times to sample when reading a touch. Must be at least 3.
 * @param display_height The height of the display in pixels
 * @param display_width The width of the display in pixels
 * @param x_factor  The factor to multiply a panel 'x' value by to get a display 'x' value
 * @param x_offset The 'x' offset between the panel and the display
 * @param y_factor  The factor to multiply a panel 'y' value by to get a display 'y' value
 * @param y_offset The 'y' offset between the panel and the display
 */
extern void tp_module_init(int sample_size, uint16_t display_height, uint16_t display_width, uint64_t x_factor, int16_t x_offset, uint64_t y_factor, int16_t y_offset);

/**
 * @brief Touch interrupt handler. Should be called when the touch panel signals a touch.
 * @ingroup touch_panel
 *
 * When called, this will perform a panel measurement.
 *
 * @param gpio The GPIO number that generated the interrupt.
 * @param events The event type(s).
 */
extern void tp_irq_handler(uint gpio, uint32_t events);

/**
 * @brief Check for a touch on the display. If touched,
 *  return the point on the display corresponding to the touch.
 * @ingroup touch_panel
 *
 * This performs a panel read and will update the 'panel point' if the
 * panel is being touched (as well as the 'touch force' value).
 *
 * @return gfx_point* The point on the display being touched or NULL
 */
extern gfx_point* tp_check_display_point();

/**
 * @brief Check for a touch on the panel. If touched, return the point on the panel.
 * @ingroup touch_panel
 *
 * This performs a force read to determine if the panel is being touched, and
 * therefore, updates the 'touch force' value.
 *
 * @return gfx_point* The point on the touch panel being touched or NULL
 */
extern gfx_point* tp_check_panel_point();

/**
 * @brief Check the force value on the panel.
 * @ingroup touch_panel
 *
 * Reads the force (pressure) of the current touch. If the panel isn't being
 * touched, zero is returned.
 *
 * @return uint16_t The force value.
 */
extern uint16_t tp_check_touch_force();

/**
 * @brief Get the current configuration of the touch screen controller.
 *
 * @return const tp_config_t*
 */
extern const tp_config_t* tp_config();

/**
 * @brief Get the last display point read. This does not perform a read operation.
 * @ingroup touch_panel
 *
 * @return gfx_point* The last read display point
 */
extern gfx_point* tp_last_display_point();

/**
 * @brief Get the last panel (raw) point read. This does not perform a read operation.
 * @ingroup touch_panel
 *
 * @return gfx_point* The last read panel (raw) point
 */
extern gfx_point* tp_last_panel_point();

/**
 * @brief Get the last force value read. This does not perform a read operation.
 * @ingroup touch_panel
 *
 * @return uint16_t The last force value
 */
extern uint16_t tp_last_touch_force();

/**
 * @brief Read the value of one of the ADCs in the controller with 8 bit resolution.
 * @ingroup touch_panel
 *
 * @param adc The address of the ADC to read
 * @return uint8_t The value read (8 bits: 0-255).
 */
extern uint8_t tp_read_adc8(tsc_adc_sel_t adc);

/**
 * @brief Read the value of one of the ADCs in the controller with 12 bit resolution.
 * @ingroup touch_panel
 *
 * @param adc The address of the ADC to read
 * @return uint16_t The value read (12 bits: 0-4095).
 */
extern uint16_t tp_read_adc12(tsc_adc_sel_t adc);

/**
 * @brief Read an ADC multiple times with 8 bit resolution and return the trimmed mean.
 * @ingroup touch_panel
 *
 * The touch panel will return slightly varying values for a given point due to a number
 * of factors affecting the measurement. To help make the values read from the touch
 * panel more consistent, a 'trimmed mean' is used. This takes multiple measurements,
 * removes the low and high values, and averages the remaining values.
 *
 * The number of reads to perform is specified in the `tp_module_init` method.
 *
 * @param adc The ADC number (address) to read.
 * @return uint8_t The trimmed mean result of the readings (8 bits: 0-255).
 */
extern uint8_t tp_read_adc8_trimmed_mean(tsc_adc_sel_t adc);

/**
 * @brief Read an ADC multiple times with 12 bit resolution and return the trimmed mean.
 * @ingroup touch_panel
 *
 * The touch panel will return slightly varying values for a given point due to a number
 * of factors affecting the measurement. To help make the values read from the touch
 * panel more consistent, a 'trimmed mean' is used. This takes multiple measurements,
 * removes the low and high values, and averages the remaining values.
 *
 * The number of reads to perform is specified in the `tp_module_init` method.
 *
 * @param adc The ADC number (address) to read.
 * @return uint16_t The trimmed mean result of the readings (12 bits: 0-4095).
 */
extern uint16_t tp_read_adc12_trimmed_mean(tsc_adc_sel_t adc);

#ifdef __cplusplus
}
#endif
#endif // _TOUCH_H_
