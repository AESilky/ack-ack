/**
 * Rover information.
 *
 * This module provides rover information including dimensions and angles.
 * The values that require calculating (typically with trig functions) are
 * done once in the module initialization. The values are then made available
 * through function calls that simply return the (already) calculated value.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
 */
#include "rover_info.h"

#include "board.h"

#include <stdbool.h>
#include <stdint.h>
#include <math.h>

// ######################################################################
// Defines used within the module for calculations and values.
// ######################################################################

/** Width, wheel centerline to wheel centerline */
#define ROVER_DIM_TRACK 325
/** Wheelbase, front axle to rear axle */
#define ROVER_DIM_WHEELBASE 350

static float _track;
static int _track_i;

static float _wheelbase;
static int _wheelbase_i;

/** @brief Centroid to Corner Wheel CP */
static float _cp_cnr_d;
/** @brief Centroid to the Front/Rear axle - Length (half of the wheelbase) */
static float _cp_l;
/** @brief Centroid to the center of the side drive wheel - Width (half of the track) */
static float _cp_w;

/** @brief The angle (radians) of the  */
static float _rip_angl;
static float _rip_cw_mw_speed;


// ######################################################################
// Public Methods
// ######################################################################

float rover_cp_cnr_d() {
    return _cp_cnr_d;
}

float rover_cp_l() {
    return _cp_l;
}

float rover_cp_w() {
    return _cp_w;
}

float rover_rip_angl() {
    return _rip_angl;
}

float rover_track() {
    return _track;
}

int rover_track_i(){
    return _track_i;
}

float rover_wheelbase() {
    return _wheelbase;
}

int rover_wheelbase_i() {
    return _wheelbase_i;
}


// ######################################################################
// Module Initialization
// ######################################################################

void rover_info_module_init() {
    static bool _initialized = false;
    if (_initialized) {
        board_panic("!!! `rover_info_module_init` called more than once !!!");
    }
    _initialized = true;

    // Store values from the defines
    _track_i = ROVER_DIM_TRACK;
    _track = (float)ROVER_DIM_TRACK;
    _wheelbase_i = ROVER_DIM_WHEELBASE;
    _wheelbase = (float)ROVER_DIM_WHEELBASE;

    // Calculate 'simple' values
    _cp_l = _wheelbase / 2.0;
    _cp_w = _track / 2.0;

    // Calculate more complex values
    _cp_cnr_d = sqrt((_cp_l * _cp_l) + (_cp_w * _cp_w));
    _rip_angl = asin(_cp_w / _cp_cnr_d); // Arc-Sine of the CP-To-Width / CP-To-Corner (the hypotenuse)
}
