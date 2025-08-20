/**
 * @brief Sensor Bank reading.
 * @ingroup sensbank
 *
 * Continuously reads the Sensor Bank and posts messages with state and changes.
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */

#include "sensbank.h"
#include "sensbank.pio.h"
#include "adc1015.h"

#include "system_defs.h"
#include "board.h"
#include "cmt/cmt.h"
#include "pio_uart/pio_uart.h"
#include "util/util.h"

#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"      // For 'gpio_set_inover'
#include "pico/time.h"

#include <stdio.h>
#include <stdlib.h>

// ############################################################################
// Value Definitions
// ############################################################################
//
#define ADC_PERIODIC_UPDATE 21      // Read and advance the ADC about 3x/sec

#define BINARY_SENSORS      0x1F    // Bits indicating which sensors are binary (0001 1111)
#define SENSBANK_ALL_OFF    0xFF

#define BAUD_SONAR_SENS     9600
#define BAUD_LIDAR_SENS     115200

#define LIDAR_FRAME_LEN     9       // LiDAR frame is (hex): 59 59 dl dh ssl ssh t rsv cs
#define LIDAR_SMPL_TO_MS    30      // LiDAR samples every 10ms. This give a little buffer.
#define SONAR_SMPLS_LEN     10      // Do two samples of 5 bytes to see if they are similar
#define SONAR_SMPL_TO_MS    120     // Sensor samples every 50ms. This give a little buffer.

// ############################################################################
// Local Types
// ############################################################################
//
typedef enum sens_rd_state_ {
    SRS_DISABLED = 0,
    SRS_BINARY_SENS = 1,
    SRS_SONAR0_SENS = 2,
    SRS_SONAR1_SENS = 3,
    SRS_LIDAR_SENS = 4,
} sens_rd_state_t;
#define SRS_NUMBER 5


// ############################################################################
// Function Declarations
// ############################################################################
//

static void _cancel_distance_rd(io_rw_32 pio_irqbits);
static void _distance_data_rdy(cmt_msg_t* msg);
static void _distance_read_to(cmt_msg_t* msg);
static void _out_sensor_addr();
static int _process_rcvd_lidar_data();
static int _process_rcvd_sonar_data();
static void _sens_state_update();
static void _uart_frame_error(cmt_msg_t* msg);


// ############################################################################
// Data
// ############################################################################
//
static bool _initialized;

static bool _adc_present;

static sensbank_dist_t _dist;           // Distance to obstacle from Sonar0, Sonar1, LiDAR
static int _dist_delta_accept;

static uint16_t _periodic_update;

static volatile uint8_t _samplerd[2];               // Store 2 samples to see if they are the same
static volatile bool _sens_lidar_ip;                // LiDAR sensor read is in progress
static volatile bool _sens_sonar_ip;                // Sonar sensor read is in progress

static volatile uint16_t _sens_lidar_dist[2];       // Two reads from LiDAR
static volatile uint16_t _sens_sonar_dist[2][2];    // Two reads from Sonar0 & Sonar1
static volatile sensbank_cah_t _sensdata;
// The sensor number being read
static uint8_t _sensor;
// True to signify the sensor number has changed.
static bool _sensor_changed;
// The sonar being read
static int _sonar;

// Current Sensor Read State
static sens_rd_state_t _srs;

// Distance PIO-SM and DMA Configurations
static volatile bool _canceling;
static int _dma_pio_rd;                         // DMA channel used to pull data from the PIO-SM
static dma_channel_config _dma_pio_rd_cfg;      // Keep the config so the channel is easy to re-run
static pio_sm_pocfg _psmcfg;                    // PIO SM Config for the PIO UART RX instance.

// LiDAR frame is 9 bytes. Sonar is 2 bytes and we read twice.
static volatile uint8_t _serial_data[max(LIDAR_FRAME_LEN, SONAR_SMPLS_LEN)];

// LiDAR Info
static volatile uint32_t _lidar_errors;
static volatile uint32_t _lidar_frame_errs;
static volatile uint32_t _lidar_format_errs;
static volatile uint32_t _lidar_chksum_errs;
static volatile uint16_t _lidar_strength;
static volatile uint8_t  _lidar_inttime;

// Sonar Info
static volatile uint32_t _sonar_errors;
static volatile uint32_t _sonar_frame_errs[2];
static volatile uint32_t _sonar_format_errs[2];

// ############################################################################
// Interrupt Handlers
// ############################################################################
//

/**
 * @brief The DMA has finished receiving the requested number of bytes from the PIO.
 */
void __isr sb_irq_dma_from_pio() {
    // Disable the SM
    pio_sm_set_enabled(_psmcfg.pio, _psmcfg.sm, false);
    // Clear the interrupt request.
    dma_irqn_acknowledge_channel(IRQn_SB_DMA_FROM_PIO, _dma_pio_rd);

    cmt_msg_t msg;
    cmt_exec_init(&msg, _distance_data_rdy);
    postDCSMsg(&msg);
}

/**
 * @brief IRQ Handler for RX Error (Framing).
 *
 * Posts an EXEC message with the IRQ flags and the handler set.
 */
void __isr sb_irq_pio_rx_err_handler() {
    cmt_msg_t msg;
    io_rw_32 pio_irqbits = _psmcfg.pio->irq;
    //
    // Stop the PIO-SM before clearing the IRQ
    //
    pio_sm_set_enabled(_psmcfg.pio, _psmcfg.sm, false);
    _psmcfg.pio->irq = 0xFF; // Writing '1' clears the IRQ Flag bit
    //
    // Initialize and post the message
    //
    cmt_exec_init(&msg, _uart_frame_error);
    msg.data.value32u = pio_irqbits;
    postDCSMsg(&msg);
}



// ############################################################################
// Message Handlers
// ############################################################################
//

static void _distance_data_rdy(cmt_msg_t *msg) {
    // Cancel our timeout message.
    scheduled_msg_cancel2(MSG_EXEC, _distance_read_to);
    // Put the input pin back to normal.
    gpio_set_inover(SENSOR_READ, GPIO_OVERRIDE_NORMAL);
    //
    // If the 2 distance values are within the acceptable delta update the distance with the average
    // What was being read?
    uint32_t now = now_ms();
    if (_sens_lidar_ip) {
        if (_process_rcvd_lidar_data() == 0) {
            int diff = ((int)_sens_lidar_dist[0] - (int)_sens_lidar_dist[1]);
            if (abs(diff) <= _dist_delta_accept) {
                uint16_t avg = (uint16_t)(((int)_sens_lidar_dist[0] + (int)_sens_lidar_dist[1]) / 2);
                _dist.lidar = avg;
                _dist.lidar_ts = now;
            }
        }
    }
    else if (_sens_sonar_ip) {
        if (_process_rcvd_sonar_data() == 0) {
            int diff = ((int)_sens_sonar_dist[_sonar][0] - (int)_sens_sonar_dist[_sonar][1]);
            if (abs(diff) <= _dist_delta_accept) {
                uint16_t avg = (uint16_t)(((int)_sens_sonar_dist[_sonar][0] + (int)_sens_sonar_dist[_sonar][1]) / 2);
                uint16_t acm = (uint16_t)roundf(cmFromIn((float)avg));
                if (_sonar == 0) {
                    _dist.sonar0 = acm;
                    _dist.sonar0_ts = now;
                }
                else {
                    _dist.sonar1 = acm;
                    _dist.sonar1_ts = now;
                }
            }
        }
    }
    _sens_lidar_ip = false;
    _sens_sonar_ip = false;
    _sens_state_update();
}

static void _uart_frame_error(cmt_msg_t *msg) {
    // The UART got a framing error. The irq handler disabled the SM.
    // Cancel the operation and move to the next state.
    io_rw_32 pio_irqbits = (io_rw_32)msg->data.value32u;
    // disable the system and DMA channel IRQ
    irq_set_enabled(SYSIRQ_SB_DMA_FROM_PIO, false);
    dma_irqn_set_channel_enabled(IRQn_SB_DMA_FROM_PIO, _dma_pio_rd, false);

    if (_sens_lidar_ip) {
        _lidar_frame_errs++;
        _lidar_errors++;
    }
    else if (_sens_sonar_ip) {
        _sonar_frame_errs[_sonar]++;
        _sonar_errors++;
    }
    _cancel_distance_rd(pio_irqbits);
}

// ############################################################################
// Internal Functions
// ############################################################################
//

static void _cancel_distance_rd(io_rw_32 pio_irqbits) {
    if (_canceling) {
        return;
    }
    _canceling = true;

    // Stop the PIO State Machine
    pio_sm_set_enabled(_psmcfg.pio, _psmcfg.sm, false);
    irq_set_enabled(PIO_SENSBANK_IRQ_ERR, false);
    pio_set_irqn_source_enabled(_psmcfg.pio, PIO_SENSBANK_IRQ_ERR_IDX, PIO_INTR_SM0_LSB, false);

    // Cancel the DMA handling the received data.
    //  Due to errata RP2350-E5(see the RP2350 datasheet for further detail),
    //  it is necessary to clear the enable bit of the channel being aborted,
    //  and any chained channels, prior to the abort to prevent (re)triggering.
    //
    // disable the system and DMA channel IRQ
    irq_set_enabled(SYSIRQ_SB_DMA_FROM_PIO, false);
    dma_irqn_set_channel_enabled(IRQn_SB_DMA_FROM_PIO, _dma_pio_rd, false);

    // See where in the received message the error occurred
    io_rw_32 dma_wr_addr = dma_channel_hw_addr(_dma_pio_rd)->write_addr;

    // Abort the channel
    dma_channel_abort(_dma_pio_rd);
    // Read the Abort register until 0
    // (this isn't done in the SDK, but the datasheet says it is needed)
    while (dma_hw->abort) tight_loop_contents();
    // clear any spurious IRQ (if there was one)
    dma_irqn_acknowledge_channel(IRQn_SB_DMA_FROM_PIO, _dma_pio_rd);

    // Clear out the RXFIFO.
    pio_sm_clear_fifos(_psmcfg.pio, _psmcfg.sm);

    // Put the input pin back to normal.
    gpio_set_inover(SENSOR_READ, GPIO_OVERRIDE_NORMAL);

    // Cancel our timeout message.
    int32_t to_r = scheduled_msg_cancel2(MSG_EXEC, _distance_read_to);
    if (to_r > 0) {
        //printf("Sensbank Cancel Distance Read. PIOIRQ: %08X Transferred: %d  TORemaining: %d\n", pio_irqbits, (int)((void*)dma_wr_addr - (void*)_serial_data), to_r);
        UNUSED(pio_irqbits);
        UNUSED(dma_wr_addr);
    }

    _sens_lidar_ip = false;
    _sens_sonar_ip = false;
    _sens_state_update();
    _canceling = false;
}

#define LIDAR_PKT_HDR_BYTE 0x59
#define LIDAR_HDR1_NDX      0
#define LIDAR_HDR2_NDX      1
#define LIDAR_DISTL_NDX     2
#define LIDAR_DISTH_NDX     3
#define LIDAR_SIG_STR_L_NDX 4
#define LIDAR_SIG_STR_H_NDX 5
#define LIDAR_INT_TIM_NDX   6
#define LIDAR_RESRVD_NDX    7
#define LIDAR_CHKSUM_NDX    8
#define LIDAR_BYTES_IN_SUM  8
/** Check the Serial Data for a valid LiDAR Packet and store the value if valid. */
static int _process_rcvd_lidar_data() {
    // Check the Header
    if ((_serial_data[LIDAR_HDR1_NDX] != LIDAR_PKT_HDR_BYTE) || (_serial_data[LIDAR_HDR2_NDX] != LIDAR_PKT_HDR_BYTE)) {
        _lidar_format_errs++;
        _lidar_errors++;
        return (1);
    }
    // Check the Sum
    uint16_t sum = 0;
    for (int i = 0; i < LIDAR_BYTES_IN_SUM; i++) {
        sum += _serial_data[i];
    }
    if (lowByte(sum) != _serial_data[LIDAR_CHKSUM_NDX]) {
        _lidar_chksum_errs++;
        _lidar_errors++;
        return (2);
    }
    // Frame looks good. Get the Distance, Strength, and Integration Time
    uint16_t dist = wordFromBytes(_serial_data[LIDAR_DISTH_NDX], _serial_data[LIDAR_DISTL_NDX]);
    uint16_t strgth = wordFromBytes(_serial_data[LIDAR_SIG_STR_H_NDX], _serial_data[LIDAR_SIG_STR_L_NDX]);
    uint8_t inttime = _serial_data[LIDAR_INT_TIM_NDX];
    _lidar_strength = strgth;
    _lidar_inttime = inttime;
    _sens_lidar_dist[(_periodic_update & 0x0001)] = dist;
    return (0);
}

#define SONAR_FRAME_LEADER ('R')
#define SONAR_FRAME_TRAILER ('\r')
#define SONAR_FRAME_LEN 5
/** Check the Serial Data for a pair of valid Sonar measurements. */
static int _process_rcvd_sonar_data() {
    if (_serial_data[0] != SONAR_FRAME_LEADER
        || _serial_data[(SONAR_FRAME_LEN - 1)] != SONAR_FRAME_TRAILER
        || _serial_data[(0 + SONAR_FRAME_LEN)] != SONAR_FRAME_LEADER
        || _serial_data[(SONAR_FRAME_LEN + (SONAR_FRAME_LEN - 1))] != SONAR_FRAME_TRAILER) {
            _sonar_format_errs[_sonar]++;
            _sonar_errors++;
            return (1);
    }
    uint16_t dist1 = (uint16_t)(((uint16_t)(_serial_data[1] & 0x0F) * 100) + ((uint16_t)(_serial_data[2] & 0x0F) * 10) + ((uint16_t)(_serial_data[3] & 0x0F)));
    uint16_t dist2 = (uint16_t)(((uint16_t)(_serial_data[6] & 0x0F) * 100) + ((uint16_t)(_serial_data[7] & 0x0F) * 10) + ((uint16_t)(_serial_data[8] & 0x0F)));
    _sens_sonar_dist[_sonar][0] = dist1;
    _sens_sonar_dist[_sonar][1] = dist2;
    return (0);
}

static void _out_sensor_addr() {
    uint32_t sa = (_sensor << SENSOR_SEL_A0);
    gpio_put_masked(SENSOR_SEL_MASK, sa);
}

static void _process_lidar_start() {
    if (_sens_lidar_ip) {
        board_panic("_process_lidar_start _sens_lidar_ip already true!");
    }
    _sens_lidar_ip = true;
    //
    // Configure PIO RD DMA channel to read from the RXFIFO MSB and write to the Sensor Enqueue buffer.
    dma_channel_configure(_dma_pio_rd, &_dma_pio_rd_cfg,
        _serial_data,                                   // Destination
        (uint8_t*)&_psmcfg.pio->rxf[PIO_SENSBANK_SM] + 3, // PIO-SM RX FIFO to read from (+3 to read the MSB)
        LIDAR_FRAME_LEN,
        false);                                         // Don't start yet

    // Reset the PIO-SM so that it is waiting for the idle period.
    pio_sm_set_enabled(_psmcfg.pio, _psmcfg.sm, false);
    piosm_reset(_psmcfg);
    pio_uart_baud_set(&_psmcfg, BAUD_LIDAR_SENS);
    //
    // Now start the DMA and PIO-SM along with a timeout
    // Tell the DMA to raise its IRQ when the channel finishes a block
    dma_irqn_set_channel_enabled(IRQn_SB_DMA_FROM_PIO, _dma_pio_rd, true);
    pio_set_irqn_source_enabled(_psmcfg.pio, PIO_SENSBANK_IRQ_ERR_IDX, PIO_INTR_SM0_LSB, true); // Interrupt on IRQ-Bit0 set
    // Enable the system interrupts
    irq_set_enabled(SYSIRQ_SB_DMA_FROM_PIO, true);
    irq_set_enabled(PIO_SENSBANK_IRQ_ERR, true);

    // Schedule a timeout message
    if (scheduled_msg_exists2(MSG_EXEC, _distance_read_to)) {
        board_panic("LIDAR Timeout already scheduled");
    }
    cmt_msg_t msg;
    cmt_exec_init(&msg, _distance_read_to);
    schedule_msg_in_ms(LIDAR_SMPL_TO_MS, &msg);

    // Start the DMA and PIO-SM
    dma_channel_set_write_addr(_dma_pio_rd, _serial_data, true);
    pio_sm_set_enabled(_psmcfg.pio, _psmcfg.sm, true);
}

static void _process_sonar_start(int inst) {
    _sens_sonar_ip = true;
    _sonar = inst;
    // Invert the input pin, as the sonar sensor uses an inverted signal.
    // This directly manipulates the hardware register for input override
    gpio_set_inover(SENSOR_READ, GPIO_OVERRIDE_INVERT);
    //
    // Configure PIO RD DMA channel to read from the RXFIFO MSB and write to the Sensor Enqueue buffer.
    dma_channel_configure(_dma_pio_rd, &_dma_pio_rd_cfg,
        _serial_data,                                   // Destination
        (uint8_t*)&_psmcfg.pio->rxf[PIO_SENSBANK_SM] + 3, // PIO-SM RX FIFO to read from (+3 to read the MSB)
        SONAR_SMPLS_LEN,
        false);                                         // Don't start yet
    //
    // Now start the DMA and PIO-SM along with a timeout
    cmt_msg_t msg;
    cmt_exec_init(&msg, _distance_read_to);
    schedule_msg_in_ms(SONAR_SMPL_TO_MS, &msg);
    // Enable the interrupts.
    dma_irqn_set_channel_enabled(IRQn_SB_DMA_FROM_PIO, _dma_pio_rd, true);
    irq_set_enabled(SYSIRQ_SB_DMA_FROM_PIO, true);
    // Reset the PIO-SM so that it is waiting for the idle period.
    piosm_reset(_psmcfg);
    pio_uart_baud_set(&_psmcfg, BAUD_SONAR_SENS);
    // Start the DMA and PIO-SM
    dma_channel_set_write_addr(_dma_pio_rd, _serial_data, true);
    pio_sm_set_enabled(_psmcfg.pio, _psmcfg.sm, true);
}

static void _sens_state_update() {
    _sensor_changed = true;
    // Update the sensor number and check for next state.
    _sensor++;
    if (_sensor > 7) {
        _sensor = 0;
    }
    switch (_sensor) {
        case 5:
            _srs = SRS_SONAR0_SENS;
            break;
        case 6:
            _srs = SRS_SONAR1_SENS;
            break;
        case 7:
            _srs = SRS_LIDAR_SENS;
            break;
        default:
            _srs = SRS_BINARY_SENS;
            break;
    }
    _out_sensor_addr();
}

/**
 * @brief The same value was read from the sensor twice.
 *
 * Move the current value to the previous, set the current, and post a message
 * if the values have changed.
 *
 */
static void _sensrd_valid() {
    _sensdata.prev_bits = _sensdata.bits;
    _sensdata.bits = _samplerd[0];  // Sensor read checked that 0 & 1 are the same.
    // Post sensor changed if appropriate
    if (_sensdata.bits != _sensdata.prev_bits) {
        cmt_msg_t msg;
        cmt_msg_init(&msg, MSG_SENSBANK_CHG);
        msg.data.sensbank_chg = _sensdata;
        postHWRTMsg(&msg);
        postDCSMsg(&msg);
    }
}

static void _distance_read_to(cmt_msg_t *msg) {
    // The distance (serial) read has timed out.
    // Cancel the DMA, stop the PIO, and move to the next Read State.
    _cancel_distance_rd(0);
}

// ############################################################################
// Public Functions
// ############################################################################
//

uint16_t sensbank_dist_delta_accept() {
    return _dist_delta_accept;
}

void sensbank_dist_delta_accept_set(uint16_t delta) {
    _dist_delta_accept = delta;
}


sensbank_dist_t sensbank_dist_get(void) {
    return _dist;
}

void sensbank_enable(bool enable) {
    bool was_enabled = (_srs != SRS_DISABLED);
    if (was_enabled && !enable) {
        // Set SA0-SA2 to 0 (sensor board doesn't enable anything for S0)
        _srs = SRS_DISABLED;
        gpio_clr_mask(SENSOR_SEL_MASK);
        _sensor = 0;
        return;
    }
    if (enable && !was_enabled) {
        _sensor_changed = true;
        _sensor = 0;
        _srs = SRS_BINARY_SENS;
        _out_sensor_addr();
    }
}

sensbank_cah_t sensbank_get(void) {
    return _sensdata;
}

void sensbank_update(void) {
    if (!_initialized) {
        return;
    }

    _periodic_update++;
    // ADC Read
    if (_adc_present && (_periodic_update % ADC_PERIODIC_UPDATE == 0)) {
        adc1015_update();
    }

    // Sensor Bank Read
    switch (_srs) {
        case SRS_DISABLED:
            // Nothing to do when disabled
            break;
        case SRS_BINARY_SENS:
            // 1st sensor read on even counts, 2nd on odd. If both are equal we store.
            if ((_periodic_update & 0x0001) == 0) {
                // Even update
                // Read the sensor (address was output when the sensor changed)
                bool v = gpio_get(SENSOR_READ);
                bitWrite(_samplerd[0], _sensor, v);
                _sensor_changed = false;
            }
            else {
                // Odd update
                if (_sensor_changed) {
                    // Don't do 'first read' on an odd update.
                    return;
                }
                // Read the sensor
                bool v = gpio_get(SENSOR_READ);
                bitWrite(_samplerd[1], _sensor, v);
                if (_samplerd[0] == _samplerd[1]) {
                    // Same value for both reads. Process the value.
                    _sensrd_valid();
                }
                _sens_state_update();
            }
            break;
        case SRS_SONAR0_SENS:
            if (!_sens_sonar_ip) {
                _process_sonar_start(0);
            }
            break;
        case SRS_SONAR1_SENS:
            if (!_sens_sonar_ip) {
                _process_sonar_start(1);
            }
            break;
        case SRS_LIDAR_SENS:
            if (!_sens_lidar_ip) {
                _process_lidar_start();
            }
            break;
        default:
            board_panic("!!! sensbank: Sense Read State (_srs) isn't a valid state: %d !!!\n", _srs);
            break;
    }
}

// ############################################################################
// Initialization and Maintainence Functions
// ############################################################################
//

void sensbank_start(void) {
    if (!_initialized) {
        return;
    }

    // If the ADC is connected, start reading the ADC
    uint8_t adc_addr = SB_ADC_ADDR1; // The ADC is at address 0x48 or 0x49
    if (i2c_device_present(adc_addr)) {
        _adc_present = true;
    }
    else if (i2c_device_present(++adc_addr)) {
        _adc_present = true;
    }
    if (_adc_present) {
        adc1015_module_init(I2C_EXTERN, adc_addr); // Init the ADC
        adc1015_start();
    }

    _sensor_changed = true;
    _sensor = 0;
    _srs = SRS_BINARY_SENS;
}


void sensbank_module_init(void) {
    if (_initialized) {
        board_panic("sensbank_module_init already called");
    }
    _initialized = true;

    _adc_present = false;

    _sensdata.prev_bits = _sensdata.bits = _samplerd[1] = _samplerd[0] = SENSBANK_ALL_OFF;
    _sensor = 0;
    _srs = SRS_BINARY_SENS;
    _out_sensor_addr();
    _dist_delta_accept = 10; // Consider 10cm delta acceptable. It can be set if needed.

    // Initialize a PIO UART RX to read the sonar and lidar sensors
    _psmcfg = pio_uart_rx_init(PIO_SENSBANK_BLOCK, PIO_SENSBANK_SM, SENSOR_READ, BAUD_SONAR_SENS);

    // Get a DMA channel that will take data from the PIO-SM RXFIFO,
    _dma_pio_rd = dma_claim_unused_channel(true);
    // Configure the processor to run DMA from PIO routine when DMA IRQ0 is asserted.
    irq_set_exclusive_handler(SYSIRQ_SB_DMA_FROM_PIO, sb_irq_dma_from_pio);
    //
    // Init the PIO RD DMA to read from the PIO when there is data ready
    _dma_pio_rd_cfg = dma_channel_get_default_config(_dma_pio_rd); //Get configurations for the RC channel
    channel_config_set_transfer_data_size(&_dma_pio_rd_cfg, DMA_SIZE_8); //Set RC PIO channel data transfer size to 8 bits
    channel_config_set_read_increment(&_dma_pio_rd_cfg, false); // Read increment to false (read from PIO)
    channel_config_set_write_increment(&_dma_pio_rd_cfg, true); // Write increment to true (advance through buffer)
    channel_config_set_dreq(&_dma_pio_rd_cfg, PIO_SENSBANK_DREQ); //Set the transfer request signal to the PIO-SM rx-fifo not empty.
    //
    irq_set_exclusive_handler(PIO_SENSBANK_IRQ_ERR, sb_irq_pio_rx_err_handler); // Set the IRQ handler
    irq_set_enabled(PIO_SENSBANK_IRQ_ERR, false); // Disable the IRQ for now
}

/*
static void pio_irq_func(void) {
    // IRQ called when the pio fifo is not empty, i.e. there is a sensbank
    // value available. This occurs ~80 per second (12-13ms).
    while (!pio_sm_is_rx_fifo_empty(PIO_SENSBANK_BLOCK, PIO_SENSBANK_SM)) {
        uint32_t dw = pio_sm_get(PIO_SENSBANK_BLOCK, PIO_SENSBANK_SM);
        // We want the value to be the same twice, to debounce switch changes.
        uint8_t d = dw & 0x000000FF;
        _samplerd[_sampleindx++] = d;
        if (_sampleindx != SAMPLES_NEEDED_) {
            continue;
        }
        else {
            _sampleindx = 0;
            if (_samplerd[0] != _samplerd[1]) {
                continue;
            }
        }
        // There were two consecutive reads the same. Check the value.
        _sensdata.prev_bits = _sensdata.bits;
        if (d != _sensdata.bits) {
            _sensdata.bits = d;
            // Some bits have changed, post a message
            cmt_msg_t msg;
            cmt_msg_init(&msg, MSG_SENSBANK_CHG);
            msg.data.sensbank_chg.prev_bits = _sensdata.prev_bits;
            msg.data.sensbank_chg.bits = _sensdata.bits;
            postHWRTMsg(&msg);
            postDCSMsgDiscardable(&msg); // DCS is for status only
        }
    }
}



static void _sensbank_program_init(PIO pio, uint sm, uint offset, uint opin, uint ipin) {
    // Set the 3 o-pin directions to output at the PIO
    pio_sm_set_consecutive_pindirs(pio, sm, opin, 3, true);
    // Set the i-pin direction to input at the PIO
    pio_sm_set_consecutive_pindirs(pio, sm, ipin, 1, false);
    // Connect these GPIOs to this PIO block
    pio_gpio_init(pio, opin);
    pio_gpio_init(pio, opin + 1);
    pio_gpio_init(pio, opin + 2);
    pio_gpio_init(pio, ipin);

    pio_sm_config c = sensbank_program_get_default_config(offset);
    // Set the OUT base pin to the provided `opin` parameter. This is the A0 bit,
    // and the next 2 numbered GPIO are A1 and A2.
    sm_config_set_out_pins(&c, opin, 3);
    // Set the IN base pin to the provided `ipin` parameter. This is the sensor input.
    sm_config_set_in_pins(&c, ipin);
    // Shift 8 bits left with AutoPush. Shifting left puts the bit read from address 7 in bit 7.
    sm_config_set_in_shift(
        &c,
        false, // Shift-to-right = false (i.e. shift to left)
        false, // Auto-push not enabled
        8      // Auto-push threshold = 8
    );
    // Data is input only, so join the TX FIFO to the RX FIFO.
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);

    // Set the clock divider to sample the 8 inputs about 80 times a second.
    // (each bit sample takes 4 clock cycles)
    float div = clock_get_hz(clk_sys) / (80 * (4 * 8));
    sm_config_set_clkdiv(&c, div);

    // Load our configuration, but don't start it
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, false);
}

    // Load the PIO program
    int offset;
    offset = pio_add_program(PIO_SENSBANK_BLOCK, &sensbank_program);
    if (offset < 0) {
        board_panic("sensbank_module_init - Unable to load PIO program");
    }
    // Enable interrupt
    irq_set_exclusive_handler(PIO_SENSBANK_IRQ_ERR, pio_irq_func); // Set the IRQ handler
    irq_set_enabled(PIO_SENSBANK_IRQ_ERR, false); // Disable the IRQ for now

    _sensbank_program_init(PIO_SENSBANK_BLOCK, PIO_SENSBANK_SM, offset, SENSOR_SEL_A0, SENSOR_READ);


    // Set pio to tell us when the FIFO is NOT empty
    pio_set_irqn_source_enabled(PIO_SENSBANK_BLOCK, PIO_SENSBANK_IRQ_ERR_IDX, pio_get_rx_fifo_not_empty_interrupt_source(PIO_SENSBANK_SM), true);
    // Enable the interrupt and start the PIO state machine
    irq_set_enabled(PIO_SENSBANK_IRQ_ERR, true);
    pio_sm_set_enabled(PIO_SENSBANK_BLOCK, PIO_SENSBANK_SM, true);
*/
