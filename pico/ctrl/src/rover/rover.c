/**
 * @brief Rover hardware (platform) overall control.
 * @ingroup rover
 *
 * Controls the hardware functionality of the rover.
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */

#include "rover.h"

#include "board.h"
#include "rover_info.h"

#include "sensbank/sensbank.h"
#include "servo/servo.h"
#include "servo/servos.h"


#include "pico/stdlib.h"



// ############################################################################
// Function Declarations
// ############################################################################
//


// ############################################################################
// Data
// ############################################################################
//


// ############################################################################
// Message Handlers
// ############################################################################
//


// ############################################################################
// Internal Functions
// ############################################################################
//


// ############################################################################
// Public Functions
// ############################################################################
//


// ############################################################################
// Initialization and Maintainence Functions
// ############################################################################
//

void rover_housekeeping(void) {
    // ZZZ - Temp, exercise the position servos
    static uint8_t hk_count = 0;
    static bool rip = false;

    if (++hk_count % (62*5) == 0) {
        if (rip) {
            servos_rip_position();
        }
        else {
            servos_zero_position();
        }
        rip = !rip;
    }
}

void rover_start(void) {
    sensbank_start();
    servos_start();
}



void rover_module_init(void) {
    static bool _initialized = false;

    if (_initialized) {
        board_panic("rover_module_init already called");
    }
    _initialized = true;

    sensbank_module_init();
    servos_module_init();
}
