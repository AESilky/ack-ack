/**
 * @brief EEPROM storage module supporting the MC24LC32 (32k-bits (4k-bytes)).
 * @ingroup eeprom
 *
 * The MicroChip 24LC32 (and 24AA32) is a 32k-bits EEPROM that uses I2C for the
 * interface. The memory is arranged as 128 pages of 32 bytes, for a total of
 * 4k bytes.
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */

#include "eeprom.h"

#include "board.h"

// ############################################################################
//
// Data
//
// ############################################################################
//
static i2c_inst_t* _i2c;
static int _addr;

// ############################################################################
//
// Module Initialization
//
// ############################################################################
//

void eeprom_module_init(i2c_inst_t* i2c, int addr) {
    static bool _initialized = false;

    if (_initialized) {
        board_panic("!!! eeprom_module_init called more than once. !!!");
    }
    _initialized = true;

    _i2c = i2c;
    _addr = addr;

}