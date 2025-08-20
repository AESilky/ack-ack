/**
 * @brief Expansion I/O Control.
 * @ingroup expio
 *
 * This works with the Expansion I/O Controller chip to set the state of
 * expansion output pins and read expansion input pins. It also initializes
 * the chip, by setting the I/O pin directions and the interrupt condition.
 *
 * It provides methods to read the source of an interrupt and to clear the
 * interrupt condition.
 *
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef _EXPIO_H_
#define _EXPIO_H_
#ifdef __cplusplus
 extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Get the position of the Board Jumper from the Expansion I/O).
 * @ingroup expio
 *
 * The board jumper on IO-7 of the Expansion I/O can be 1-2 or 2-3.
 *
 * @return 0=[1-2], 1=[2-3]
 */
extern uint8_t eio_board_jumper(void);

/**
 * @brief Display Backlight Enable.
 * @ingroup expio
 *
 * Turn ON/OFF the display backlight.
 *
 * @param on true to turn the backlight on, false to turn it off
 */
extern void eio_display_backlight_on(bool on);

/**
 * @brief Turn the LED-A On/Off.
 * @ingroup expio
 *
 * @param on true to turn the LED on, false for off.
 */
extern void eio_leda_on(bool on);

/**
 * @brief Turn the LED-B On/Off.
 * @ingroup expio
 *
 * @param on true to turn the LED on, false for off.
 */
extern void eio_ledb_on(bool on);

/**
 * @brief Initialize the Expansion I/O
 * @ingroup display
 *
 * This must be called early in the board init (soon after SPI init).
 * The board address (0 or 1) is read from the Expansion I/O and parts
 * of the board initialization depend on knowing whether the board is the
 * main board (B0) or the secondary board (B1).
 */
extern void expio_module_init(void);

#ifdef __cplusplus
}
#endif
#endif // _EXPIO_H_

