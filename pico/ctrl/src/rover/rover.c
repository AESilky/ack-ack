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

#include "sensbank/adc1015.h"
#include "sensbank/sensbank.h"
#include "servo/servo.h"

#include "pico/stdlib.h"


#define BATT1_ADC_INPUT 0
#define BATT2_ADC_INPUT 1
#define APC_ADC_INPUT   2
#define LIGHT_ADC_INPUT 3

/** @brief Factor to multiply the ADC voltage by to get the Battery-1 voltage */
#define ADC_V_B1_V_CNV (14.211)  // From measurement
/** @brief Factor to multiply the ADC voltage by to get the Battery-2 voltage */
#define ADC_V_B2_V_CNV (13.820)  // From measurement
//#define ADC_V_BATT_V_CNV (8.5) // Theoretical value from calculation

#define K_ILIS 560.0             // For current measurement. From BTS5150-2 datasheet
#define R_SENSE 1780.0           // Resistance for sense current measurement

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

// ZZZ - TODO: Convert the voltage to a current level value
float rover_aux_pwr_ma() {
    float v = adc1015_volts(adc1015_value(APC_ADC_INPUT));
    float i_is = v/R_SENSE;
    return (i_is * K_ILIS);
}

void rover_aux_pwr_on(bool on) {
    gpio_put(AUX_PWR_CTRL, (on ? SENSVO_PWR_ON : SENSVO_PWR_OFF));
}

bool rover_aux_pwr_is_on() {
    return (gpio_get(AUX_PWR_CTRL) != 0);
}

float rover_batt1_voltage() {
    float v = adc1015_volts(adc1015_value(BATT1_ADC_INPUT)) * ADC_V_B1_V_CNV;
    return v;
}

float rover_batt2_voltage() {
    float v = adc1015_volts(adc1015_value(BATT2_ADC_INPUT)) * ADC_V_B2_V_CNV;
    return v;
}

float rover_light_lvl() {
    float v = adc1015_volts(adc1015_value(LIGHT_ADC_INPUT)); // TODO: Is there something meaningful to convert to?
    return v;
}


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
