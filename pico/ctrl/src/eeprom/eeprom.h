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
 * This module supports two modes:
 * 1. Buffered
 * 2. Immediate
 *
 * Buffered:
 *  Updates are held in a page buffer until either:
 *    a. The `write` method is called.
 *    b. An `set_value` method specifies a different page from what has been buffered.
 *       This writes the page that had been buffered and reads the new page.
 *    c. A 'write timer' expires.
 *
 * Immediate:
 *  The value set with `set_value` is immediately written.
 *
 * The module is initialized to either 'EEPROM_BUFFERED' or 'EEPROM_IMMEDIATE'.
 * The mode can be changed with the `set_mode` method. The 'write timer' time can be
 * set.
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

/** @brief EEPROM Mode */
typedef enum eeprom_mode_ {
    EEPROM_BUFFERED,
    EEPROM_IMMEDIATE
} eeprom_mode_t;

typedef enum eeprom_op_status_ {
    EEPROM_OTHER_ERR    = -4,
    /** Module Not Initialized */
    EEPROM_MNI          = -3,
    /** EEPROM possibly finishing a write operation */
    EEPROM_NOT_READY    = -2,
    EEPROM_TIMEOUT      = -1,
    EEPROM_SUCCESS      = 0,
    EEPROM_PENDING      = 1
} eeprom_op_status_t;

#define EEPROM_PAGE_SIZE 32
#define EEPROM_PAGES 128

/**
 * @brief Check that the EEPROM is available (the module has been initialized).
 *
 * If the board init finds the EEPROM it will initialize the module and the module
 * checks that it can access the EEPROM.
 *
 * @return true The module has been initialized and the EEPROM is available.
 * @return false The module hasn't been initialized or the EEPROM can't be read.
 */
extern bool eeprom_available();

/**
 * @brief Get data from the EEPROM.
 *
 * The operation of this method depends on the mode.
 *
 * Immediate:
 *      The EEPROM address is calculated and a request is sent to the EEPROM for the data.
 *      The data is read directly into the buffer specified in this method call.
 *
 * Buffered:
 *      The page is checked to see if it is already buffered. If it is, the data is copied
 *      from the buffered data to the buffer specified in this method call. If the page is
 *      not currently buffered and the current buffer needs to be written to the EEPROM
 *      (a write operation is required), the current buffer is written to the EEPROM. Once
 *      written, the correct page is read into the internal buffer.
 *
 *      Once the correct page is buffered, the data requested is copied into the buffer
 *      specified in this method call.
 *
 * Either Mode:
 *      It is possible that the EEPROM might be busy finishing a write when the read is
 *      requested. In that case, EEPROM_NOT_READY will be returned. If the EEPROM was busy, retrying
 *      the operation might succeed. If any other error status is returned, retrying isn't
 *      likely to succeed.
 *
 * @param page The page: 0-127
 * @param start The start offset within the page: 0-31
 * @param buf The buffer to copy data into
 * @param size The number of bytes to read
 * @return eeprom_op_status_t Operation status
 */
extern eeprom_op_status_t eeprom_data_get(uint8_t page, uint8_t start, uint8_t* buf, size_t size);

/**
 * @brief Set data to be written to the EEPROM.
 *
 * The operation of this method depends on the mode.
 *
 * Immediate:
 *      An attempt is made to write the data to the EEPROM immediately. The status returned
 *      indicates whether the operation was successful or if there was an error. In this
 *      mode, it is possible that EEPROM_NOT_READY is returned, as the EEPROM might be
 *      busy with a previous write operation. If EEPROM_SUCCESS is returned, it means
 *      the EEPROM accepted the data and started writing it.
 * Buffered:
 *      The data is put into the buffer to be written. If the page isn't the same as the
 *      page currently being buffered, and the bufferred page contains changes, the currently
 *      buffered page is written, then the page specified is loaded.
 *
 *      When the loaded page matches the requested page, the data is copied into the
 *      page buffer and the buffer is marked as needing to be written.
 *
 *      In this mode, it is possible that EEPROM_NOT_READY or EEPROM_OTHER_ERR is returned.
 *      EEPROM_NOT_READY will be returned if a page needs to be written to accept this
 *      data and the EEPROM is busy from a previous write operation.
 *
 *
 * @param page The page: 0-127
 * @param start The start point within the page.
 * @param buf Pointer to the byte buffer to be written.
 * @param size The size of the data buffer. The value of 'start'+'size' must not be larger than a page.
 * @return eeprom_op_status_t eeprom_op_status_t of the operation.
 */
extern eeprom_op_status_t eeprom_data_set(uint8_t page, uint8_t start, uint8_t* buf, size_t size);

/**
 * @brief Write the current buffered page if needed.
 *
 * @return eeprom_op_status_t
 */
extern eeprom_op_status_t eeprom_write();

/**
 * @brief Get the current write mode.
 *
 * @return eeprom_mode_t
 */
extern eeprom_mode_t eeprom_wrmode_get();

/**
 * @brief Set the write mode.
 *
 * If the mode is currently EEPROM_BUFFERED and it is set to EEPROM_IMMEDIATE
 * and there are writes pending, a write will be performed.
 *
 * @param mode The mode to use
 */
extern void eeprom_wrmode_set(eeprom_mode_t mode);

/**
 * @brief Initialize the module.
 *
 * @param i2c i2c_inst_t Instance to use (i2c0 or i2c1)
 * @param addr The I2C address of the device (0x50 or 0x51)
 * @param mode The mode to use: Immediate or Buffered
 */
extern void eeprom_module_init(i2c_inst_t *i2c, int addr, eeprom_mode_t mode);

#ifdef __cplusplus
    }
#endif
#endif // ADC1015_H_
