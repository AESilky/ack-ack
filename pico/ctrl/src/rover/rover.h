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

/**
 * @brief Housekeeping for the Rover module.
 * @ingroup rover
 *
 * This performs regular housekeeping for the Rover Module.
 * It is expected to be called every ~16ms by the Hardware Control OS.
 */
extern void rover_housekeeping(void);

/**
 * @brief Starts the various hardware functions of the rover.
 * @ingroup servo
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
