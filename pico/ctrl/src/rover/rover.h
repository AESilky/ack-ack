/**
 * @brief Rover (platform) hardware control.
 * @ingroup rover
 *
 * Controls and monitors the rover platform.
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef _ROVER_H_
#define _ROVER_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

// ############################################################################
// Public Functions
// ############################################################################
//

/**
 * @brief The current being used by the Auxilary (switched Sensor/Servo) Power.
 *
 * @return float The current being used in milli-amps (mA)
 */
extern float rover_aux_pwr_ma();

/**
 * @brief Turn the Auxilary (Sensor/Servo) Power ON/OFF
 *
 * @param on true for ON, false for OFF
 */
extern void rover_aux_pwr_on(bool on);

/**
 * @brief The measured voltage of Battery-1.
 *
 * @return float Battery voltage
 */
extern float rover_batt1_voltage();

/**
 * @brief The measured voltage of Battery-2.
 *
 * @return float Battery voltage
 */
extern float rover_batt2_voltage();

/**
 * @brief The measured light level.
 *
 * @return float Level in 'uinits'. Greater is more light.
 */
extern float rover_light_lvl();

// ############################################################################
// Initialization and Maintainence Functions
// ############################################################################
//

/**
 * @brief Housekeeping for the Rover module.
 * @ingroup rover
 *
 * This performs regular housekeeping for the Rover Module.
 * It is expected to be called every ~16ms by the Hardware Control Runtime.
 */
extern void rover_housekeeping(void);

/**
 * @brief Starts the various hardware functions of the rover.
 * @ingroup rover
 *
 * This should be called after the messaging system is up and running.
 */
extern void rover_start(void);

/**
 * @brief Initialize the (overall) rover hardware platform).
 * @ingroup rover
 */
extern void rover_module_init(void);


#ifdef __cplusplus
    }
#endif
#endif // _ROVER_H_
