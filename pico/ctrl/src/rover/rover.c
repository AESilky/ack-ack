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
#include "servos.h"

#include "board.h"
#include "rover_info.h"

#include "sensbank/sensbank.h"
#include "servo/servo.h"

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
    sensbank_update();
    servos_housekeeping();
}

void rover_start(void) {
    sensbank_start();
    servos_start();
}



void rover_module_init(void) {
    static bool _initialized = false;

    if (_initialized) {
        board_panic("!!! `rover_module_init` already called !!!");
    }
    _initialized = true;

    rover_info_module_init();
    sensbank_module_init();
    servos_module_init();
}
