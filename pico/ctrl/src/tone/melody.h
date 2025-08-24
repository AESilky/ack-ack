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
#ifndef MELODY_H_
#define MELODY_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "melody_t.h"
#include "tone_t.h"

#include <stdint.h>

/**
 * @brief Get a melody (tone_note_t*) for a Melody ID.
 * @ingroup tone
 *
 * @return const tone_note_t* Melody structure list.
 */
extern const tone_note_t* melody(melody_id_t id);

/**
 * @brief Get the ID of a melody.
 * @ingroup tone
 *
 * @param melody A tone_note_t pointer
 * @return const melody_id_t The ID of the melody, or MELODY_CUSTOM, or MELODY_NONE.
 */
extern const melody_id_t melody_id(tone_note_t* melody);

#ifdef __cplusplus
    }
#endif
#endif // MELODY_H_

