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
#include "cmt/cmt.h"
#include "util/util.h"

#include <string.h>


#define PAGE_ADDR_BYTES 2 // The first two bytes of the buffer are the page address
#define ADDR_PAGE_SHIFT 5
#define START_SIZE_LEN_MASK 0x001F // Mask to assure that the start/size/len is within page
#define PAGE_NONE (-1)
#define WR_TIME_DEFAULT TEN_SECONDS_MS

// Declarations
//
static int _read_bytes(uint8_t* buf, uint8_t count);
static inline void _set_addr_hl(uint8_t page, uint8_t start);
static eeprom_op_status_t _status_from_rw(int wr_bytes_ret);
static int _write_buffered_page();

typedef struct addr_data_ {
    uint8_t addr_h;
    uint8_t addr_l;
    uint8_t data[EEPROM_PAGE_SIZE];
} addr_data_t;
#define PAGE_CTRL_DATA_BUF_SIZE (PAGE_ADDR_BYTES + EEPROM_PAGE_SIZE)
#define RD_TIME_PER_BYTE_US 100
#define WR_TIME_PER_BYTE_US 100


// ############################################################################
//
// Data
//
// ############################################################################
//
static bool _initialized;
static bool _device_ok;

static i2c_inst_t* _i2c;
static int _addr;
static eeprom_mode_t _mode;
static bool _wr_msg_queued;
static bool _wr_needed;
static bool _wr_requested;
static uint32_t _wr_time_ms;
static int _wr_status;

static addr_data_t _addr_data;
static int _page_buffered;


// ############################################################################
//
// Local Methods
//
// ############################################################################
//

static eeprom_op_status_t _load_page(uint8_t page) {
    // If a write is needed, write the page...
    int status;
    if (_wr_needed) {
        status = _write_buffered_page();
        if (status != PICO_ERROR_NONE) {
            return _status_from_rw(status);
        }
    }
    // Read the required page
    _set_addr_hl(page, 0);
    status = _read_bytes(_addr_data.data, EEPROM_PAGE_SIZE);
    if (status == PICO_ERROR_GENERIC) {
        // The EEPROM is either busy with a previous write or it doesn't exist.
        if (_wr_requested) {
            // It is probably busy with a previous page write...
            // The caller will need to try again.
            return EEPROM_NOT_READY;
        }
    }
    if (status == PICO_ERROR_NONE) {
        _page_buffered = page;
    }
    return _status_from_rw(status);
}

static int _read_bytes(uint8_t* buf, uint8_t count) {
    // Set the address.
    int rs = i2c_write_timeout_us(_i2c, _addr, (uint8_t*)&_addr_data, PAGE_ADDR_BYTES, false, (WR_TIME_PER_BYTE_US * PAGE_ADDR_BYTES));
//    int rs = i2c_write_blocking(_i2c, _addr, (uint8_t*)&_addr_data, PAGE_ADDR_BYTES, false);
    if (rs != PAGE_ADDR_BYTES) {
        return rs;
    }
    // Read 'count' bytes into buffer.
    rs = i2c_read_timeout_us(_i2c, _addr, buf, count, false, (RD_TIME_PER_BYTE_US * (count + 1)));
//    rs = i2c_read_blocking(_i2c, _addr, buf, count, false);
    if (rs == count) {
        _wr_requested = false; // If the read succeeded, it means that any previous write is complete.
        rs = PICO_ERROR_NONE;
    }
    return rs;
}

/**
 * @brief Set the _addr_data addr_h and addr_l from the page and start.
 *
 * @param page Page number: 0-127
 * @param start Byte within page: 0-31
 */
static inline void _set_addr_hl(uint8_t page, uint8_t start) {
    uint16_t addr = (page << ADDR_PAGE_SHIFT) + start;
    _addr_data.addr_h = highByte(addr);
    _addr_data.addr_l = lowByte(addr);
}

static eeprom_op_status_t _status_from_rw(int wr_bytes_ret) {
    if (!_device_ok) {
        return EEPROM_MNI;
    }
    eeprom_op_status_t rv;
    switch (wr_bytes_ret) {
    case PICO_ERROR_NONE:
        rv = EEPROM_SUCCESS;
        break;
    case PICO_ERROR_TIMEOUT:
        rv = EEPROM_TIMEOUT;
        break;
    case PICO_ERROR_GENERIC:
        rv = EEPROM_NOT_READY;
        break;
    default:
        rv = EEPROM_OTHER_ERR;
        break;
    }
    return rv;
}

static void _schd_write_mh(cmt_msg_t *msg) {
    _wr_msg_queued = false;
    _write_buffered_page();
}

static int _write_bytes(uint8_t count) {
        // Write the first byte(s) buffer
    size_t len = (count + PAGE_ADDR_BYTES);
    int rv = i2c_write_timeout_us(_i2c, _addr, (uint8_t*)&_addr_data, len, false, (WR_TIME_PER_BYTE_US * (len + 1)));
//    int rv = i2c_write_blocking(_i2c, _addr, (uint8_t*)&_addr_data, len, false);
    if (rv == len) {
        _wr_requested = true;
        _wr_status = PICO_ERROR_NONE;
    }
    else {
        _wr_status = rv;
    }
    return _wr_status;
}

static int _write_buffered_page() {
    if (_wr_needed) {
        _wr_needed = false;
        // Cancel our scheduled write message
        if (_wr_msg_queued) {
            _wr_msg_queued = false;
            scheduled_msg_cancel2(MSG_EXEC, _schd_write_mh);
        }
        // Write the buffer
        _write_bytes(EEPROM_PAGE_SIZE);
    }
    return _wr_status;
}

// ############################################################################
//
// Public Methods
//
// ############################################################################
//

eeprom_op_status_t eeprom_data_get(uint8_t page, uint8_t start, uint8_t* buf, size_t size) {
    // if (!_device_ok) {
    //     return EEPROM_MNI;
    // }
    eeprom_op_status_t status;
    start &= START_SIZE_LEN_MASK; // Assure that 'start' is within a page.
    // Assure that the start+size doesn't cross a page boundary
    if ((size + start) > EEPROM_PAGE_SIZE) {
        size = EEPROM_PAGE_SIZE - start;
    }
    if (_mode == EEPROM_IMMEDIATE) {
        _set_addr_hl(page, start);
        return _status_from_rw(_read_bytes(buf, size));
    }
    else { // EEPROM_BUFFERED
        if (page != _page_buffered) {
            status = _load_page(page);
            if (status != EEPROM_SUCCESS) {
                return status;
            }
        }
        memcpy(buf, _addr_data.data + start, size);
    }
    return EEPROM_SUCCESS;
}

eeprom_op_status_t eeprom_data_set(uint8_t page, uint8_t start, uint8_t* buf, size_t size) {
    if (!_device_ok) {
        return EEPROM_MNI;
    }
    eeprom_op_status_t status;
    start &= START_SIZE_LEN_MASK; // Assure that 'start' is within a page.
    // Assure that the start+size doesn't cross a page boundary
    if ((size + start) > EEPROM_PAGE_SIZE) {
        size = EEPROM_PAGE_SIZE - start;
    }
    if (_mode == EEPROM_IMMEDIATE) {
        _set_addr_hl(page, start);
        memcpy(_addr_data.data, buf, size);
        return _status_from_rw(_write_bytes(size));
    }
    else { // EEPROM_BUFFERED
        if (page != _page_buffered) {
            status = _load_page(page);
            if (status != EEPROM_SUCCESS) {
                return status;
            }
        }
        // The page is (now) the buffered page, merge this data in.
        memcpy((_addr_data.data + start), buf, size);
        _wr_needed = true;
        if (!_wr_msg_queued) {
            cmt_msg_t msg;
            cmt_exec_init(&msg, _schd_write_mh);
            schedule_msg_in_ms(_wr_time_ms, &msg);
            _wr_msg_queued = true;
        }
    }
    return EEPROM_PENDING;
}

eeprom_op_status_t eeprom_write() {
    if (!_device_ok) {
        return EEPROM_MNI;
    }
    return (_status_from_rw(_write_buffered_page()));
}

eeprom_mode_t eeprom_wrmode_get() {
    return _mode;
}

void eeprom_wrmode_set(eeprom_mode_t mode) {
    if (_device_ok) {
        if (mode == EEPROM_IMMEDIATE && _wr_needed) {
            _write_buffered_page();
        }
        _mode = mode;
    }
}

// ############################################################################
//
// Module Initialization
//
// ############################################################################
//

void eeprom_module_init(i2c_inst_t* i2c, int addr, eeprom_mode_t mode) {
    if (_initialized) {
        board_panic("!!! eeprom_module_init called more than once. !!!");
    }
    _initialized = true;

    _i2c = i2c;
    _addr = addr;
    _mode = mode;
    _wr_time_ms = WR_TIME_DEFAULT;
    _page_buffered = PAGE_NONE;

    // Do a read from the device to make sure it is responding.
    //  We just read 1 byte from page0, byte0, so that little needs to be set
    //  up.
    uint8_t buf[1];
    _addr_data.addr_h = 0;
    _addr_data.addr_l = 0;
    int rs = _read_bytes(buf, 1);
    _device_ok = (rs == PICO_ERROR_NONE);
}
