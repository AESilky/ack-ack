/**
 * @brief INA3221 3 Channel Current & Voltage Power Monitor w/I2C.
 *
 * Copyright 2023-24 AESilky
 * Portions copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _PWRMON_3221_H_
#define _PWRMON_3221_H_
#ifdef __cplusplus
 extern "C" {
#endif

/**
 * @brief Power Monitor Channel IDs.
 *
 * These are in numerical order starting with `0` so they can be
 * used as array indexes.
 */
typedef enum _pwrchan_ {
    PWRCH1 = 0,
    PWRCH2 = 1,
    PWRCH3 = 2,
} pwrchan_t;

/**
 * @brief Power Monitor Error IDs
 */
typedef enum _pwrerr_ {
    PWRERR_MFG = 1,
} pwrerr_t;

/**
 * @brief Read the last recorded channel current value in µA.
 *
 * µA are used to avoid using any floating point operations.
 *
 * @param channel The channel number to read.
 * @return int32_t The current in µA
 */
extern int32_t pwrmon_current(pwrchan_t channel);

/**
 * @brief Read the last recorded channel BUS voltage in µV.
 *
 * µV are used to avoid using any floating point operations. For the
 * BUS, mV are actually fine (no floating point required), but micros
 * are used for consistency across measurements.
 *
 * @param channel The channel number to read.
 * @return int32_t The BUS voltage in µV
 */
extern int32_t pwrmon_bus_voltage(pwrchan_t channel);

/**
 * @brief Initialize the Power Module module.
 *
 */
extern void pwrmon_module_init(void);

#ifdef __cplusplus
}
#endif
#endif // _PWRMON_3221_H_
