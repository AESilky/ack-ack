/**
 * Servo control functions.
 *
 * Use a PIO to control up to 4 servos via PWM.
 *
 * The PIO program runs a loop that takes 3 SM cycles times the 'period-multiplier'
 * for the period, and generates a pulse that is 3 SM cycles times the 'pulse-multiplier'.
 *
 * For the best resolution, the SM clock should be set as high as possible while being
 * able to hit the period and pulse with 32 bit multipliers.
 *
 * Standard servos run at a frequency of 50Hz (20ms (20,000µs) period), though high-performance
 * servos can run at a faster frequency (shorter period).
 * Standard servos represent 0° (neutral) with a pulse width of 1500µs, +90° with 2400µs
 * and -90° with 750µs. Therefore, 1° is a delta of 9.16µs.
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#include "servo.h"
#include "system_defs.h"

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"

#include "pwm.pio.h"

// Default count values
#define DECIDEGREE_COUNT_DEF 9 // 9µs per deg
#define NEUTRAL_COUNT_DEF 15000
#define PERIOD_COUNT_DEF 200000
#define SERVO_MAX_COUNT_DEF 23000
#define SERVO_MIN_COUNT_DEF 4000

static servoctl_t _servos[PIO_SERVO_COUNT];

int32_t _servo_adj_pos_val(servoctl_t *servo, int32_t pos) {
    if (pos > servo->max_count) {
        return servo->max_count;
    }
    if (pos < servo->min_count) {
        return servo->min_count;
    }
    return pos;
}

/**
 * @brief Write the `period-multiplier` to the PIO ISR.
 *
 * The PIO's ISR is used to hold the period multiplier value.
 */
void _servo_set_period_count(uint servo, uint32_t period_count) {
    pio_sm_set_enabled(PIO_SERVOS, servo, false);
    pio_sm_put_blocking(PIO_SERVOS, servo, period_count);
    pio_sm_exec(PIO_SERVOS, servo, pio_encode_pull(false, false));
    pio_sm_exec(PIO_SERVOS, servo, pio_encode_out(pio_isr, 32));
    pio_sm_set_enabled(PIO_SERVOS, servo, _servos[servo].enabled);
}

/**
 * @brief Write the pulse width cycles to TX FIFO.
 */
void _servo_set_pulse(uint servo, uint32_t pwc) {
    pio_sm_put_blocking(PIO_SERVOS, servo, pwc);
}

void servo_disable(int servo_num) {
    servo_set_enabled(servo_num, false);
}

void servo_enable(int servo_num) {
    servo_set_enabled(servo_num, true);
}

void servo_get(int servo_num, servoctl_t *servo) {
    servoctl_t *src = &_servos[servo_num];
    servo->decidegree_count = src->decidegree_count;
    servo->enabled = src->enabled;
    servo->pos = src->pos;
    servo->zero_count = src->zero_count;
}

void servo_set(int servo_num, servoctl_t *servo) {
    servoctl_t *dst = &_servos[servo_num];
    bool mod_pos = false;
    bool mod_en = false;

    dst->decidegree_count = servo->decidegree_count;
    dst->zero_count = servo->zero_count;
    if (dst->pos != servo->pos) {
        mod_pos = true;
        dst->pos = _servo_adj_pos_val(dst, servo->pos);
    }
    if (dst->enabled != servo->enabled) {
        mod_en = true;
        dst->enabled = servo->enabled;
    }
    if (mod_en) {
        // If turning off, turn off before setting position.
        // If turning on, set position before turning on.
        bool enabled = servo->enabled;
        if (enabled) {
            _servo_set_pulse(servo_num, servo->pos);
            servo_set_enabled(servo_num, true);
        }
        else {
            servo_set_enabled(servo_num, false);
            _servo_set_pulse(servo_num, servo->pos);
        }
        // In either case, we've set the position, so no need to do it again.
        mod_pos = false;
    }
    if (mod_pos) {
        _servo_set_pulse(servo_num, servo->pos);
    }
}

void servo_set_angle(int servo_num, int32_t decidegree) {
    servoctl_t *servo = &_servos[servo_num];
    int32_t pos = servo->zero_count + (servo->decidegree_count * decidegree);
    servo->pos = _servo_adj_pos_val(servo, pos);
    _servo_set_pulse(servo_num, pos);
}

void servo_set_enabled(int servo_num, bool enabled) {
    _servos[servo_num].enabled = enabled;
    pio_sm_set_enabled(PIO_SERVOS, PIO_SM_SERVO0+servo_num, enabled);
}

void servo_module_init() {
    // Set up the PIO to control servos via PWM.
    uint offset = pio_add_program(PIO_SERVOS, &pwm_program);

    // Get the SM clock divisor for the PIO.
    //
    // Support various system clock settings.
    uint32_t clkhz = clock_get_hz(clk_sys);
    // Calculate a SM clock divisor so that a pulse width count value of 1 is 0.1µs.
    // In the PIO code, there are 3 SM clock cycles per period 'tick'.
    float clkdiv = (((float)clkhz) / 30000000.0f);

    for (int i = 0; i < PIO_SERVO_COUNT; i++) {
        servoctl_t *servo = &_servos[i];
        servo->zero_count = NEUTRAL_COUNT_DEF;
        servo->decidegree_count = DECIDEGREE_COUNT_DEF;
        servo->min_count = SERVO_MIN_COUNT_DEF;
        servo->max_count = SERVO_MAX_COUNT_DEF;
        servo->pos = NEUTRAL_COUNT_DEF;
        servo->enabled = false;

        pwm_program_init(PIO_SERVOS, PIO_SM_SERVO0+i, offset, clkdiv, SERVO1_PIN+i);
        _servo_set_period_count(i, PERIOD_COUNT_DEF);
        _servo_set_pulse(i, servo->pos);
        servo_set_enabled(i, false);
    }
}
