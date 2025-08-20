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

#include "adc1015.h"

#include "board.h"

/* ADS1015 Registers */
#define REG_CONVERSION          (0x00)
#define REG_CONFIG              (0x01)
#define REG_THRESH_LO           (0x02)
#define REG_THRESH_HI           (0x03)

/* Configuration Bits - Defined as MSB and LSB to be written to the device */
/** Operation Status. Single Sample Start. (MSB) */
#define CFG_OS_BITS             (0x80)   // (0x8000) // 1xxx xxxx xxxx xxxx
#define CFG_OS_WR_NOP           (0x00)   // (0x0000)
#define CFG_OS_WR_START         (0x80)   // (0x8000)
#define CFG_OS_RD_BUSY          (0x00)   // (0x0000)
#define CFG_OS_RD_IDLE          (0x80)   // (0x8000)
/** MUX (input selection) (MSB) */
#define CFG_MUX_BITS            (0x70)   // (0x7000) // x111 xxxx xxxx xxxx
#define CFG_MUX_SNGLEND         (0x40)   // (0x4000) // This is used in addition to a 0-3 shifted value
#define CFG_MUX_INPUT_SHIFT     (4)      // (12) Number of bits to shift the Input-Select left
/** PGA (Programmable Gain Amplifier) (MSB) */
#define CFG_PGA_BITS            (0x0E)   // (0x0E00) // xxxx 111x xxxx xxxx
#define CFG_PGA_6144            (0x00)   // (0x0000) // ±6.144 V
#define CFG_PGA_4096            (0x02)   // (0x0200) // ±4.096 V
#define CFG_PGA_2048            (0x04)   // (0x0400) // * ±2.048 V
#define CFG_PGA_1024            (0x06)   // (0x0600) // ±1.024 V
#define CFG_PGA_0512            (0x08)   // (0x0800) // ±0.512 V
#define CFG_PGA_0256            (0x0A)   // (0x0A00) // ±0.256 V
#define CFG_PGA_0256b           (0x0C)   // (0x0C00) // ±0.256 V
#define CFG_PGA_0256c           (0x0E)   // (0x0E00) // ±0.256 V
/** Mode (MSB) */
#define CFG_MODE_BITS           (0x01)   // (0x0100) // xxxx xxx1 xxxx xxxx
#define CFG_MODE_CONT           (0x00)   // (0x0000) // Continuous Conversion
#define CFG_MODE_SS             (0x01)   // (0x0100) // * Single-Shot Conversion
/** Data Rate (Samples-Per-Second SPS) (LSB) */
#define CFG_DR_BITS             (0xE0)   // (0x00E0) // xxxx xxxx 111x xxxx
#define CFG_DR_0128             (0x00)   // (0x0000) // 128 SPS
#define CFG_DR_0250             (0x20)   // (0x0020) // 250 SPS
#define CFG_DR_0490             (0x40)   // (0x0040) // 490 SPS
#define CFG_DR_0920             (0x60)   // (0x0060) // 920 SPS
#define CFG_DR_1600             (0x80)   // (0x0080) // * 1600 SPS
#define CFG_DR_2400             (0xA0)   // (0x00A0) // 2400 SPS
#define CFG_DR_3300             (0xC0)   // (0x00C0) // 3300 SPS
#define CFG_DR_3300b            (0xE0)   // (0x00E0) // 3300 SPS
/** Comparator Mode (LSB) */
#define CFG_COMP_MODE_BITS      (0x10)   // (0x0010) // xxxx xxxx xxx1 xxxx
#define CFG_COMP_MODE_STD       (0x00)   // (0x0000) // Standard/Traditional Comparator (trigger high, reset low)
#define CFG_COMP_MODE_WIN       (0x10)   // (0x0010) // Window Comparator
/** Comparator Pin Polarity (LSB) */
#define CFG_COMP_POLARITY_BITS  (0x08)   // (0x0008) // xxxx xxxx xxxx 1xxx
#define CFG_COMP_POLARITY_AL    (0x00)   // (0x0000) // * Active Low
#define CFG_COMP_POLARITY_AH    (0x08)   // (0x0008) // Active High
/** Comparator Latching (LSB) */
#define CFG_COMP_LATCHING_BITS  (0x04)   // (0x0004) // xxxx xxxx xxxx x1xx
#define CFG_COMP_LATCH_OFF      (0x00)   // (0x0000) // * Non-latching Comparator
#define CFG_COMP_LATCH_ON       (0x04)   // (0x0004) // Latching Comparator
/** Comparator Queue Size and Disable (LSB) */
#define CFG_COMP_QUE_BITS       (0x03)   // (0x0003) // xxxx xxxx xxxx xx11
#define CFG_COMP_QUE_AA1        (0x00)   // (0x0000) // Assert after 1 sample
#define CFG_COMP_QUE_AA2        (0x01)   // (0x0001) // Assert after 2 samples
#define CFG_COMP_QUE_AA4        (0x02)   // (0x0002) // Assert after 4 samples
#define CFG_COMP_QUE_DIS        (0x03)   // (0x0003) // Disable the comparator and set ALERT/RDY to HIGH-Z

/** Configuration MSB For Stopped (Single-Shot not started) */
#define DEVICE_CFG_STOPPED_MSB  (CFG_OS_WR_NOP | CFG_MUX_SNGLEND | CFG_PGA_4096 | CFG_MODE_SS)
/** Configuration LSB For Stopped (Single-Shot not started) */
#define DEVICE_CFG_STOPPED_LSB  (CFG_DR_0128 | CFG_COMP_MODE_STD | CFG_COMP_POLARITY_AL | CFG_COMP_LATCH_OFF | CFG_COMP_QUE_DIS)
/** Configuration MSB For Running (Continuous and running) */
#define DEVICE_CFG_RUNNING_MSB  (CFG_OS_WR_START | CFG_MUX_SNGLEND | CFG_PGA_4096 | CFG_MODE_CONT)
/** Configuration LSB For Running (Continuous and running) */
#define DEVICE_CFG_RUNNING_LSB  (CFG_DR_0128 | CFG_COMP_MODE_STD | CFG_COMP_POLARITY_AL | CFG_COMP_LATCH_OFF | CFG_COMP_QUE_DIS)


static int _addr;
static i2c_inst_t *_i2c;
static bool _running;

static uint16_t _input;
static int16_t _values[4];

#define I2C_TIMEOUT 10000

// ###############################################################################
// ##                                                                           ##
// ## Local Methods                                                             ##
// ##                                                                           ##
// ###############################################################################

static void _init_device() {
    uint8_t buf[3];
    buf[0] = REG_CONFIG;
    buf[1] = DEVICE_CFG_STOPPED_MSB;
    buf[2] = DEVICE_CFG_STOPPED_LSB;
    i2c_write_timeout_per_char_us(_i2c, _addr, buf, 3, false, I2C_TIMEOUT);
}

static void _run_continuous(uint16_t input) {
    uint8_t buf[3];
    uint16_t aisel = (input << CFG_MUX_INPUT_SHIFT);
    buf[0] = REG_CONFIG;
    buf[1] = DEVICE_CFG_RUNNING_MSB | aisel;
    buf[2] = DEVICE_CFG_RUNNING_LSB;
    i2c_write_timeout_per_char_us(_i2c, _addr, buf, 3, false, I2C_TIMEOUT);
}

// ###############################################################################
// ##                                                                           ##
// ## Public Methods                                                            ##
// ##                                                                           ##
// ###############################################################################

void adc1015_update() {
    if (_running) {
        uint8_t buf[3];
        // Point to Conversion register
        buf[0] = REG_CONVERSION;
        i2c_write_timeout_us(_i2c, _addr, buf, 1, false, I2C_TIMEOUT);
        // Read the current value
        i2c_read_timeout_us(_i2c, _addr, buf, 2, false, I2C_TIMEOUT);
        _values[_input] = ((int16_t)((buf[0] << 8) | buf[1])) / 16; // Value is 12 bits
        // Move to the next input
        _input = ((_input + 1) & 0x00000003);
        _run_continuous(_input);
    }
}

bool adc1015_is_running() {
    return _running;
}

void adc1015_sleep() {
    // _init_device puts it into stopped/sleep state
    _running = false;
    _init_device();
}

void adc1015_start() {
    _run_continuous(_input);
    _running = true;
}

int16_t adc1015_value(uint8_t input) {
    return _values[(input & 0x03)]; // Assure that the index is 0-3
}

float adc1015_volts(int16_t value) {
    float v = (float)value * 0.002; // 2mV is the LSB when gain is set to 4.096V limit
    v = ((v < 0) ? 0 : v);
    return v;
}


// ###############################################################################
// ##                                                                           ##
// ## Initialization Methods                                                    ##
// ##                                                                           ##
// ###############################################################################

void adc1015_module_init(i2c_inst_t *i2c, int addr) {
    static bool _initialized = false;

    if (_initialized) {
        board_panic("!!! adc1015_module_init called more than once. !!!");
    }
    _initialized = true;

    _running = false;
    _i2c = i2c;
    _addr = addr;

    _input = 0;
    _values[3] = _values[2] = _values[1] = _values[0] = 0;

    // Initialize the ADS1015
    _init_device();
}
