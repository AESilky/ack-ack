#include "dcs_rc.h"

#include "board.h"
#include "cmt/cmt.h"
#include "rcrx/rcrx.h"

// Direct Control state
static bool _dc;
// Direct Control Channel
static uint8_t _dcch;

// ====================================================================
// Local Methods
// ====================================================================

/**
 * @brief Read the 'Direct Control' state value from the RC Channel Buffer.
 *
 * If the RC Buffer indicates that it is in 'Fail-Safe' turn 'Direct Control'
 * off.
 */
static void _rc_rd_dc_state() {
    const rcrx_state_t* chst = rcrx_get_ch_state();
    bool dc_was = _dc;
    if (chst->failsafe) {
        _dc = false;
    }
    else {
        _dc = (chst->ch_data[_dcch].v > 0);
    }
    if (_dc != dc_was) {
        cmt_msg_t msg;
        cmt_msg_init(&msg, MSG_DIRECT_CTRL_CHG);
        msg.data.bv = _dc;
        postDCSMsg(&msg);
    }
}

// ====================================================================
// Message handler functions
// ====================================================================

/**
 * @brief Handle a Radio Control Receiver Update.
 *
 * @param msg
 */
static void _handle_rcrx_update(cmt_msg_t* msg) {
    _rc_rd_dc_state();
}

/**
 * @brief Handle a Radio Control Receiver 'FailSafe' changed.
 *
 * @param msg
 */
static void _handle_rcrx_failsafe_chg(cmt_msg_t* msg) {
    _rc_rd_dc_state();
}


// ====================================================================
// Public Methods
// ====================================================================

uint8_t dcs_rc_dcch() {
    return _dcch;
}

void dcs_rc_dcch_set(uint8_t channel) {
    _dcch = channel;
}

bool dcs_rc_direct_ctrl() {
    return _dc;
}

// ====================================================================
// Initialization and Start-Up Methods
// ====================================================================

void dcs_rc_start() {
    cmt_msg_hdlr_add(MSG_RC_RECEIVED, _handle_rcrx_update);
    cmt_msg_hdlr_add(MSG_RC_FAILSAFE_CHG, _handle_rcrx_failsafe_chg);
}

void dcs_rc_module_init() {
    static bool _initialized = false;

    if (_initialized) {
        board_panic("!!! `dcs_rc_module_init` called more than once !!!");
    }
    _initialized = true;

    _dc = false;
    _dcch = DIRECT_CTRL_SEL_CH;
}
