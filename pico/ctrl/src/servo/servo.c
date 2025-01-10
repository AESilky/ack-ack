/**
 * @brief Serial Bus Servo control.
 * @ingroup servo
 *
 * Controls a HiWonder Serial Bus servo (using its ID).
 *
 * The documentation for the servo serial protocol is found here:
 * https://drive.google.com/drive/folders/1yaZ8iRYgWncdHPopioo7OYuHoGhrMxCi
 *  - also in the 'component_docs' directory
 *
 * Most of the servos have a position range of 0-240° (±120°) with values of 0-1000 (0.24°).
 * The HX-35HM servo has a position range of a full 360° with values of 0-1500 (0.24°).
 *
 * The format of a Bus Servo command and a Bus Servo Status message is:
 * +------+------+------+------+------+------+------+------+-------|
 * |   0  |   1  |   2  |   3  |   4  |  5     ...    LEN+2| LEN+3 |
 * +------+------+------+------+------+------+------+------+-------|
 * | 0x55 | 0x55 |  ID  |  LEN |  CMD | pd1    ...    pdN  |  CSUM |
 * +------+------+------+------+------+------+------+------+-------|
 * LEN = The length of the entire packet minus the 2 header bytes and the ID.
 *
 * Copyright 2023-24 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#include "servo.h"

#include "board.h"
#include "cmt/cmt.h"
#include "system_defs.h"

#include <string.h>

//Macro function  get lower 8 bits of A
#define GET_LOW_BYTE(A) (uint8_t)((A))
//Macro function  get higher 8 bits of A
#define GET_HIGH_BYTE(A) (uint8_t)((A) >> 8)
//Macro Function  put A as higher 8 bits   B as lower 8 bits   which amalgamated into 16 bits integer
#define BYTES_TO_WORD(A, B) ((((uint16_t)(A)) << 8) | (uint8_t)(B))

#define BS_BAUDRATE         115200
#define BS_RXD_TIMEOUT_MS   20  // Timeout if no data received in 20ms (typ=600µs)
#define INPUT_BUF_SIZE_     16  // Needs to be a power of 2

// ############################################################################
// Bus Servo Commands
// ############################################################################
//
#define BS_FRAME_HEADER                 0x55
#define BS_MOVE_TIME_WRITE               1
#define BS_MOVE_TIME_READ                2
#define BS_MOVE_TIME_WAIT_WRITE          7
#define BS_MOVE_TIME_WAIT_READ           8
#define BS_MOVE_START                   11
#define BS_MOVE_STOP                    12
#define BS_ID_WRITE                     13
#define BS_ID_READ                      14
#define BS_ANGLE_OFFSET_ADJUST          17
#define BS_ANGLE_OFFSET_WRITE           18
#define BS_ANGLE_OFFSET_READ            19
#define BS_ANGLE_LIMIT_WRITE            20
#define BS_ANGLE_LIMIT_READ             21
#define BS_VIN_LIMIT_WRITE              22
#define BS_VIN_LIMIT_READ               23
#define BS_TEMP_MAX_LIMIT_WRITE         24
#define BS_TEMP_MAX_LIMIT_READ          25
#define BS_TEMP_READ                    26
#define BS_VIN_READ                     27
#define BS_POS_READ                     28
#define BS_SERVO_OR_MOTOR_MODE_WRITE    29
#define BS_SERVO_OR_MOTOR_MODE_READ     30
#define BS_LOAD_OR_UNLOAD_WRITE         31
#define BS_LOAD_OR_UNLOAD_READ          32
#define BS_LED_CTRL_WRITE               33
#define BS_LED_CTRL_READ                34
#define BS_LED_ERROR_WRITE              35
#define BS_LED_ERROR_READ               36

#define BSS_CHKSUM_OFF 3  // 3 more than the data length


// ############################################################################
// Function Declarations
// ############################################################################
//
typedef void (*servo_vv_func)(void);
inline static bool _rxd_input_available(void);
static void _rxd_discard();
static void _rxd_stash(uint8_t ch);
static void _rxd_status_asm_cont();
static void _rxd_status_clr(servo_t* rxs);
static bs_rx_status_t* _store_bs_status(bs_rx_status_t* bs_status);
static void _uart_drain();
static void _uart_intr_disable();
static void _uart_intr_enable();
static void _write_bs(const uint8_t* buf);

// ############################################################################
// Data
// ############################################################################
//
auto_init_mutex(tx_mutex);

static char _input_buf[INPUT_BUF_SIZE_];
static bool _input_buf_overflow = false;
static uint16_t _input_buf_in = 0;
static uint16_t _input_buf_out = 0;

static servo_t *_servo_in_proc;

static cmt_msg_t _msg_rxd_to;
static servo_vv_func _rxd_handler;

static bool _tx_enabled;

// ############################################################################
// Interrupt Service Routines
// ############################################################################
//
void _on_uart_rx() {
    while (uart_is_readable(SERVO_CTRL_UART)) {
        uint8_t c = uart_getc(SERVO_CTRL_UART);
        _rxd_stash(c);
    }
}


// ############################################################################
// Local Routines
// ############################################################################
//

static uint8_t _gen_checksum(uint8_t buf[]) {
    uint8_t i;
    uint16_t sum = 0;
    for (i = 2; i < buf[3] + 2; i++) {
        sum += buf[i];
    }
    i = GET_LOW_BYTE(sum);
    i = ~i;

    return i;
}

static void _post_servo_error_msg(servo_t *servo) {
    if (servo) {
        _rxd_status_clr(servo);
        // Indicate that we encountered an error with this servo
        uint8_t id = servo->id;
        cmt_msg_t msg;
        cmt_msg_init(&msg, MSG_SERVO_READ_ERROR);
        msg.data.servo_params.servo_id = id;
        postHWCtrlMsg(&msg);
        _servo_in_proc = SERVO_NONE;
    }
}

static void _rxd_clear() {
    _input_buf_in = _input_buf_out = 0;
    _input_buf_overflow = false;
}

static int _rxd_getc(void) {
    if (!_rxd_input_available()) {
        return (-1);
    }
    int c = (int)_input_buf[_input_buf_out];
    _input_buf_out = (_input_buf_out + 1) % INPUT_BUF_SIZE_;

    return (c);
}

inline static bool _rxd_input_available() {
    return (_input_buf_in != _input_buf_out);
}

static bool _rxd_overflowed() {
    bool retval = _input_buf_overflow;
    _input_buf_overflow = false;

    return (retval);
}

static void _rxd_stash(uint8_t ch) {
    // Called from an ISR - make it quick!
    //
    // See if we have room for it in the input buffer
    if (((_input_buf_in + 1) % INPUT_BUF_SIZE_) == _input_buf_out) {
        // No room. Just throw it away.
        _input_buf_overflow = true;
    }
    else {
        bool post_msg = false;
        // If this is the first character going into the buffer
        // post a message so others know that data is available.
        if (_input_buf_in == _input_buf_out) {
            post_msg = true;
        }
        // Store it, then continue reading
        _input_buf[_input_buf_in] = (char)ch;
        _input_buf_in = (_input_buf_in + 1) % INPUT_BUF_SIZE_;
        // Post a message?
        if (post_msg) {
            cmt_msg_t msg;
            cmt_msg_init(&msg, MSG_SERVO_DATA_RCVD);
            postHWCtrlMsg(&msg);
        }
    }
}

/**
 * @brief Begin a Status Read Operation if possible.
 * @ingroup servo
 * 
 * This checks to see if we are already waiting for an inbound message,
 * and if so, it returns `false`. If not, it enters into the 'tx_mutex'
 * and sets everything up to be ready to receive the response.
 * 
 * !!! Enters the 'tx_mutex' and doesn't exit it if successful !!!
 * 
 * @param servo The servo to collect the response into.
 * @return true Ready to receive a response.
 * @return false Already waiting for a response.
 */
static bool _rxd_status_asm_bgn(servo_t *servo) {
    // If we are waiting for a status data packet, indicate that we can't begin...
    if (servo_status_inbound_pending()) {
        return false;
    }
    // Enter into Mutex protected area!
    mutex_enter_blocking(&tx_mutex);

    // Clear any pending Status Read timeout message.
    servo_t* pending = _servo_in_proc;
    _servo_in_proc = SERVO_NONE;
    if (pending) {
        _rxd_status_clr(pending);
    }
    _rxd_status_clr(servo);
    servo->_rxstatus.pending = true;
    _servo_in_proc = servo;
    scheduled_msg_cancel(MSG_SERVO_DATA_RX_TO);
    _rxd_clear();
    _rxd_handler = _rxd_status_asm_cont;
    schedule_core0_msg_in_ms(BS_RXD_TIMEOUT_MS, &_msg_rxd_to);

    return (true);
}

static void _rxd_status_asm_cont() {
    // We have been notified that data has been received from the servos.
    if (!servo_status_inbound_pending()) {
        // We aren't expecting anything. Set our receive function
        // to discard and slurp up any inbound data.
        _uart_intr_disable();
        _rxd_handler = _rxd_discard;
        _rxd_clear();
        return;
    }
    int ch;
    while ((ch = _rxd_getc()) >= 0) {
        // Put the data into the status packet
        _servo_in_proc->_rxstatus.buf[_servo_in_proc->_rxstatus.data_off++] = ch;
        if (!_servo_in_proc->_rxstatus.frame_started) {
            // We are looking for the HEADER bytes
            if (ch == BS_FRAME_HEADER) {
                // Receiving 2 header bytes in a row signals the start of a frame.
                if (_servo_in_proc->_rxstatus.data_off == 2) {
                    _servo_in_proc->_rxstatus.frame_started = true;
                    scheduled_msg_cancel(MSG_SERVO_DATA_RX_TO);
                }
            }
            else {
                _servo_in_proc->_rxstatus.frame_started = false;
                _servo_in_proc->_rxstatus.data_off = 0;
            }
        }
        else {
            if (_servo_in_proc->_rxstatus.data_off == BSPKT_LEN + 1) {
                _servo_in_proc->_rxstatus.len = ch;
                if (ch < 3) {
                    // Something was wrong with this packet.
                    _rxd_status_clr(_servo_in_proc);
                    break;
                }
            }
            else if (_servo_in_proc->_rxstatus.data_off > BSPKT_LEN) {
                if (_servo_in_proc->_rxstatus.data_off > BSPKT_PAYLOAD_MAX_LEN) {
                    // Something was wrong with this packet.
                    _rxd_status_clr(_servo_in_proc);
                    break;
                }
                if (_servo_in_proc->_rxstatus.data_off == (_servo_in_proc->_rxstatus.len + BSS_CHKSUM_OFF)) {
                    // Compare the checksums
                    if (_gen_checksum(_servo_in_proc->_rxstatus.buf) == _servo_in_proc->_rxstatus.buf[_servo_in_proc->_rxstatus.data_off - 1]) {
                        // All is good.
                        _rxd_handler = _rxd_discard;  // Discard any additional received data
                        _servo_in_proc->_rxstatus.pending = false;
                        // Post a message with the status
                        cmt_msg_t msg;
                        cmt_msg_init(&msg, MSG_SERVO_STATUS_RCVD);
                        msg.data.servo_params.servo_id = _servo_in_proc->id;
                        postHWCtrlMsg(&msg);
                    }
                    else {
                        _post_servo_error_msg(_servo_in_proc);
                    }
                    break;
                }
            }
        }
    }
    _servo_in_proc = SERVO_NONE;
    _uart_intr_disable();
    _rxd_handler = _rxd_discard;
    _rxd_clear();
    mutex_exit(&tx_mutex);  // Exit out of the 'tx_mutex' protected area!
}

/**
 * @brief Handle a timeout waiting for a servo response.
 * @ingroup servo
 *
 * @param msg Message
 */
static void _rxd_status_asm_to(cmt_msg_t *msg) { 
    servo_t* servo = _servo_in_proc;
    _post_servo_error_msg(servo);
    mutex_exit(&tx_mutex);  // Exit out of the 'tx_mutex' protected area!
}

static void _rxd_discard() {
    // Just clear out any received data.
    _rxd_clear();
}

static void _rxd_status_clr(servo_t *servo) {
    if (servo) {
        servo->_rxstatus.frame_started = false;
        servo->_rxstatus.pending = true;
        servo->_rxstatus.data_off = 0;  // 0-1=Hdr, 2=ID, 3=Len, 4=Cmd, 5+=data,
        servo->_rxstatus.len = 0;
    }
}

/**
 * @brief Send an 'action' type of command (doesn't return any information).
 * @ingroup servo
 *
 * The servos use a single line to transmit and receive using half-duplex
 * communication. Because of this, if we sent a 'read status' type of command
 * and we are waiting for a response, we can't send out a command, as we
 * would *step on* the response coming back.
 *
 * This function checks to see if we are waiting for a response, and if so
 * it returns false and doesn't send the command. In this case, the command
 * needs to be retried after waiting.
 *
 * @param buf A buffer containing a read status command packet.
 * @return true The buffer was able to be sent
 * @return false The buffer could not be sent
 */
static bool _send_action_cmd(uint8_t *buf) {
    if (servo_status_inbound_pending()) {
        return false;
    }
    mutex_enter_blocking(&tx_mutex);
    _write_bs(buf);
    mutex_exit(&tx_mutex);

    return true;
}

/**
 * @brief Send a 'read status' type of command packet to the servo bus.
 * @ingroup servo
 *
 * The servos use a single line to transmit and receive using half-duplex
 * communication. Because of this, once we know that we can send the
 * command we enter into the 'tx_mutex'. The 'tx_mutex' blocks other use
 * of the tx/rx data line. The 'tx_mutex' is held until we receive the
 * response, or timeout.
 *
 * @param servo The servo the command is for (response will be read into)
 * @param buf A buffer containing a read status command packet.
 * @return true The buffer was able to be sent
 * @return false The buffer could not be sent
 */
static bool _send_rd_status_cmd(servo_t *servo, uint8_t *buf) {
    if (_rxd_status_asm_bgn(servo)) { // Enters the 'tx_mutex' if successful.
        _write_bs(buf);
        _uart_intr_enable();
        return true;
    }
    return false;
}


/**
 * @brief Disable transmitting to the servo bus.
 * @ingroup servo
 *
 * The servo bus communication is half-duplex, therefore the transmit driver
 * must be enabled to send data to the servo bus. Once a command is sent the
 * transmit driver must be disabled to allow the servo to transmit a reply.
 */
static void _tx_disable(void) {
    gpio_put(SERVO_CTRL_TX_EN_GPIO, SERVO_CTRL_TX_DIS);     // Bus-Servo TX Disabled
    _tx_enabled = false;
}

/**
 * @brief Enable transmitting to the servo bus.
 * @ingroup servo
 *
 * The servo bus communication is half-duplex, therefore the transmit driver
 * must be enabled to send data to the servo bus. Once a command is sent the
 * transmit driver must be disabled to allow the servo to transmit a reply.
 */
static void _tx_enable(void) {
    gpio_put(SERVO_CTRL_TX_EN_GPIO, SERVO_CTRL_TX_EN);     // Bus-Servo TX Enabled
    _tx_enabled = true;
}

/**
 * @brief Read all available data from the UART and discard it.
 * @ingroup servo
 */
static void _uart_drain() {
    // Clear any 'junk' out of the UART.
    while (uart_is_readable(SERVO_CTRL_UART)) {
        uart_getc(SERVO_CTRL_UART);
    }
}

/**
 * @brief Disable UART receive interrupts.
 */
static void _uart_intr_disable() {
    irq_set_enabled(SERVO_CTRL_IRQ, false);
    // Disable the UART from sendint interrupts
    uart_set_irq_enables(SERVO_CTRL_UART, false, false);
}

/**
 * @brief Enable UART receive interrupts. This first drains the UART RX FIFO.
 */
static void _uart_intr_enable() {
    _uart_drain(); // Remove all data before enabling interrupts
    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(SERVO_CTRL_UART, true, false);
    // Main enable
    irq_set_enabled(SERVO_CTRL_IRQ, true);
}

static void _write_bs(const uint8_t* buf) {
    size_t len = (size_t)(buf[BSPKT_LEN] + 3);
    _uart_intr_disable();
    _tx_enable();  // Enable the TX output to the bus.
    uart_write_blocking(SERVO_CTRL_UART, buf, len);
    uart_tx_wait_blocking(SERVO_CTRL_UART);  // Wait for all data to be sent.
    _tx_disable();  // Disable the TX output so that we can read.
}


// ############################################################################
// Message Handlers
// ############################################################################
//

static void _handle_servo_rxd(cmt_msg_t* msg) {
    // Data has been received from the servos
    _rxd_handler();
}

const msg_handler_entry_t servo_rxd_handler_entry = { MSG_SERVO_DATA_RCVD, _handle_servo_rxd };

// ############################################################################
// Public Methods
// ############################################################################
//

bool servo_load(servo_t* servo) {
    uint8_t buf[7];
    buf[0] = buf[1] = BS_FRAME_HEADER;
    buf[2] = servo->id;
    buf[3] = 4;
    buf[4] = BS_LOAD_OR_UNLOAD_WRITE;
    buf[5] = 1;
    buf[6] = _gen_checksum(buf);
    return (_send_action_cmd(buf));
}

bool servo_move(servo_t *servo, int16_t position, uint16_t time) {
    uint8_t buf[10];
    if (position < 0)
        position = 0;
    if (position > 1000)
        position = 1000;
    buf[0] = buf[1] = BS_FRAME_HEADER;
    buf[2] = servo->id;
    buf[3] = 7;
    buf[4] = BS_MOVE_TIME_WRITE;
    buf[5] = GET_LOW_BYTE(position);
    buf[6] = GET_HIGH_BYTE(position);
    buf[7] = GET_LOW_BYTE(time);
    buf[8] = GET_HIGH_BYTE(time);
    buf[9] = _gen_checksum(buf);
    return (_send_action_cmd(buf));
}

int16_t servo_position(servo_t *servo) {
    // Make sure this is a position packet.
    if (servo->_rxstatus.buf[BSPKT_CMD] != BS_POS_READ) {
        return (-1);
    }
    return ((int16_t)BYTES_TO_WORD(servo->_rxstatus.buf[BSPKT_DATA + 2], servo->_rxstatus.buf[BSPKT_DATA + 1]));
}

bool servo_position_read(servo_t *servo) {
    uint8_t buf[6];

    buf[0] = buf[1] = BS_FRAME_HEADER;
    buf[2] = servo->id;
    buf[3] = 3;
    buf[4] = BS_POS_READ;
    buf[5] = _gen_checksum(buf);

    return (_send_rd_status_cmd(servo, buf));
}

bool servo_run(servo_t *servo, int16_t speed) {
    return (servo_set_mode(servo, BS_MOTOR_MODE, speed));
}

bool servo_set_id(uint8_t oldID, uint8_t newID) {
    uint8_t buf[7];
    buf[0] = buf[1] = BS_FRAME_HEADER;
    buf[2] = oldID;
    buf[3] = 4;
    buf[4] = BS_ID_WRITE;
    buf[5] = newID;
    buf[6] = _gen_checksum(buf);
    return (_send_action_cmd(buf));
}

bool servo_set_mode(servo_t *servo, servo_mode_t mode, int16_t speed) {
    uint8_t buf[10];
    buf[0] = buf[1] = BS_FRAME_HEADER;
    buf[2] = servo->id;
    buf[3] = 7;
    buf[4] = BS_SERVO_OR_MOTOR_MODE_WRITE;
    buf[5] = mode;
    buf[6] = 0;
    buf[7] = GET_LOW_BYTE((uint16_t)speed);
    buf[8] = GET_HIGH_BYTE((uint16_t)speed);
    buf[9] = _gen_checksum(buf);
    return (_send_action_cmd(buf));
}

bool servo_status_inbound_pending(void) {
    return (_servo_in_proc != SERVO_NONE);
}

bool servo_stop_move(servo_t *servo) {
    uint8_t buf[6];
    buf[0] = buf[1] = BS_FRAME_HEADER;
    buf[2] = servo->id;
    buf[3] = 3;
    buf[4] = BS_MOVE_STOP;
    buf[5] = _gen_checksum(buf);
    return (_send_action_cmd(buf));
}

bool servo_unload(servo_t* servo) {
    uint8_t buf[7];
    buf[0] = buf[1] = BS_FRAME_HEADER;
    buf[2] = servo->id;
    buf[3] = 4;
    buf[4] = BS_LOAD_OR_UNLOAD_WRITE;
    buf[5] = 0;
    buf[6] = _gen_checksum(buf);
    return (_send_action_cmd(buf));
}

int16_t servo_vin(servo_t *servo) {
    // Make sure this is a position packet.
    if (servo->_rxstatus.buf[BSPKT_CMD] != BS_VIN_READ) {
        return (-1);
    }
    return ((int16_t)BYTES_TO_WORD(servo->_rxstatus.buf[BSPKT_DATA + 2], servo->_rxstatus.buf[BSPKT_DATA + 1]));
}

bool servo_vin_read(servo_t *servo) {
    uint8_t buf[6];

    buf[0] = buf[1] = BS_FRAME_HEADER;
    buf[2] = servo->id;
    buf[3] = 3;
    buf[4] = BS_VIN_READ;
    buf[5] = _gen_checksum(buf);

    return (_send_rd_status_cmd(servo, buf));
}

void servo_module_init() {
    static bool _initialized = false;

    if (_initialized) {
        board_panic("servo_module_init already called");
    }
    _initialized = true;
    _tx_disable();
    //
    // Clear out the servo in progress.
    _servo_in_proc = SERVO_NONE;
    _rxd_handler = _rxd_discard;
    cmt_msg_init(&_msg_rxd_to, MSG_SERVO_DATA_RX_TO);
    _msg_rxd_to.hndlr = _rxd_status_asm_to;  // Handler for RX receive timeout
    // Set up our UART with the required speed.
    uart_init(SERVO_CTRL_UART, BS_BAUDRATE);
    uart_set_hw_flow(SERVO_CTRL_UART, false, false);  // CTS/RTS off
    uart_set_format(SERVO_CTRL_UART, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(SERVO_CTRL_UART, true);
    uart_set_translate_crlf(SERVO_CTRL_UART, false);
    // Set the TX and RX pins by using the function select on the GPIO
    gpio_set_function(SERVO_CTRL_TX, GPIO_FUNC_UART);
    gpio_set_function(SERVO_CTRL_RX, GPIO_FUNC_UART);
    // Set up the interrupt handler
    irq_set_exclusive_handler(SERVO_CTRL_IRQ, _on_uart_rx);
    irq_set_enabled(SERVO_CTRL_IRQ, false);
    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(SERVO_CTRL_UART, true, false);
}

void servo_module_start() {
    _uart_drain();
}

