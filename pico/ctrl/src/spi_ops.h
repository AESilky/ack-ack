/**
 * hwctrl SPI operations.
 *
 * The SPI is used by 3 devices:
 *  Display (on HW SPI0)
 *  Touch Panel (on HW SPI1 with the Expansion I/O)
 *  Expansion I/O (on HW SPI1 with Touch Panel)
 *
 * The Display and Touch Panel are on different HW SPI because the speed
 * allowed on the display is significantly faster than that allowed on
 * the touch panel. The expansion is on the
 * read independently from the display being updated.
 *
 * These functions allow the SPIs
 * to be read/written in a coordinated way.
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#ifndef _SPI_OPS_H_
#define _SPI_OPS_H_
#ifdef __cplusplus
 "C" {
#endif
#include "system_defs.h"

#include <stddef.h>
#include <stdbool.h>

typedef enum _SPI_DEVICE_SEL_ {
    SPI_DISPLAY_SELECT     = SPI_DISP_CS_ADDR,
    SPI_TOUCH_SELECT       = SPI_TOUCH_CS_ADDR,
    SPI_EXPANSION_SELECT   = SPI_EXPANSION_CS_ADDR,
    SPI_NONE_SELECT        = SPI_NONE_CS_ADDR
} spi_device_sel_t;

#define SPI_HIGH_TXD_FOR_READ 0xFF
#define SPI_LOW_TXD_FOR_READ 0x00

/**
 * @brief Take control of the SPI bus for use by the display.
 * @ingroup spi_ops
 *
 * Make sure we have control of the SPI for one or more operations.
 * `spi_end` must be called when the SPI is done being used.
 *
 * No other SPI devices can be used between `begin` and `end`
 *
 * @see spi_display_end()
 */
extern void spi_display_begin(void);

/**
 * @brief Signal the end of one or more SPI operations on the Display
 * @ingroup spi_ops
 *
 * `spi_display_begin` must have been called at the beginning of the operations.
 */
extern void spi_display_end(void);

extern int spi_display_read_buf(uint8_t txval, uint8_t* dst, size_t len);

extern uint8_t spi_display_read8(uint8_t txval);

/**
 * @brief Select the SPI Display device for activity
 * @ingroup spi_ops
 *
 * This selects the Display as the active SPI device. Only one SPI
 * device can be active at a time. Once operations are complete
 * on the device, `spi_none_select` must be called.
 *
 * @see spi_none_select()
 *
 */
extern void spi_display_select();

extern int spi_display_write8(uint8_t data);

extern int spi_display_write8_buf(const uint8_t* buf, size_t len);

extern int spi_display_write16(uint16_t data);

extern int spi_display_write16_buf(const uint16_t* buf, size_t len);


/**
 * @brief Take control of the SPI bus for use by the Expansion I/O.
 * @ingroup spi_ops
 *
 * Make sure we have control of the SPI for one or more operations.
 * `spi_end` must be called when the SPI is done being used.
 *
 * No other SPI devices can be used between `begin` and `end`
 *
 * @see spi_expansion_end()
 */
extern void spi_expio_begin(void);

/**
 * @brief Signal the end of one or more SPI operations on the Expansion I/O.
 * @ingroup spi_ops
 *
 * `spi_expansion_begin` must have been called at the beginning of the operations.
 */
extern void spi_expio_end(void);

extern int spi_expio_read_buf(uint8_t txval, uint8_t* dst, size_t len);

extern uint8_t spi_expio_read8(uint8_t txval);

/**
 * @brief Select the SPI Expansion I/O device for activity
 * @ingroup spi_ops
 *
 * This selects the Expansion I/O Controller as the active SPI device.
 * Only one SPI device can be active at a time. Once operations are complete
 * on the device, `spi_none_select` must be called.
 *
 * @see spi_none_select()
 *
 */
extern void spi_expio_select();

extern int spi_expio_write8(uint8_t data);

extern int spi_expio_write8_buf(const uint8_t* buf, size_t len);


/**
 * @brief Deselect all of the SPI devices
 * @ingroup spi_ops
 *
 * This deselects all SPI devices.
 *
 */
extern void spi_none_select();


/**
 * @brief Take control of the SPI bus for use by the Touch Panel.
 * @ingroup spi_ops
 *
 * Make sure we have control of the SPI for one or more operations.
 * `spi_end` must be called when the SPI is done being used.
 *
 * No other SPI devices can be used between `begin` and `end`
 *
 * @see spi_touch_end()
 */
extern void spi_touch_begin(void);

/**
 * @brief Signal the end of one or more SPI operations on the Touch Panel.
 * @ingroup spi_ops
 *
 * `spi_touch_begin` must have been called at the beginning of the operations.
 */
extern void spi_touch_end(void);

extern int spi_touch_read_buf(uint8_t txval, uint8_t* dst, size_t len);

extern uint8_t spi_touch_read8(uint8_t txval);

/**
 * @brief Select the SPI Touch Panel device for activity
 * @ingroup spi_ops
 *
 * This selects the Touch Panel as the active SPI device. Only one SPI
 * device can be active at a time. Once operations are complete
 * on the device, `spi_none_select` must be called.
 *
 * @see spi_none_select()
 *
 */
extern void spi_touch_select();

extern int spi_touch_write8(uint8_t data);

extern int spi_touch_write8_buf(const uint8_t* buf, size_t len);


/**
 * @brief Initialize the SPI Operations module.
 * @ingroup spi_ops
 */
extern void spi_ops_module_init();

#ifdef __cplusplus
 }
#endif
#endif // _SPI_OPS_H_
