/*!
 * \brief Information (constants/definitions) about the rover.
 * \ingroup rover
 *
 * This contains the dimensions, servo IDs, etc. for the Rover.
 *
 * Note:
 * 1. Dimensions are in millimeters unless indicated otherwise.
 * 2. The rover_info module calculates dimensions from formulas during initialization.
 *
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef _ROVER_INFO_H_
#define _ROVER_INFO_H_
#ifdef __cplusplus
extern "C" {
#endif

// Servo IDs
//
/** Servo ID Drive Right-Front */
#define SRVO_ID_DRV_RF 11
/** Servo ID Drive Right-Middle */
#define SRVO_ID_DRV_RM 13
/** Servo ID Drive Right-Rear */
#define SRVO_ID_DRV_RR 15
/** Servo ID Drive Left-Front */
#define SRVO_ID_DRV_LF 10
/** Servo ID Drive Left-Middle */
#define SRVO_ID_DRV_LM 12
/** Servo ID Drive Left-Rear */
#define SRVO_ID_DRV_LR 14
//
/** Servo ID Direction (turn) Right-Front */
#define SRVO_ID_DIR_RF 3
/** Servo ID Direction (turn) Right-Rear */
#define SRVO_ID_DIR_RR 5
/** Servo ID Direction (turn) Left-Front */
#define SRVO_ID_DIR_LF 2
/** Servo ID Direction (turn) Left-Rear */
#define SRVO_ID_DIR_LR 4

/**
 * @brief Distance from rover center-point to a corner drive wheel (center-point).
 *
 * @return float
 */
extern float rover_cp_cnr_d();

/**
 * @brief Rover center-point length (CP to CL of front or back wheelbase-line)
 *
 * @return float
 */
extern float rover_cp_l();

/**
 * @brief Half of the wheelbase in millimeters (as an integer)
 *
 * @return int Half of the wheelbase (rounded)
 */
extern int rover_cp_l_i();

/**
 * @brief Rover center-point track (CP to CL of left or right track-line)
 *
 * @return float
 */
extern float rover_cp_t();

/**
 * @brief Half of the track value (as an integer)
 *
 * @return int Half of the track (rounded int)
 */
extern int rover_cp_t_i();

/**
 * @brief The angle for the (front-right) drive wheel required for 'Rotate-In-Place'.
 *
 * @return float Angle in Radians
 */
extern float rover_rip_angl();

/**
 * @brief The rover track in millimeters.
 *
 * @see `rover_track_i` for an integer value.
 *
 * @return float The track (left wheels centerline to right wheels centerline)
 */
extern float rover_track();

/**
 * @brief The rover track in millimeters (as an integer)
 *
 * @return int
 */
extern int rover_track_i();

/**
 * @brief Wheel circumference in millimeters.
 *
 * @return float circumference
 */
extern float rover_wheel_cir();

/**
 * @brief Wheel diameter in millimeters.
 *
 * @return int diameter
 */
extern int rover_wheel_dia();

/**
 * @brief The rover wheelbase in millimeters.
 *
 * @see `rover_wheelbase_i` for an integer value.
 *
 * @return float The wheelbase (front wheel centerline to rear wheel centerline)
 */
extern float rover_wheelbase();

/**
 * @brief The rover wheelbase in millimeters (as an integer)
 *
 * @see `rover_wheelbase` for the value as a float.
 *
 * @return int The wheelbase
 */
extern int rover_wheelbase_i();

/**
 * @brief Initialize the Rover Info module.
 *
 * This performs all of the calculations to derive that various lengths,
 * angles, multipliers for the rover's static values.
 *
 */
extern void rover_info_module_init();

#ifdef __cplusplus
}
#endif
#endif // _ROVER_INFO_H_

