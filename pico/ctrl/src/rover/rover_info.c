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
#define ROVER_DIM_TRACK 315
/** Wheel diameter in millimeters */
#define ROVER_DIM_WHEEL_DIA 110
/** Wheelbase, front axle to rear axle */
#define ROVER_DIM_WHEELBASE 352

// ######################################################################
// Data members that are initialized/calculated once.
// ######################################################################

static float _track;
static int _track_i;

static float _wheel_circumference;
static float _wheelbase;
static int _wheelbase_i;

/** @brief Centroid to Corner Wheel CP */
static float _cp_cnr_d;
/** @brief Centroid to the Front/Rear axle - Length (half of the wheelbase) */
static float _cp_l;
static int _cp_l_i;
/** @brief Centroid to the center of the side drive wheel - Width (half of the track) */
static float _cp_t;
static int _cp_t_i;

/** @brief The angle (radians) of the Rotate-In-Place mode */
static float _rip_angl;


// ######################################################################
// Public Methods
// ######################################################################

float rover_cp_cnr_d() {
    return _cp_cnr_d;
}

float rover_cp_l() {
    return _cp_l;
}

int rover_cp_l_i() {
    return _cp_l_i;
}

float rover_cp_t() {
    return _cp_t;
}

int rover_cp_t_i() {
    return _cp_t_i;
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

float rover_wheel_cir() {
    return _wheel_circumference;
}

int rover_wheel_dia() {
    return ROVER_DIM_WHEEL_DIA;
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
    _wheel_circumference = (float)(ROVER_DIM_WHEEL_DIA * M_PI);
    _wheelbase_i = ROVER_DIM_WHEELBASE;
    _wheelbase = (float)ROVER_DIM_WHEELBASE;

    // Calculate the 'logical' dimensions. These are important for many calculations
    // that involve the 'Logical Center' 'LC' and 'Logical Steered' 'LS' wheels that
    // are used for a 'Logical Bicycle' navigation model. The angles and speeds of
    // all of the physical wheels are calculated from the 'LS' angle and 'LC'
    // speed.
    _cp_l = _wheelbase / 2.0;
    _cp_l_i = (int)roundf(_cp_l);
    _cp_t = _track / 2.0;
    _cp_t_i = (int)roundf(_cp_t);

    // Calculate more complex, constant, values
    _cp_cnr_d = sqrt((_cp_l * _cp_l) + (_cp_t * _cp_t));
    _rip_angl = atan2(_cp_l, _cp_t);
}
