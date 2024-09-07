/**
 * @brief Support for an INA3221 Power Monitor module.
 *
 * The modules that are readily available use a 0.1Ω shunt resistor.
 * Using Ohm's Law to get current from the measured shunt voltage, we
 * get that a measurement of 1mV would be 10mA
 *      0.001V / 0.1Ω = 0.01A
 *
 * This module avoids using floating point (for speed), so values to/from
 * functions are in µV and µA using 32 bit signed values.
 *
 * The shunt (current portion) measurement of the INA3221 is ±163.8mV
 * (12 bits + sign), so that would be 1.638A
 *
 *
 * Copyright 2023-24 AESilky
 * Portions copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "system_defs.h"    // Include first so I2C_PORT is defined.
#include "pwrmon_3221.h"
#include "board.h"
#include "util/util.h"

#include <stdio.h>

#include "hardware/i2c.h"

// I2C values if not set outside...
#ifndef I2C_PORT
#define I2C_PORT i2c_default
#endif

// device I2C address
#define INA3221_ADDR _u(0x40)

// registers (see data sheet)
#define INA3221_CONFIGURATION_R         _u(0x00)    // RW
#define INA3221_CH1_SHUNT_V_R           _u(0x01)    // R
#define INA3221_CH1_BUS_V_R             _u(0x02)    // R
#define INA3221_CH2_SHUNT_V_R           _u(0x03)    // R
#define INA3221_CH2_BUS_V_R             _u(0x04)    // R
#define INA3221_CH3_SHUNT_V_R           _u(0x05)    // R
#define INA3221_CH3_BUS_V_R             _u(0x06)    // R
#define INA3221_CH1_CRIT_LIM_R          _u(0x07)    // RW
#define INA3221_CH1_WARN_LIM_R          _u(0x08)    // RW
#define INA3221_CH2_CRIT_LIM_R          _u(0x09)    // RW
#define INA3221_CH2_WARN_LIM_R          _u(0x0A)    // RW
#define INA3221_CH3_CRIT_LIM_R          _u(0x0B)    // RW
#define INA3221_CH3_WARN_LIM_R          _u(0x0C)    // RW
#define INA3221_SHUNT_V_SUM_R           _u(0x0D)    // R
#define INA3221_SHUNT_V_SUM_LIM_R       _u(0x0E)    // RW
#define INA3221_MASK_ENABLE_R           _u(0x0F)    // RW
#define INA3221_PWR_VALID_ULIM_R        _u(0x10)    // RW
#define INA3221_PWR_VALID_LLIM_R        _u(0x11)    // RW
#define INA3221_MFG_ID_R                _u(0xFE)    // R
#define INA3221_DIE_ID_R                _u(0xFF)    // R
//
// Configuration Register BIT values. One value from each category (..._BV_n)
// can be OR'ed together to generate a value to be written to the register.
//
// For reading, see the MASK and SHIFT values below.
//
#define INA3221_CFG_RESET_BV            _u(0x8000)      // Reset the device (other bits ignored)
// cat-1 Bit 14 : Channel 1 Enable
#define INA3221_CH1_EN_BV_1             _u(0x4000)      // Enable CH1
// cat-2 Bit 13 : Channel 2 Enable
#define INA3221_CH2_EN_BV_2             _u(0x2000)      // Enable CH2
// cat-3 Bit 12 : Channel 3 Enable
#define INA3221_CH3_EN_BV_3             _u(0x1000)      // Enable CH3
// cat-4 Bits 11-9 : Averaging Mode number of samples to average
#define INA3221_AVGMODE_0001_BV_4       _u(0x0000)      // Use each sample (no averaging) (default)
#define INA3221_AVGMODE_0004_BV_4       _u(0x0200)      // Average 4 samples
#define INA3221_AVGMODE_0016_BV_4       _u(0x0400)      // Average 16 samples
#define INA3221_AVGMODE_0064_BV_4       _u(0x0600)      // Average 64 samples
#define INA3221_AVGMODE_0128_BV_4       _u(0x0800)      // Average 128 samples
#define INA3221_AVGMODE_0256_BV_4       _u(0x0A00)      // Average 256 samples
#define INA3221_AVGMODE_0512_BV_4       _u(0x0C00)      // Average 512 samples
#define INA3221_AVGMODE_1024_BV_4       _u(0x0E00)      // Average 256 samples
// cat-5 Bits 8-6 : V-Bus Conversion Time
#define INA3221_VBUSCT_0140_BV_5        _u(0x0000)      // V-Bus Conversion Time 140µs
#define INA3221_VBUSCT_0204_BV_5        _u(0x0040)      // V-Bus Conversion Time 204µs
#define INA3221_VBUSCT_0332_BV_5        _u(0x0080)      // V-Bus Conversion Time 332µs
#define INA3221_VBUSCT_0588_BV_5        _u(0x00C0)      // V-Bus Conversion Time 588µs
#define INA3221_VBUSCT_1100_BV_5        _u(0x0100)      // V-Bus Conversion Time 1.100ms (default)
#define INA3221_VBUSCT_2116_BV_5        _u(0x0140)      // V-Bus Conversion Time 2.116ms
#define INA3221_VBUSCT_4156_BV_5        _u(0x0180)      // V-Bus Conversion Time 4.156ms
#define INA3221_VBUSCT_8244_BV_5        _u(0x01C0)      // V-Bus Conversion Time 8.244ms
// cat-6 Bits 5-3 : V-Shunt (current) Conversion Time
#define INA3221_VSHNTCT_0140_BV_6       _u(0x0000)      // V-Shunt Conversion Time 140µs
#define INA3221_VSHNTCT_0204_BV_6       _u(0x0008)      // V-Shunt Conversion Time 204µs
#define INA3221_VSHNTCT_0332_BV_6       _u(0x0010)      // V-Shunt Conversion Time 332µs
#define INA3221_VSHNTCT_0588_BV_6       _u(0x0018)      // V-Shunt Conversion Time 588µs
#define INA3221_VSHNTCT_1100_BV_6       _u(0x0020)      // V-Shunt Conversion Time 1.100ms (default)
#define INA3221_VSHNTCT_2116_BV_6       _u(0x0028)      // V-Shunt Conversion Time 2.116ms
#define INA3221_VSHNTCT_4156_BV_6       _u(0x0030)      // V-Shunt Conversion Time 4.156ms
#define INA3221_VSHNTCT_8244_BV_6       _u(0x0038)      // V-Shunt Conversion Time 8.244ms
// cat-7 Bits 0-2 : Operating Mode
#define INA3221_MODE_PWRDWN_BV_7        _u(0x0000)      // Power down the device (can still be read from)
#define INA3221_MODE_SHNTV_SS_BV_7      _u(0x0001)      // Shunt voltage, single-shot (triggered)
#define INA3221_MODE_BUSV_SS_BV_7       _u(0x0002)      // Bus voltage, single-shot (triggered)
#define INA3221_MODE_SBV_SS_BV_7        _u(0x0003)      // Shunt & Bus voltage, single-shot (triggered)
#define INA3221_MODE_PWRDWN2_BV_7       _u(0x0004)      // Power down the device (can still be read from)
#define INA3221_MODE_SHNTV_BV_7         _u(0x0005)      // Shunt voltage, continuous
#define INA3221_MODE_BUSV_BV_7          _u(0x0006)      // Bus voltage, continuous
#define INA3221_MODE_SBV_BV_7           _u(0x0007)      // Shunt & Bus voltage, continuous
//
// Configuration value MASK values. Use the MASK after reading the Configuration value
// and then use the SHIFT.
//
// Channel 1 Enable (MASK Bit 14)
#define INA3221_CH1_EN_MASK             _u(0x4000)
// Channel 2 Enable (MASK Bit 13)
#define INA3221_CH2_EN_MASK             _u(0x2000)
// Channel 3 Enable (MASK Bit 12)
#define INA3221_CH3_EN_BV_3             _u(0x1000)
// Averaging Mode - Number of samples to average (MASK Bits 11-9)
#define INA3221_AVGMODE_MASK            _u(0x0E00)
// V-Bus Conversion Time (MASK Bits 8-6)
#define INA3221_VBUSCT_MASK             _u(0x01C0)
// V-Shunt (current) Conversion Time (MASK Bits 5-3)
#define INA3221_VSHNTCT_MASK            _u(0x0038)
// Operating Mode (MASK Bits 0-2)
#define INA3221_MODE_PWRDWN_MASK        _u(0x0007)
//
// Configuration value SHIFT values. Use these to shift right after
// reading the value and masking.
//
// Channel 1 Enable (SHIFT Bit 14)
#define INA3221_CH1_EN_SHIFT            14
// Channel 2 Enable (SHIFT Bit 13)
#define INA3221_CH2_EN_SHIFT            13
// Channel 3 Enable (SHIFT Bit 12)
#define INA3221_CH3_EN_SHIFT            12
// Averaging Mode - Number of samples to average (SHIFT Bits 11-9)
#define INA3221_AVGMODE_SHIFT            9
// V-Bus Conversion Time (SHIFT Bits 8-6)
#define INA3221_VBUSCT_SHIFT             6
// V-Shunt (current) Conversion Time (SHIFT Bits 5-3)
#define INA3221_VSHNTCT_SHIFT            3
// Operating Mode (SHIFT Bits 0-2)
#define INA3221_MODE_SHIFT               0
//
// Mask/Enable Register MASK values. Use the MASK after reading the Configuration
// value and then use the SHIFT.
//
// Sum CH 1 Enable (MASK Bit 14)
#define INA3221_CH1_SUM_EN_MASK          _u(0x4000)
// Sum CH 2 Enable (MASK Bit 13)
#define INA3221_CH2_SUM_EN_MASK          _u(0x2000)
// Sum CH 3 Enable (MASK Bit 12)
#define INA3221_CH3_SUM_EN_MASK          _u(0x1000)
// Warn Alert Latch Enable (MASK Bit 11)
#define INA3221_WARN_ALRT_LATCH_EN_MASK  _u(0x0800)
// Critical Alert Latch Enable (MASK Bit 10)
#define INA3221_CRIT_ALRT_LATCH_EN_MASK  _u(0x0400)
// Critical Alert Flag CH1 (MASK Bit 9)
#define INA3221_CRIT_ALRT_CH1_MASK       _u(0x0200)
// Critical Alert Flag CH2 (MASK Bit 8)
#define INA3221_CRIT_ALRT_CH2_MASK       _u(0x0100)
// Critical Alert Flag CH3 (MASK Bit 7)
#define INA3221_CRIT_ALRT_CH3_MASK       _u(0x0080)
// Critical Alert Flags ALL (MASK Bits 9-7)
#define INA3221_CRIT_ALRT_ALL_MASK       _u(0x0380)
// Summation Alert Flag (MASK Bit 6)
#define INA3221_SUM_ALRT_MASK            _u(0x0040)
// Warning Alert Flag CH1 (MASK Bit 5)
#define INA3221_WARN_ALRT_CH1_MASK       _u(0x0020)
// Warning Alert Flag CH2 (MASK Bit 4)
#define INA3221_WARN_ALRT_CH2_MASK       _u(0x0010)
// Warning Alert Flag CH3 (MASK Bit 3)
#define INA3221_WARN_ALRT_CH3_MASK       _u(0x0008)
// Warning Alert Flags ALL (MASK Bits 5-3)
#define INA3221_WARN_ALRT_ALL_MASK       _u(0x0038)
// Power Valid Alert Flag (MASK Bit 2)
#define INA3221_PWR_VALID_ALRT_MASK      _u(0x0004)
// Timing Control Alert Flag (MASK Bit 1)
#define INA3221_TIM_CTRL_ALRT_MASK       _u(0x0002)
// Power Valid Alert Flag (MASK Bit 0)
#define INA3221_CONVERSION_RDY_MASK      _u(0x0001)

typedef struct _init_pair_ {
    uint8_t reg;
    uint16_t val;
} init_pair_t;

static init_pair_t _init_pairs[] = {
    {INA3221_CONFIGURATION_R,
        INA3221_CH1_EN_BV_1 |
        INA3221_CH2_EN_BV_2 |
        INA3221_CH3_EN_BV_3 |
        INA3221_AVGMODE_0004_BV_4 |
        INA3221_VBUSCT_0204_BV_5 |
        INA3221_VSHNTCT_0204_BV_6 |
        INA3221_MODE_SBV_BV_7},
    {INA3221_CH1_CRIT_LIM_R, (100000 / 5)},  // Critical = 100mA (value is µA/5)
    {INA3221_CH1_WARN_LIM_R, (10000 / 5)},   // Warn = 10mA (value is µA/5)
    {INA3221_CH2_CRIT_LIM_R, (100000 / 5)},  // Critical = 100mA (value is µA/5)
    {INA3221_CH2_WARN_LIM_R, (10000 / 5)},   // Warn = 10mA (value is µA/5)
    {INA3221_CH3_CRIT_LIM_R, (100000 / 5)},  // Critical = 100mA (value is µA/5)
    {INA3221_CH3_WARN_LIM_R, (10000 / 5)},   // Warn = 10mA (value is µA/5)
};

typedef struct _chnl_regs {
    uint8_t shunt_volts;
    uint8_t bus_volts;
    uint8_t crit_limit;
    uint8_t warn_limit;
} channel_regs_t;

/** @brief Array of the channel specific registers. Initialized with values for the 3 channels */
static channel_regs_t _channel_regs[3];

static inline uint8_t _highbyte(uint16_t d) {
    return ((d & 0xFF00) >> 8);
}

static inline uint8_t _lowbyte(uint16_t d) {
    return (d & 0x00FF);
}

static void _write_register(uint8_t reg, uint16_t data) {
    // I2C write process expects a register address byte followed by data
    uint8_t buf[3] = {reg, _highbyte(data), _lowbyte(data)};
    i2c_write_blocking(I2C_PORT, (INA3221_ADDR), buf, 3, false);
}

static uint32_t _read_register(uint8_t reg) {
    uint8_t buf[2] = {reg};
    i2c_write_blocking(I2C_PORT, INA3221_ADDR, buf, 1, false);
    i2c_read_blocking(I2C_PORT, INA3221_ADDR, buf, 2, false);
    uint32_t data = (buf[0] << 8 | buf[1]);
    return (data);
}

int32_t pwrmon_current(pwrchan_t channel) {
    channel_regs_t *chregs = &_channel_regs[channel];
    int16_t rawval = _read_register(chregs->shunt_volts);
    int32_t adjval = rawval;
    // If rawval is negative, we must do adjustments before converting to µA
    // (don't assume the machine uses 2's compliment for math)
    if (rawval & 0x8000) {
        adjval = ((rawval ^ 0xFFFF) + 1); // 2's compliment
    }
    // Now we have a positive value from 0-0x7FF8. This is the 12 bit ADC value
    // that has been shifted left 3 bits. Bit 3 (pre-shifted Bit 0) is 40µV.
    // So, we need to shift it back 3 bits and multiply by 40 to true get µV. This is
    // the same as dividing by 8 and multiplying by 40, or simply multiplying by 5.
    int32_t micro_volts = ((adjval * 5) * (rawval < 0 ? -1 : 1));
    // The value is now in µV. The module uses 0.1Ω shunt resistors, so the current
    // is (using Ohm's Law): I = V/R => micro_volts / 0.1 => micro_volts * 10
    int32_t micro_amps = micro_volts * 10;

    return (micro_amps);
}

int32_t pwrmon_bus_voltage(pwrchan_t channel) {
    channel_regs_t *chregs = &_channel_regs[channel];
    int16_t rawval = _read_register(chregs->bus_volts);
    int32_t adjval = rawval;
    // If rawval is negative, we must do adjustments before converting to µA
    // (don't assume the machine uses 2's compliment for math)
    if (rawval & 0x8000) {
        adjval = ((rawval ^ 0xFFFF) + 1); // 2's compliment
    }
    // Now we have a positive value from 0-0x7FF8. This is the 12 bit ADC value
    // that has been shifted left 3 bits. Bit 3 (pre-shifted Bit 0) is 8mV.
    // So, we need to shift it back 3 bits and multiply by 8 to get true mV. This is
    // the same as dividing by 8 and multiplying by 8, so the value is actually
    // in mV.
    int32_t micro_volts = (adjval * 1000);

    return (micro_volts);
}


void pwrmon_module_init(void) {
    // Try to read the MFG register
    uint32_t d = _read_register(INA3221_MFG_ID_R);
    if (d != 0x5449) { // Datasheet says that it is 'TI'.
        error_printf("PWRMON %d", PWRERR_MFG);
    }
    // Now the config register
    //d = _read_register(INA3221_CONFIGURATION_R);
    //printf("%04X\n", d);

    // Set up our structures
    _channel_regs[PWRCH1].bus_volts = INA3221_CH1_BUS_V_R;
    _channel_regs[PWRCH2].bus_volts = INA3221_CH2_BUS_V_R;
    _channel_regs[PWRCH3].bus_volts = INA3221_CH3_BUS_V_R;
    _channel_regs[PWRCH1].shunt_volts = INA3221_CH1_SHUNT_V_R;
    _channel_regs[PWRCH2].shunt_volts = INA3221_CH2_SHUNT_V_R;
    _channel_regs[PWRCH3].shunt_volts = INA3221_CH3_SHUNT_V_R;
    _channel_regs[PWRCH1].crit_limit = INA3221_CH1_CRIT_LIM_R;
    _channel_regs[PWRCH2].crit_limit = INA3221_CH2_CRIT_LIM_R;
    _channel_regs[PWRCH3].crit_limit = INA3221_CH3_CRIT_LIM_R;
    _channel_regs[PWRCH1].warn_limit = INA3221_CH1_WARN_LIM_R;
    _channel_regs[PWRCH2].warn_limit = INA3221_CH2_WARN_LIM_R;
    _channel_regs[PWRCH3].warn_limit = INA3221_CH3_WARN_LIM_R;

    // Now reset the module
    _write_register(INA3221_CONFIGURATION_R, INA3221_CFG_RESET_BV);

    // Write the initial register values
    for (int i = 0; i < ARRAY_ELEMENT_COUNT(_init_pairs); i++) {
        uint8_t reg = _init_pairs[i].reg;
        uint16_t val = _init_pairs[i].val;
        _write_register(reg, val);
    }
}

