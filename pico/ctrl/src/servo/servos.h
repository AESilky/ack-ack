/**
 * @brief Serial Bus Servo group control.
 * @ingroup servo
 *
 * Controls a group of HiWonder Serial Bus servos.
 * The servos are for the rover and are as follows:
 *
 * LF_DIR                              RF_DIR
 * LF_DRIVE                            RF_DRIVE
 *
 *                BOGIE_PIVOT
 * LM_DRIVE                            RM_DRIVE
 *
 *
 * LR_DRIVE                            RR_DRIVE
 * LR_DIR                              RR_DIR
 *
 * LF_DIR   : Turns the Left-Front drive wheel from -120° to 0 to +120°
 * LF_DRIVE : Drives the Left-Front drive wheel with a speed from -1000 to 0 to +1000
 * LM_DRIVE : Drives the Left-Middle drive wheel
 * LR_DRIVE : Drives the Left-Rear drive wheel
 * LR_DIR   : Turns the Left-Rear drive wheel
 * RF_DIR   : Turns the Right-Front drive wheel from -120° to 0 to +120°
 * RF_DRIVE : Drives the Right-Front drive wheel with a speed from -1000 to 0 to +1000
 * RM_DRIVE : Drives the Right-Middle drive wheel
 * RR_DRIVE : Drives the Right-Rear drive wheel
 * RR_DIR   : Turns the Right-Rear drive wheel
 * BOGIE_PIVOT : Reads, and can drive, the bogie pivot arm angle
 *
 * Copyright 2023-24 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef _SERVOS_H_
#define _SERVOS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Position the directional servos for a Rotate-In-Place manuever.
 * @ingroup servos
 *
 * For Rotate-In-Place (spin) the front steering needs to be toe-in and the
 * rear steering needs to be toe-out. This is done to an angle, such that,
 * the axles are pointing to the drive center point.
 */
extern void servos_rip_position();

/**
 * @brief Position the directional servos to the zero (straight-forward/backward) position.
 */
extern void servos_zero_position();

/**
 * @brief Housekeeping for the Servos module.
 * @ingroup servo
 *
 * This performs regular housekeeping for the Servos Module.
 * It is expected to be called every ~16ms by the Hardware Control OS.
 */
extern void servos_housekeeping(void);

/**
 * @brief Starts the various servos on the rover.
 * @ingroup servo
 *
 * This should be called after the messaging system is up and running.
 * This reads the position, sets the position, and powers up all of the
 * servos on the rover.
 */
extern void servos_start(void);

/**
 * @brief Initialize the Serial Bus Servos (group) control module.
 * @ingroup servo
 */
extern void servos_module_init(void);


#ifdef __cplusplus
    }
#endif
#endif // _SERVOS_H_
