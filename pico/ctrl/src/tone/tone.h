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
#ifndef TONE_H_
#define TONE_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "tone_t.h"
#include "melody_t.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Cancel the playing of a tone/melody.
 */
extern void tone_cancel_play();

/**
 * @brief Frequency for a given pitch (returns frequency*1000)
 *
 * @param pitch A pitch number from the tone_pitch_ enum
 * @return uint16_t Frequency x1000 (for example, A4=4400)
 */
extern uint16_t tone_freq_for_pitch(tone_pitch_t pitch);

/**
 * @brief Check for a melody playing.
 *
 * @return true Melody is playing
 * @return false No melody is playing
 */
extern bool tone_melody_is_playing();

/**
 * @brief Get the Melody ID of the melody that is playing.
 *
 * @return melody_id_t The Melody ID, MELODY_CUSTOM, or MELODY_NONE.
 */
extern melody_id_t tone_melody_playing();

/**
 * @brief Play (sound) a specific frequency for a duration.
 * @ingroup tone
 *
 * This plays any frequency, and can be used to play tones that aren't
 * in the standard pitch enum.
 * @see tone_play_pitch to play standard notes from the pitch enum.
 *
 * @param freq Frequency x1000 to play (for example, A440 = 4400)
 * @param duration Duration in 10ms units
 */
extern void tone_play_frequency(uint16_t freq, uint16_t duration);

/**
 * @brief Play a melody of notes.
 * @ingroup tone
 *
 * Plays a melody of multiple notes. A note consists of a pitch and duration.
 * A note with a duration of DURATION_END (0) signals the end of the melody (and
 * isn't played). A melody can be repeated by using a duration of DURATION_REPEAT.
 *
 * @param melody A pointer to a list of tone_note_t values.
 */
extern void tone_play_melody(const tone_note_t *melody);

/**
 * @brief Play a melody, using a Melody ID.
 * @ingroup tone
 *
 * @see tone/melody_t.h for Melody IDs
 *
 * @param melody_id ID of the Melody to play
 */
extern void tone_play_melody_id(melody_id_t melody_id);

/**
 * @brief Play (sound) a pitch for a duration.
 * @ingroup tone
 *
 * This plays standard notes from the pitch enum.
 * @see tone_play_frequency for sounding frequencies.
 *
 * @param pitch Pitch value to play
 * @param duration Duration in 10ms units
 */
extern void tone_play_pitch(tone_pitch_t pitch, uint16_t duration);


/**
 * @brief Initialize the module. Must be called before using module methods.
 */
extern void tone_module_init(void);

#ifdef __cplusplus
    }
#endif
#endif // TONE_H_
