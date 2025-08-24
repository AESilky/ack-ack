/**
 * @brief Melodies for use in tone_play_melody().
 * @ingroup tone
 *
 * Canned melodies.
 *
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef MELODY_T_H_
#define MELODY_T_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum melody_id_ {
    MELODY_STOP = 0,
    MELODY_STARTUP,
    MELODY_ERROR,
    MELODY_SINGLE_BEEP,
    MELODY_NOTIFY_POSITIVE,
    MELODY_NOTIFY_NEUTRAL,
    MELODY_NOTIFY_NEGATIVE,
    MELODY_ARMING,
    MELODY_BATTERY_WARNING_SLOW,
    MELODY_BATTERY_WARNING_FAST,
    MELODY_GPS_WARNING,
    MELODY_ARMING_FAILURE,
    MELODY_PARACHUTE_RELEASE,
    MELODY_EKF_WARNING,
    MELODY_BARO_WARNING,
    MELODY_HOME_SET,
    /* Do not include these unused tunes
    MELODY_CHARGE,
    MELODY_DIXIE,
    MELODY_CUCURACHA,
    MELODY_YANKEE,
    MELODY_DAISY,
    MELODY_WILLIAM_TELL, */
    MELODY_NUM,
    //
    MELODY_CUSTOM = -1,
    MELODY_NONE = -2,
} melody_id_t;

#ifdef __cplusplus
    }
#endif
#endif // MELODY_T_H_

