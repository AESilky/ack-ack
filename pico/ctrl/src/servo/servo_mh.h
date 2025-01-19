/**
 * @brief Serial Bus Servo control.
 * @ingroup servo
 *
 * Controls a collection of HiWonder Serial Bus servos.
 * This file contains the Message Handler declarations (only)
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef SERVO_MH_H_
#define SERVO_MH_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "cmt/cmt.h"

extern const msg_handler_entry_t servo_rxd_handler_entry;

#ifdef __cplusplus
    }
#endif
#endif // SERVO_MH_H_
