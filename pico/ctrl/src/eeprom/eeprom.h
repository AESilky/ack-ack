/**
 * @brief EEPROM storage module supporting the MC24LC32 (32k-bits (4k-bytes)).
 * @ingroup eeprom
 *
 * The MicroChip 24LC32 (and 24AA32) is a 32k-bits EEPROM that uses I2C for the
 * interface. The memory is arranged as 128 pages of 32 bytes, for a total of
 * 4k bytes.
 *
 * When writing values, writing a single byte internally (in the MC24LC32) writes
 * the complete page the byte is contained within, so it is most efficient to
 * write a page of 32 bytes at once. This accomplishes two things, it is faster,
 * as the device only incurs the write time delay once for the 32 bytes, and it
 * also extends the life of the device, as it only 'counts' as a single write
 * operation.
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
 * @brief Initialize the module.
 *
 * This initializes the module
 *
 * @param i2c i2c_inst_t Instance to use (i2c0 or i2c1)
 * @param addr The I2C address of the device (0x48 or 0x49)
 */
extern void eeprom_module_init(i2c_inst_t *i2c, int addr);

#ifdef __cplusplus
    }
#endif
#endif // ADC1015_H_
