/**
 * Provides tone generation and melody playing functionality.
 *
 * Uses a PWM slice-channel to drive a GPIO that is connected to a speaker driver.
 *
 * This can be used to simply turn on/off a tone, or to play a melody.
 * A melody can be provided to a method or one of a number of pre-defined melodies
 * can be played.
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#include "tone.h"
#include "pitch_freq.h"

#include "board.h"
#include "system_defs.h"
#include "cmt/cmt.h"

#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "pico/float.h"

// ////////////////////////////////////////////////////////////////////////////
// Method Declarations
// ////////////////////////////////////////////////////////////////////////////

static void _handle_tone_done(cmt_msg_t* msg);
static void _play_frequency(uint16_t freq, uint16_t duration);
static void _play_melody(bool from_prev_note);
static void _tone_stop();

// ////////////////////////////////////////////////////////////////////////////
// Data
// ////////////////////////////////////////////////////////////////////////////

static bool _initialized;

static const tone_note_t* _melody;
static const tone_note_t* _melody_pp;
static volatile bool _playing;

// ////////////////////////////////////////////////////////////////////////////
// Internal Methods
// ////////////////////////////////////////////////////////////////////////////

void _play_frequency(uint16_t freq, uint16_t duration) {
    if (freq == 0) {
        // stop any currently playing tone and perform a 'rest'.
        _tone_stop();
    }
    else {
        // Configure TOP and Compare (Level)
        uint16_t periodcnt = (uint16_t)(10000000.0f / uint2float(freq));
        uint16_t tgl = periodcnt / 2;
        pwm_set_wrap(PWM_SPKRDRV_SLICE, periodcnt);
        pwm_set_chan_level(PWM_SPKRDRV_SLICE, PWM_SPKRDRV_CHAN, tgl);
        pwm_set_counter(PWM_SPKRDRV_SLICE, 0);
        // Start the output
        pwm_set_enabled(PWM_SPKRDRV_SLICE, true);
    }
    // Schedule a message for the duration * 10
    cmt_msg_t msg;
    uint32_t time = duration * 10;
    cmt_exec_init(&msg, _handle_tone_done);
    schedule_core0_msg_in_ms(time, &msg);  // Tones are handled by Core 0 regardless of the current core
    _playing = true;
}

static void _play_melody_next_note(void* data) {
    if (_melody_pp != (tone_note_t*)NULL) {
        _melody_pp++;
        _play_melody(false);
    }
}

static void _play_melody(bool from_prev_note) {
    if (from_prev_note) {
        // Do a between note gap (silence) and then move to the next note
        //  30% of the duration of the note played (plus a small amount to deal with 0)
        uint32_t gap_ms = ((_melody_pp->duration * 10) / 30) + 2;
        cmt_run_after_ms(gap_ms, _play_melody_next_note, NULL);
        return;
    }
    if (_melody_pp->duration == DURATION_END) {
        _melody_pp = _melody = (tone_note_t*)NULL;
        _tone_stop();
        return;
    }
    if (_melody_pp->duration == DURATION_REPEAT) {
        _melody_pp = _melody; // Go back to the beginning
    }
    uint16_t freq = tone_freq_for_pitch(_melody_pp->pitch);
    _play_frequency(freq, _melody_pp->duration);
}

static void _playing_done() {
    _tone_stop();
    _playing = false;
}

static void _tone_stop() {
    // Stop the output
    pwm_set_enabled(PWM_SPKRDRV_SLICE, false);
}

// ////////////////////////////////////////////////////////////////////////////
// Message Handlers
// ////////////////////////////////////////////////////////////////////////////

static void _handle_tone_done(cmt_msg_t *msg) {
    _tone_stop();
    // If we are playing a melody, move to the next note.
    if (_melody_pp != (tone_note_t*)NULL) {
        _play_melody(true);
    }
    else {
        _playing = false;
    }
}

// ////////////////////////////////////////////////////////////////////////////
// Public Methods
// ////////////////////////////////////////////////////////////////////////////

uint16_t tone_freq_for_pitch(tone_pitch_t pitch) {
    if (pitch == TONE_NOTE_REST) {
        return (0);
    }
    return (pitch_freq[pitch]);
}

void tone_cancel_play() {
    scheduled_msg_cancel3(MSG_EXEC, _handle_tone_done, 0);
    // Stop any melody from playing additional notes.
    _melody_pp = _melody = (tone_note_t*)NULL;
    // Stop the currently playing tone and mark as done.
    _playing_done();
}

bool tone_melody_is_playing() {
    return (_melody != (tone_note_t*)NULL);
}

melody_id_t tone_melody_playing() {
    return melody_id(_melody);
}

void tone_play_frequency(uint16_t freq, uint16_t duration) {
    if (_playing) {
        scheduled_msg_cancel3(MSG_EXEC, _handle_tone_done, 0);
    }
    _play_frequency(freq, duration);
}

void tone_play_melody(const tone_note_t* melody) {
    if (_playing) {
        tone_cancel_play();
    }
    _playing = true;
    _melody = melody;       // Keep the original to handle 'repeat'
    _melody_pp = melody;    // Start at the beginning
    _play_melody(false);    // Start playing it
}

void tone_play_melody_id(melody_id_t melody_id) {
    tone_play_melody(melody(melody_id));
}

void tone_play_pitch(tone_pitch_t pitch, uint16_t duration) {
    uint16_t freq = tone_freq_for_pitch(pitch);
    tone_play_frequency(freq, duration);
}

void tone_module_init() {
    if (_initialized) {
        board_panic("!!! tone_module_init called more than once !!!");
    }
    _initialized = true;

    // PWM is used to generate a square-wave to drive the GPIO output that
    // then drives a speaker.
    //
    // The PWM clock is set to a rate of 1000Hz (1ms). The counter is then
    // set to divide by the frequency * 1000, with the toggle set to half.
    //
    gpio_set_function(SPKRDRV_GPIO, GPIO_FUNC_PWM);
    gpio_set_dir(SPKRDRV_GPIO, GPIO_OUT);
    gpio_set_drive_strength(SPKRDRV_GPIO, GPIO_DRIVE_STRENGTH_8MA);
    //
    pwm_config cfg = pwm_get_default_config();
    // Calculate the clock divider to achieve a 0.1ms count rate.
    //uint32_t sys_freq = clock_get_hz(clk_sys);
    //uint8_t div = uint2float(sys_freq) / 9850.0f;
    uint8_t clkdiv = 150;
    pwm_config_set_clkdiv_int(&cfg, clkdiv);
    pwm_init(PWM_SPKRDRV_SLICE, &cfg, false);
    pwm_set_clkdiv_mode(PWM_SPKRDRV_SLICE, PWM_DIV_FREE_RUNNING);
}