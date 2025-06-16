#include "dcs_rc.h"

#include "board.h"
#include "cmt/cmt.h"
#include "rcrx/rcrx.h"

// Direct Control state
static bool _dc;
// Direct Control Channel
static uint8_t _dcch;
// Forward-Rotate-Reverse state
static dcs_frr_t _frr;
// Forward-Rotate-Reverse Channel
static uint8_t _frrch;


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

static void _rc_rd_frr_control() {
    const rcrx_state_t* chst = rcrx_get_ch_state();
    dcs_frr_t frr_was = _frr;
    if (chst->failsafe) {
        _frr = DCS_FRR_ROTATE;
    }
    else {
        int16_t v = chst->ch_data[_frrch].v;
        if (v < -500) {
            _frr = DCS_FRR_FORWARD;
        }
        else if (v > 500) {
            _frr = DCS_FRR_REVERSE;
        }
        else {
            _frr = DCS_FRR_ROTATE;
        }
    }
    if (_frr != frr_was) {
        cmt_msg_t msg;
        cmt_msg_init(&msg, MSG_FORWARD_ROTATE_REVERSE_CHG);
        msg.data.value16 = (int16_t)_frr;
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
    _rc_rd_frr_control();
}

/**
 * @brief Handle a Radio Control Receiver 'FailSafe' changed.
 *
 * @param msg
 */
static void _handle_rcrx_failsafe_chg(cmt_msg_t* msg) {
    _rc_rd_dc_state();
    _rc_rd_frr_control();
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

uint8_t dcs_rc_frrch() {
    return _frrch;
}

void dcs_rc_frrch_set(uint8_t channel) {
    _frrch = channel;
}

dcs_frr_t dcs_rc_fwd_rot_rev() {
    return _frr;
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
    _dcch = CH_DIRECT_CTRL_SEL;
    _frr = DCS_FRR_ROTATE;
    _frrch = CH_FWD_ROT_REV;
}
