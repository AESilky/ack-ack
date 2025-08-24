/**
 * @brief TONE (melody/speaker/tone) Module.
 * @ingroup tone
 *
 * Provides tone generation and melody playing functionality.
 *
 * Uses a PWM slice-channel to drive a GPIO that is connected to a speaker driver.
 *
 * This can be used to simply turn on/off a tone, or it allows playing a melody.
 * A melody can be provided to a method or one of a number of pre-defined melodies
 * can be played.
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef TONE_T_H_
#define TONE_T_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum tone_note_duration_ {
    NOTE_WHOLE = 100,
    NOTE_3Q = 75,
    NOTE_HALF = 50,
    NOTE_1Q = 25,
    NOTE_8TH = 12,
    NOTE_16TH = 6,
    NOTE_32ND = 3
} tone_note_duration_t;

typedef enum tone_pitch_ {
    TONE_NOTE_C3,   /* C3 (Low C)*/
    TONE_NOTE_C3S,  /* C#3/Db3 */
    TONE_NOTE_D3,   /* D3 */
    TONE_NOTE_D3S,  /* D#3/Eb3 */
    TONE_NOTE_E3,   /* E3 */
    TONE_NOTE_F3,   /* F3 */
    TONE_NOTE_F3S,  /* F#3/Gb3 */
    TONE_NOTE_G3,   /* G3 */
    TONE_NOTE_G3S,  /* G#3/Ab3 */
    TONE_NOTE_A3,   /* A3 */
    TONE_NOTE_A3S,  /* A#3/Bb3 */
    TONE_NOTE_B3,   /* B3 */
    TONE_NOTE_C4,   /* C4 (Middle C)*/
    TONE_NOTE_C4S,  /* C#4/Db4 */
    TONE_NOTE_D4,   /* D4 */
    TONE_NOTE_D4S,  /* D#4/Eb4 */
    TONE_NOTE_E4,   /* E4 */
    TONE_NOTE_F4,   /* F4 */
    TONE_NOTE_F4S,  /* F#4/Gb4 */
    TONE_NOTE_G4,   /* G4 */
    TONE_NOTE_G4S,  /* G#4/Ab4 */
    TONE_NOTE_A4,   /* A4 */
    TONE_NOTE_A4S,  /* A#4/Bb4 */
    TONE_NOTE_B4,   /* B4 */
    TONE_NOTE_C5,   /* C5 (Tenor C)*/
    TONE_NOTE_C5S,  /* C#5/Db5 */
    TONE_NOTE_D5,   /* D5 */
    TONE_NOTE_D5S,  /* D#5/Eb5 */
    TONE_NOTE_E5,   /* E5 */
    TONE_NOTE_F5,   /* F5 */
    TONE_NOTE_F5S,  /* F#5/Gb5 */
    TONE_NOTE_G5,   /* G5 */
    TONE_NOTE_G5S,  /* G#5/Ab5 */
    TONE_NOTE_A5,   /* A5 */
    TONE_NOTE_A5S,  /* A#5/Bb5 */
    TONE_NOTE_B5,   /* B5 */
    TONE_NOTE_C6,   /* C6 (High C)*/

    TONE_NOTE_MAX,

    TONE_NOTE_REST = 255 /* Rest is used to 'play' a duration of silence */
} tone_pitch_t;

/* structure describing one note in a tone pattern */
typedef struct tone_note_ {
    tone_pitch_t	pitch;
    int16_t         duration;   /* duration is multiplied by 10ms */
#define DURATION_END		 0	/* ends the pattern */
#define DURATION_REPEAT		-1	/* resets the note counter to zero */
} tone_note_t;

#ifdef __cplusplus
    }
#endif
#endif // TONE_T_H_
