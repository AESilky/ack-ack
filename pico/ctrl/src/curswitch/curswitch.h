/**
 * @brief Cursor Switches functionality through analog input.
 * @ingroup curswitch
 *
 * This reads analog inputs for values indicating switch presses. The switches are laid out in
 * a cursor pattern (as below). Each switch is connected via a voltage divider such that it
 * produces a unique value when pressed. Some combinations can also be detected (with less
 * certainty).
 *
 * A bank is laid out with switches as follows with each switch producing the indicated voltage:
 *
 *                  < UP - n.nn >
 *  < LF - n.nn >   < HM - n.nn >   < RT - n.nn >
 *                  < DN - n.nn >
 *
 *                  < ENTER - 0 >
 *
 * The switch voltage input for a bank is through either, or both, of the IR inputs on the board.
 *
 *
 * Copyright 2024 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef _CURSWITCH_H_
#define _CURSWITCH_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "curswitch_t.h"

/**
 * @brief Get the switch short name for an ID.
 * 
 * @return const char* Switch short name (2 chars)
 */
extern const char* curswitch_shortname_for_swid(switch_id_t sw_id);

/**
 * @brief Get the current state of a specific cursor switch.
 * 
 * Given a bank (1 or 2) and a switch, fill in the state.
 * 
 * @param bank 1 or 2
 * @param sw Switch ID
 * @param state Pointer to a switch state structure to populate
 * 
 */
extern void curswitch_state(switch_id_t sw, sw_state_t *state);

/**
 * @brief True if the bank-switch is pressed.
 * 
 * @param bank Bank1 or Bank2
 * @param sw Switch ID
 * @return True is the bank-switch is pressed
 */
extern bool curswitch_sw_pressed(switch_id_t sw);

/**
 * @brief The number of milliseconds that the bank-switch has been pressed.
 * 
 * return milliseconds that the switch has been pressed. 0 if switch isn't pressed.
 */
extern uint32_t curswitch_sw_pressed_duration(switch_id_t sw);

/**
 * @brief Trigger the reading of the current state of the switch bank, process the values,
 *          and post messages.
 *
 * This reads the enabled switch banks, and converts the values into switch released and
 * pressed states. It then determines if switch states changed (was released and now pressed,
 * was pressed and now released), it posts messages for each.
 */
extern void curswitch_trigger_read();


/**
 * @brief Initialize (or reinitialize) the module, indicating which switch bank(s) should be used.
 *
 * This may be called more than once to enable/disable the banks.
 */
extern void curswitch_module_init();

#ifdef __cplusplus
}
#endif
#endif // _CURSWITCH_H_