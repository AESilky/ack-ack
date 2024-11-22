/*!
 * \brief Information (constants/definitions) about the rover.
 * \ingroup board
 *
 * This contains the dimensions, servo IDs, etc. for the Rover.
 * 
 * All:
 * 1. dimensions are in millimeters unless indicated otherwise.
 *
 * Copyright 2023-24 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef _ROVER_INFO_H_
#define _ROVER_INFO_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>

#define ROVER_DIM_TRACK 360 // Width, wheel centerline to wheel centerline
#define ROVER_DIM_WHEELBASE 600 // Wheelbase, front axle to rear axle

#define ROVER_DIM_CL_2W (ROVER_DIM_TRACK / 2)   // Rover Centerline to Middle Drive Wheel CL (width)
#define ROVER_DIM_CL_2L (ROVER_DIM_WHEELBASE / 2) // Rover WB Centerline to Front/Rear Axle 

#define ROVER_DIM_CL_HYP (sqrt((ROVER_DIM_CL_2W * ROVER_DIM_CL_2W) + (ROVER_DIM_CL_2L * ROVER_DIM_CL_2L)))
#define ROVER_ANGL_RIP (asin(ROVER_DIM_CL_2W / ROVER_DIM_CL_HYP))  // Directional wheel angle for Rotate-In-Place

#ifdef __cplusplus
}
#endif
#endif // _ROVER_INFO_H_

