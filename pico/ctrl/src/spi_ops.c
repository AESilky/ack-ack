/**
 * hwctrl SPI operations.
 *
 * The SPI is used by 3 devices, so this provides routines
 * to read/write to them in a coordinated way.
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#include "system_defs.h"
#include "spi_ops.h"
#include "board.h"

#include "hardware/spi.h"
#include "pico/sem.h"

typedef struct _Spi_Dev_Sem_ {
    volatile spi_device_sel_t device;
    volatile uint corenum;
    semaphore_t sem;
} spi_dev_sem_t;

static spi_dev_sem_t _dev_passkey;     // Passkey used to control singular access to the SPI device select

static void _owns_passkey(spi_device_sel_t device) {
    // Check that the device owns the Passkey.
    if (!(device == _dev_passkey.device && get_core_num() == _dev_passkey.corenum && sem_available(&_dev_passkey.sem) == 0)) {
        error_printf(true, "PANIC: SPI Device Op w/o passkey. %u:%u", device, _dev_passkey.device);
        panic("SPI Device Op w/o passkey. %u:%u", device, _dev_passkey.device);
    }
}

static void _begin(spi_device_sel_t device) {
    sem_acquire_blocking(&_dev_passkey.sem);
    _dev_passkey.device = device;
    _dev_passkey.corenum = get_core_num();
}

static void _end(spi_device_sel_t device) {
    _owns_passkey(device);
    _dev_passkey.device = SPI_NONE_SELECT;
    _dev_passkey.corenum = -1;
    sem_release(&_dev_passkey.sem);
}

static void _device_select(spi_device_sel_t device) {
    // The value of the spi_device_cs_t is the 2-bit address value that needs to be
    // written to the SPI Address Selector.
    //
    // The high bit is written to one GPIO and the low bit is written to another.
    //
    // Check that either NONE is being selected, or that the device being selected
    // owns the Passkey.
    if (device != SPI_NONE_SELECT) {
        _owns_passkey(_dev_passkey.device);
    }
    uint8_t hbit = (device & 0x0002) >> 1;
    uint8_t lbit = (device & 0x0001);
    uint32_t value = (hbit << SPI_ADDR_1) | (lbit << SPI_ADDR_0);
    gpio_put_masked(SPI_ADDR_MASK, value);
}

static int _read_buf(spi_inst_t* spi, uint8_t txv, uint8_t* dst, size_t len) {
    return (spi_read_blocking(spi, txv, dst, len));
}

static uint8_t _read8(spi_inst_t* spi, uint8_t txv) {
    uint8_t v;
    int r = spi_read_blocking(spi, txv, &v, 1);
    if (r != 1) {
        v = 0;
    }
    return v;
}

static int _write8(spi_inst_t* spi, uint8_t data) {
    return (spi_write_blocking(spi, &data, 1));
}

static int _write8_buf(spi_inst_t* spi, const uint8_t* buf, size_t len) {
    return (spi_write_blocking(spi, buf, len));
}

static int _write16(spi_inst_t* spi, uint16_t data) {
    size_t len = sizeof(uint16_t);
    uint8_t bytes[] = {(data & 0xff00) >> 8, data & 0xff};
    spi_write_blocking(spi, bytes, len);
    return (len);
}

static int _write16_buf(spi_inst_t* spi, const uint16_t* buf, size_t len) {
    int written = 0;
    while (len--) {
        _write16(spi, *buf);
        buf++;
        written++;
    }
    return (written);
}

void spi_display_begin(void) {
    _begin(SPI_DISPLAY_SELECT);
}

void spi_display_end(void) {
    _end(SPI_DISPLAY_SELECT);
}

int spi_display_read_buf(uint8_t txval, uint8_t* dst, size_t len) {
    int r = _read_buf(SPI_DISP_EXP_DEVICE, txval, dst, len);
    return r;
}

uint8_t spi_display_read8(uint8_t txval) {
    return (_read8(SPI_DISP_EXP_DEVICE, txval));
}

void spi_display_select() {
    _device_select(SPI_DISPLAY_SELECT);
}

int spi_display_write8(uint8_t data) {
    int r = _write8(SPI_DISP_EXP_DEVICE, data);
    return r;
}

int spi_display_write8_buf(const uint8_t* buf, size_t len) {
    int r = _write8_buf(SPI_DISP_EXP_DEVICE, buf, len);
    return r;
}

int spi_display_write16(uint16_t data) {
    int r = _write16(SPI_DISP_EXP_DEVICE, data);
    return r;
}

int spi_display_write16_buf(const uint16_t* buf, size_t len) {
    int r = _write16_buf(SPI_DISP_EXP_DEVICE, buf, len);
    return r;
}


void spi_expio_begin(void) {
    _begin(SPI_EXPANSION_SELECT);
}

void spi_expio_end(void) {
    _end(SPI_EXPANSION_SELECT);
}

int spi_expio_read_buf(uint8_t txval, uint8_t* dst, size_t len) {
    int r = _read_buf(SPI_DISP_EXP_DEVICE, SPI_HIGH_TXD_FOR_READ, dst, len);
    return r;
}

uint8_t spi_expansion_read8(uint8_t txval) {
    return (_read8(SPI_DISP_EXP_DEVICE, txval));
}

void spi_expio_select() {
    _device_select(SPI_EXPANSION_SELECT);
}

int spi_expio_write8(uint8_t data) {
    int r = _write8(SPI_DISP_EXP_DEVICE, data);
    return r;
}

int spi_expio_write8_buf(const uint8_t* buf, size_t len) {
    int r = _write8_buf(SPI_DISP_EXP_DEVICE, buf, len);
    return r;
}


void spi_none_select() {
    _device_select(SPI_NONE_SELECT);
}


void spi_touch_begin(void) {
    _begin(SPI_TOUCH_SELECT);
}

void spi_touch_end(void) {
    _end(SPI_TOUCH_SELECT);
}

int spi_touch_read_buf(uint8_t txval, uint8_t* dst, size_t len) {
    int r = _read_buf(SPI_TOUCH_DEVICE, txval, dst, len);
    return r;
}

uint8_t spi_touch_read8(uint8_t txval) {
    return (_read8(SPI_TOUCH_DEVICE, txval));
}

void spi_touch_select() {
    _device_select(SPI_TOUCH_SELECT);
}

int spi_touch_write8(uint8_t data) {
    int r = _write8(SPI_TOUCH_DEVICE, data);
    return r;
}

int spi_touch_write8_buf(const uint8_t* buf, size_t len) {
    int r = _write8_buf(SPI_TOUCH_DEVICE, buf, len);
    return r;
}


void spi_ops_module_init() {
    _device_select(SPI_NONE_SELECT);
    sem_init(&_dev_passkey.sem, 1, 1);
    _dev_passkey.device = SPI_NONE_SELECT;
    _dev_passkey.corenum = -1;
}

