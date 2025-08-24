/**
 * Frequency for pitch table.
 *
 * The table is an array with elements for each of the entries in the tone_pitch_ enum.
 * The value is the (standard) frequency x1000 (for example, A4 is 4400).
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#include "melody.h"

#include <stddef.h>

static const tone_note_t _stop[] = {
    {TONE_NOTE_C3, NOTE_32ND},
    {TONE_NOTE_REST, NOTE_32ND},
    {TONE_NOTE_C3, NOTE_32ND},
    {TONE_NOTE_REST, DURATION_END}
};
static const tone_note_t _startup[] = {
    {TONE_NOTE_C5, NOTE_32ND},
    {TONE_NOTE_REST, NOTE_32ND},
    {TONE_NOTE_C5, NOTE_32ND},
    { TONE_NOTE_REST, DURATION_END }
};
static const tone_note_t _error[] = {
    {TONE_NOTE_C3S, NOTE_1Q},
    { TONE_NOTE_REST, DURATION_END }
};
static const tone_note_t _beep[] = {
    {TONE_NOTE_E3, NOTE_1Q},
    { TONE_NOTE_REST, DURATION_END }
};
static const tone_note_t _notify_pos[] = {
    {TONE_NOTE_C4, NOTE_16TH},
    {TONE_NOTE_REST, NOTE_16TH},
    {TONE_NOTE_C5, NOTE_16TH},
    { TONE_NOTE_REST, DURATION_END }
};
static const tone_note_t _notify_neut[] = {
    {TONE_NOTE_C4, NOTE_16TH},
    {TONE_NOTE_REST, NOTE_16TH},
    {TONE_NOTE_C4, NOTE_16TH},
    { TONE_NOTE_REST, DURATION_END }
};
static const tone_note_t _notify_neg[] = {
    {TONE_NOTE_C5, NOTE_16TH},
    {TONE_NOTE_REST, NOTE_16TH},
    {TONE_NOTE_C4, NOTE_16TH},
    { TONE_NOTE_REST, DURATION_END }
};
static const tone_note_t _arming[] = {
    {TONE_NOTE_C5, NOTE_32ND},
    {TONE_NOTE_REST, NOTE_32ND},
    {TONE_NOTE_D5, NOTE_32ND},
    {TONE_NOTE_REST, NOTE_32ND},
    {TONE_NOTE_E5, NOTE_32ND},
    {TONE_NOTE_REST, NOTE_32ND},
    {TONE_NOTE_F5, NOTE_32ND},
    { TONE_NOTE_REST, DURATION_END }
};
static const tone_note_t _battery_warn_slow[] = {
    {TONE_NOTE_A5, NOTE_32ND},
    {TONE_NOTE_REST, NOTE_16TH},
    {TONE_NOTE_A5, NOTE_32ND},
    {TONE_NOTE_REST, 1000}, // 10 second pause before repeat
    { TONE_NOTE_REST, DURATION_REPEAT }
};
static const tone_note_t _battery_warn_fast[] = {
    {TONE_NOTE_A5, NOTE_32ND},
    {TONE_NOTE_REST, NOTE_16TH},
    {TONE_NOTE_A5, NOTE_32ND},
    {TONE_NOTE_REST, 400}, // 4 second pause before repeat
    { TONE_NOTE_REST, DURATION_REPEAT }
};
static const tone_note_t _gps_warn[] = {
    {TONE_NOTE_F5, NOTE_16TH},
    {TONE_NOTE_REST, NOTE_16TH},
    {TONE_NOTE_D5, NOTE_16TH},
    { TONE_NOTE_REST, DURATION_END }
};
static const tone_note_t _arm_fail[] = {
    {TONE_NOTE_F5, NOTE_32ND},
    {TONE_NOTE_REST, NOTE_32ND},
    {TONE_NOTE_E5, NOTE_32ND},
    {TONE_NOTE_REST, NOTE_32ND},
    {TONE_NOTE_D5, NOTE_32ND},
    {TONE_NOTE_REST, NOTE_32ND},
    {TONE_NOTE_C5, NOTE_32ND},
    { TONE_NOTE_REST, DURATION_END }
};


static const tone_note_t* _melodies[] = {
    _stop,              // MELODY_STOP
    _startup,           // MELODY_STARTUP
    _error,             // MELODY_ERROR
    _beep,              // MELODY_SINGLE_BEEP
    _notify_pos,        // MELODY_NOTIFY_POSITIVE
    _notify_neut,       // MELODY_NOTIFY_NEUTRAL
    _notify_neg,        // MELODY_NOTIFY_NEGATIVE
    _arming,            // MELODY_ARMING
    _battery_warn_slow, // MELODY_BATTERY_WARNING_SLOW
    _battery_warn_fast, // MELODY_BATTERY_WARNING_FAST
    _gps_warn,          // MELODY_GPS_WARNING
    _arm_fail,          // MELODY_ARMING_FAILURE/FAILSAFE
    // MELODY_EKF_WARNING
    // MELODY_BARO_WARNING
    // MELODY_HOME_SET
    // MELODY_CHARGE
    /* Do not include these unused tunes
    // MELODY_DIXIE
    // MELODY_CUCURACHA
    // MELODY_YANKEE
    // MELODY_DAISY
    // MELODY_WILLIAM_TELL
    */
};

const tone_note_t* melody(melody_id_t id) {
    const tone_note_t* mel = (id < MELODY_NUM ? _melodies[id] : _error);
    return (mel);
}

const melody_id_t melody_id(tone_note_t* melody) {
    melody_id_t id = MELODY_NONE;
    if (melody != (tone_note_t*)NULL) {
        for (melody_id_t i = MELODY_STOP; i < MELODY_NUM; i++) {
            if (melody == _melodies[i]) {
                return (i);
            }
        }
        id = MELODY_CUSTOM;
    }
    return (id);
}

