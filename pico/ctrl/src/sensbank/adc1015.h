/**
 * @brief Analog to Digital Converter ADS1015 module.
 * @ingroup sensbank
 *
 * The ADS1015 is a member of the TI-ADS101x-Q1 family that includes a 4-input analog
 * mux/selector. The other members of the family do not include the mux/selector.
 *
 * This module specifically targets the ADS1015, rather than trying to be general
 * purpose. It uses the 'housekeeping' method to read each of the inputs in succession.
 * It makes the readings available via a channel value method. An input is read every
 * n'th `housekeeping` call (where 'n' is set through a configuration method), so all
 * four channel values are read every 4n `housekeeping` calls (4n*16ms).
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef ADC1015_H_
#define ADC1015_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "hardware/i2c.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Housekeeping method where work is done. This should be called from the
 *      housekeeping message handler that is using this module.
 *
 * This module doesn't register a message handler. Rather, it relies on this method
 * being called regularly.
 */
extern void adc1015_housekeeping();

/**
 * @brief Check if the ADC1015 is running (collecting samples)
 *
 * @return true Samples are being collected
 * @return false Device is idle/sleeping
 */
extern bool adc1015_is_running();

/**
 * @brief Set the rate at which samples are read and the input advanced. This is the
 *      number of `housekeeping` calls that will be processed before a sample is
 *      read from the ADS1015 device and the mux/selector moved to the next input.
 *
 * @param r Number of `housekeeping` calls to take to read a sample
 */
extern void adc1015_sample_rate(int r);

/**
 * @brief Stop sampling and allow the ADS1015 to go into sleep (power save) mode.
 *
 * @see `adc1015_sample()` to take the ADS1015 out of sleep mode
 *
 */
extern void adc1015_sleep();

/**
 * @brief Start the ADS1015 sampling.
 *
 * @see `adc1015_sleep()` to put the ADS1015 into sleep (power save) mode
 *
 */
extern void adc1015_start();

/**
 * @brief Get the last value read for the requested input (0-3).
 *
 * Values are read from the ADS1015 in a round-robin fashion at a rate set by a call
 * to the `adc1015_sample_rate(int r)` method. This method returns the last value
 * read for the requested input (0-3).
 *
 * @param input The input to get the value for. The input will be masked to a 0-3 value.
 * @return int16_t The last value read for the input. The value is a signed 12-bit int.
 */
extern int16_t adc1015_value(uint8_t input);

/**
 * @brief Initialize the module and the ADS1015. Optionally, initialize the I2C instance.
 *
 * This initializes the module and optionally the I2C instance, and set the sample rate.
 * This does not start the sampling. Use the `adc1015_start` method to start sampling.
 *
 * @see adc1015_sample_rate()
 * @see adc1015_start()
 *
 * @param i2c i2c_inst_t Instance to use (i2c0 or i2c1)
 * @param init_i2c True to also initialize the I2C instance for use
 * @param addr The I2C address of the device (0x48 or 0x49)
 * @param rate The sample rate to use (@see `adc1015_sample_rate`)
 */
extern void adc1015_module_init(i2c_inst_t *i2c, int addr, int rate);

#ifdef __cplusplus
    }
#endif
#endif // ADC1015_H_
