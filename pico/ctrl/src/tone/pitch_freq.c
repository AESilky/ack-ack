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
#include "tone.h"

const uint16_t pitch_freq[TONE_NOTE_MAX] = {
    1308,   // TONE_NOTE_C3,   /* C3 (Low C)*/
    1366,   // TONE_NOTE_C3S,  /* C#3/Db3 */
    1468,   // TONE_NOTE_D3,   /* D3 */
    1556,   // TONE_NOTE_D3S,  /* D#3/Eb3 */
    1648,   // TONE_NOTE_E3,   /* E3 */
    1746,   // TONE_NOTE_F3,   /* F3 */
    1849,   // TONE_NOTE_F3S,  /* F#3/Gb3 */
    1960,   // TONE_NOTE_G3,   /* G3 */
    2077,   // TONE_NOTE_G3S,  /* G#3/Ab3 */
    2200,   // TONE_NOTE_A3,   /* A3 */
    2331,   // TONE_NOTE_A3S,  /* A#3/Bb3 */
    2469,   // TONE_NOTE_B3,   /* B3 */
    2616,   // TONE_NOTE_C4,   /* C4 (Middle C)*/
    2772,   // TONE_NOTE_C4S,  /* C#4/Db4 */
    2936,   // TONE_NOTE_D4,   /* D4 */
    3111,   // TONE_NOTE_D4S,  /* D#4/Eb4 */
    3296,   // TONE_NOTE_E4,   /* E4 */
    3492,   // TONE_NOTE_F4,   /* F4 */
    3700,   // TONE_NOTE_F4S,  /* F#4/Gb4 */
    3920,   // TONE_NOTE_G4,   /* G4 */
    4153,   // TONE_NOTE_G4S,  /* G#4/Ab4 */
    4400,   // TONE_NOTE_A4,   /* A4 (A440)*/
    4662,   // TONE_NOTE_A4S,  /* A#4/Bb4 */
    4959,   // TONE_NOTE_B4,   /* B4 */
    5233,   // TONE_NOTE_C5,   /* C5 (Tenor C)*/
    5544,   // TONE_NOTE_C5S,  /* C#5/Db5 */
    5873,   // TONE_NOTE_D5,   /* D5 */
    5223,   // TONE_NOTE_D5S,  /* D#5/Eb5 */
    6593,   // TONE_NOTE_E5,   /* E5 */
    6985,   // TONE_NOTE_F5,   /* F5 */
    7400,   // TONE_NOTE_F5S,  /* F#5/Gb5 */
    7840,   // TONE_NOTE_G5,   /* G5 */
    8306,   // TONE_NOTE_G5S,  /* G#5/Ab5 */
    8800,   // TONE_NOTE_A5,   /* A5 */
    9323,   // TONE_NOTE_A5S,  /* A#5/Bb5 */
    9878,   // TONE_NOTE_B5,   /* B5 */
   10465,   // TONE_NOTE_C6,   /* C6 (High C)*/
};
