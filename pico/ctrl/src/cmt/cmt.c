/**
 * hwctrl Cooperative Multi-Tasking.
 *
 * Contains message loop, scheduled message, and other CMT enablement functions.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#include "cmt.h"
#include "system_defs.h"
#include "board.h"
#include "debug_support.h"
#include "util/util.h"

#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "hardware/structs/nvic.h"
#include "pico/float.h"
#include "pico/mutex.h"
#include "pico/stdlib.h"
#include "pico/time.h"

#include <string.h>


#define SCHEDULED_MESSAGES_MAX 32

#define SM_OVERHEAD_US_PER_MS_ (20)
#define SMD_FREE_INDICATOR_ (-1)

typedef bool (*get_msg_nowait_fn)(cmt_msg_t* msg);

typedef struct _scheduled_msg_data_ {
    int32_t remaining;
    uint8_t corenum;
    int32_t ms_requested;
    const cmt_msg_t* client_msg;
    cmt_msg_t sleep_msg;
} _scheduled_msg_data_t;


auto_init_mutex(sm_mutex);
static _scheduled_msg_data_t _scheduled_message_datas[SCHEDULED_MESSAGES_MAX]; // Objects to use (no malloc/free)

static bool _msg_loop_0_running = false;
static bool _msg_loop_1_running = false;

static proc_status_accum_t _psa[2];         // One Proc Status Accumulator for each core
static proc_status_accum_t _psa_sec[2];     // Proc Status Accumulator per second for each core

void cmt_handle_sleep(cmt_msg_t* msg);

const msg_handler_entry_t cmt_sm_sleep_handler_entry = { MSG_CMT_SLEEP, cmt_handle_sleep };

static uint8_t _housekeep_rt;  // Incremented each ms. Generates a Housekeeping msg every 16ms (62.5Hz)

/**
 * @brief Recurring Interrupt Handler (1ms from PWM).
 *
 * Handles the PWM 'wrap' recurring interrupt. This adjusts the time left in scheduled messages
 * (including our 'sleep') and posts a message to the appropriate core when time hits 0.
 *
 * This also posts a MSG_HOUSEKEEPING_RT message every 16ms (62.5Hz) that allows modules
 * to perform regular operations without having to set up scheduled messages or timers
 * of their own.
 *
 */
static void _on_recurring_interrupt(void) {
        // Adjust scheduled messages.
        for (int i = 0; i < SCHEDULED_MESSAGES_MAX; i++) {
            _scheduled_msg_data_t* smd = &_scheduled_message_datas[i];
            if (smd->remaining > 0) {
                if (0 == --smd->remaining) {
                    if (0 == smd->corenum) {
                        post_to_core0(smd->client_msg);
                    }
                    else {
                        post_to_core1(smd->client_msg);
                    }
                    smd->remaining = SMD_FREE_INDICATOR_;
                }
            }
        }
        _housekeep_rt = ((_housekeep_rt + 1) & 0x0F);
        if (_housekeep_rt == 0) {
            cmt_msg_t msg;
            cmt_msg_init2(&msg, MSG_HOUSEKEEPING_RT, MSG_PRI_LP);
            postBothMsgDiscardable(&msg);  // Housekeeping RT is low-priority/discardable
        }
    // Clear the interrupt flag that brought us here so it can occur again.
    pwm_clear_irq(CMT_PWM_RECINT_SLICE);
}

static void _scheduled_msg_init() {
    for (int i = 0; i < SCHEDULED_MESSAGES_MAX; i++) {
        // Initialize these as 'free'
        _scheduled_msg_data_t* smd = &_scheduled_message_datas[i];
        smd->remaining = SMD_FREE_INDICATOR_;
    }
}

void cmt_msg_init(cmt_msg_t* msg, msg_id_t id) {
    msg->id = id;
    msg->priority = MSG_PRI_NORM;
    msg->hdlr = NULL_MSG_HDLR;
    msg->n = 0;
    msg->t = 0;
}

void cmt_msg_init2(cmt_msg_t* msg, msg_id_t id, msg_priority_t priority) {
    msg->id = id;
    msg->priority = priority;
    msg->hdlr = NULL_MSG_HDLR;
    msg->n = 0;
    msg->t = 0;
}

void cmt_msg_init3(cmt_msg_t* msg, msg_id_t id, msg_priority_t priority, msg_handler_fn hdlr) {
    msg->id = id;
    msg->priority = priority;
    msg->hdlr = hdlr;
    msg->n = 0;
    msg->t = 0;
}

void cmt_msg_rm_hdlr(cmt_msg_t* msg) {
    msg->hdlr = NULL;
}


bool cmt_message_loop_0_running() {
    return (_msg_loop_0_running);
}

bool cmt_message_loop_1_running() {
    return (_msg_loop_1_running);
}

bool cmt_message_loops_running() {
    return (_msg_loop_0_running && _msg_loop_1_running);
}

void cmt_handle_sleep(cmt_msg_t* msg) {
    cmt_sleep_fn fn = msg->data.cmt_sleep.sleep_fn;
    if (fn) {
        (fn)(msg->data.cmt_sleep.user_data);
    }
}

void cmt_proc_status_sec(proc_status_accum_t* psas, uint8_t corenum) {
    if (corenum < 2) {
        proc_status_accum_t* psa_sec = &_psa_sec[corenum];
        psas->retrieved = psa_sec->retrieved;
        psas->t_active = psa_sec->t_active;
        psas->msg_longest = psa_sec->msg_longest;
        psas->t_msg_longest = psa_sec->t_msg_longest;
        psas->interrupt_status = psa_sec->interrupt_status;
        psas->ts_psa = psa_sec->ts_psa;
    }
}

int cmt_sched_msg_waiting() {
    int count = 0;
    uint32_t flags = save_and_disable_interrupts();
    mutex_enter_blocking(&sm_mutex);
    for (int i = 0; i < SCHEDULED_MESSAGES_MAX; i++) {
        _scheduled_msg_data_t* smd = &_scheduled_message_datas[i];
        if (SMD_FREE_INDICATOR_ != smd->remaining) {
            count++;
        }
    }
    mutex_exit(&sm_mutex);
    restore_interrupts_from_disabled(flags);

    return (count);
}

bool cmt_sched_msg_waiting_ids(int max, uint16_t *buf) {
    bool msgs_waiting = false;
    int values_num = (max > SCHEDULED_MESSAGES_MAX ? SCHEDULED_MESSAGES_MAX : max);
    int values_index = 0;
    buf[0] = -1; // Put a '-1' in to indicate the end
    uint32_t flags = save_and_disable_interrupts();
    mutex_enter_blocking(&sm_mutex);
    for (int i = 0; i < values_num; i++) {
        _scheduled_msg_data_t* smd = &_scheduled_message_datas[i];
        if (SMD_FREE_INDICATOR_ != smd->remaining) {
            msgs_waiting = true;
            buf[values_index] = smd->client_msg->id;
            values_index++;
        }
        // If we are less than the 'max' put a '-1' in to indicate the end.
        if (i+1 < max) {
            buf[i+1] = -1;
        }
    }
    mutex_exit(&sm_mutex);
    restore_interrupts_from_disabled(flags);

    return (msgs_waiting);
}

void cmt_sleep_ms(int32_t ms, cmt_sleep_fn sleep_fn, void* user_data) {
    bool scheduled = false;

    uint8_t core_num = (uint8_t)get_core_num();
    uint32_t flags = save_and_disable_interrupts();
    mutex_enter_blocking(&sm_mutex);
    // Get a free smd
    for (int i = 0; i < SCHEDULED_MESSAGES_MAX; i++) {
        _scheduled_msg_data_t* smd = &_scheduled_message_datas[i];
        if (SMD_FREE_INDICATOR_ == smd->remaining) {
            // This is free;
            smd->sleep_msg.id = MSG_CMT_SLEEP;
            smd->sleep_msg.data.cmt_sleep.sleep_fn = sleep_fn;
            smd->sleep_msg.data.cmt_sleep.user_data = user_data;
            smd->client_msg = &smd->sleep_msg;
            smd->ms_requested = ms;
            smd->corenum = core_num;
            smd->remaining = ms;
            scheduled = true;
            break;
        }
    }
    mutex_exit(&sm_mutex);
    restore_interrupts_from_disabled(flags);
    if (!scheduled) {
        board_panic("CMT - No SMD available for use for sleep.");
    }
}

void _schedule_core_msg_in_ms(uint8_t core_num, int32_t ms, const cmt_msg_t* msg) {
    bool scheduled = false;
    uint32_t flags = save_and_disable_interrupts();
    mutex_enter_blocking(&sm_mutex);
    // Get a free smd
    for (int i = 0; i < SCHEDULED_MESSAGES_MAX; i++) {
        _scheduled_msg_data_t* smd = &_scheduled_message_datas[i];
        if (SMD_FREE_INDICATOR_ == smd->remaining) {
            // This is free;
            smd->client_msg = msg;
            smd->ms_requested = ms;
            smd->corenum = core_num;
            smd->remaining = ms;
            scheduled = true;
            break;
        }
    }
    mutex_exit(&sm_mutex);
    restore_interrupts_from_disabled(flags);
    if (!scheduled) {
        board_panic("CMT - No SM Data slot available for use.");
    }
}

void schedule_core0_msg_in_ms(int32_t ms, const cmt_msg_t* msg) {
    _schedule_core_msg_in_ms(0, ms, msg);
}

void schedule_core1_msg_in_ms(int32_t ms, const cmt_msg_t* msg) {
    _schedule_core_msg_in_ms(1, ms, msg);
}

void schedule_msg_in_ms(int32_t ms, const cmt_msg_t* msg) {
    uint8_t core_num = (uint8_t)get_core_num();
    _schedule_core_msg_in_ms(core_num, ms, msg);
}

void scheduled_msg_cancel(msg_id_t sched_msg_id) {
    uint32_t flags = save_and_disable_interrupts();
    mutex_enter_blocking(&sm_mutex);
    for (int i = 0; i < SCHEDULED_MESSAGES_MAX; i++) {
        _scheduled_msg_data_t* smd = &_scheduled_message_datas[i];
        if (smd->remaining != SMD_FREE_INDICATOR_ && smd->client_msg && smd->client_msg->id == sched_msg_id) {
            // This matches, so set the remaining to -1;
            smd->remaining = SMD_FREE_INDICATOR_;
        }
    }
    mutex_exit(&sm_mutex);
    restore_interrupts_from_disabled(flags);
}

extern bool scheduled_message_exists(msg_id_t sched_msg_id) {
    bool exists = false;
    uint32_t flags = save_and_disable_interrupts();
    mutex_enter_blocking(&sm_mutex);
    for (int i = 0; i < SCHEDULED_MESSAGES_MAX; i++) {
        _scheduled_msg_data_t* smd = &_scheduled_message_datas[i];
        if (smd->remaining != SMD_FREE_INDICATOR_ && smd->client_msg && smd->client_msg->id == sched_msg_id) {
            // This matches
            exists = true;
            break;
        }
    }
    mutex_exit(&sm_mutex);
    restore_interrupts_from_disabled(flags);
    return (exists);
}

/*
 * Endless loop reading and dispatching messages.
 * This is called/started once from each core, so two instances are running.
 */
void message_loop(const msg_loop_cntx_t* loop_context, start_fn fstart) {
    // Setup occurs once when called by a core.
    uint8_t corenum = loop_context->corenum;
    get_msg_nowait_fn get_msg_function = (corenum == 0 ? get_core0_msg_nowait : get_core1_msg_nowait);
    cmt_msg_t msg;
    proc_status_accum_t *psa = &_psa[corenum];
    proc_status_accum_t *psa_sec = &_psa_sec[corenum];
    psa->ts_psa = now_us();

    // Indicate that the message loop is running for the calling core.
    if (corenum == 0) {
        _msg_loop_0_running = true;
    }
    else {
        _msg_loop_1_running = true;
    }

    // Call the 'started' notification function...
    if (fstart) {
        fstart();
    }

    // Enter into the endless loop reading and dispatching messages to the handlers...
    while (1) {
        uint64_t t_start = now_us();
            // Store and reset the process status accumulators once every second
        if (t_start - psa->ts_psa >= ONE_SECOND_US) {
            psa_sec->retrieved = psa->retrieved;
            psa->retrieved = 0;
            psa_sec->t_active = psa->t_active;
            psa->t_active = 0;
            psa_sec->interrupt_status = *nvic_hw->iser; // On Pico2 this is an array[2]
            psa_sec->msg_longest = psa->msg_longest;
            psa_sec->t_msg_longest = psa->t_msg_longest;
            psa->msg_longest = MSG_COMMON_NOOP;
            psa->t_msg_longest = 0;
            psa_sec->ts_psa = psa->ts_psa;
            psa->ts_psa = t_start;
        }

        if (get_msg_function(&msg)) {
            psa->retrieved += 1; // A message was retrieved, count it
            // Find the handler
            //  Does the message designate a handler?
            if (msg.hdlr != NULL_MSG_HDLR) {
                gpio_put(PICO_DEFAULT_LED_PIN, 1); // Turn the Pico LED on while the handler runs
                msg.hdlr(&msg);
                gpio_put(PICO_DEFAULT_LED_PIN, 0);
            }
            else {
                const msg_handler_entry_t** handler_entries = loop_context->handler_entries;
                while (*handler_entries) {
                    const msg_handler_entry_t* handler_entry = *handler_entries++;
                    if (msg.id == handler_entry->msg_id) {
                        gpio_put(PICO_DEFAULT_LED_PIN, 1);
                        handler_entry->msg_handler(&msg);
                        gpio_put(PICO_DEFAULT_LED_PIN, 0);
                    }
                }
            }
            // No more handlers found for this message.
            uint64_t now = now_us();
            uint64_t t_this_msg = now - t_start;
            psa->t_active += t_this_msg;
            // Update the 'longest' message if needed
            if (t_this_msg > psa->t_msg_longest) {
                psa->t_msg_longest = t_this_msg;
                psa->msg_longest = msg.id;
            }
        }
    }
}

void cmt_module_init() {
    // PWM is used to generate a 100µs interrupt that is used for
    // scheduled messages, sleep, and the regular housekeeping message.
    // (the PWM outputs are not directed to GPIO pins)
    //
    pwm_config cfg = pwm_get_default_config();
    // Calculate the clock divider to achieve a 1µs count rate.
    uint32_t sys_freq = clock_get_hz(clk_sys);
    float div = uint2float(sys_freq) / 1000000.0f;
    pwm_config_set_clkdiv(&cfg, div);
    pwm_config_set_wrap(&cfg, 1000);  // Reach 0 every millisecond
    pwm_init(CMT_PWM_RECINT_SLICE, &cfg, false);
    // These aren't used, but we set them...
    //   Set output high for one cycle before dropping
    pwm_set_chan_level(CMT_PWM_RECINT_SLICE, PWM_CHAN_A, 1);
    pwm_set_chan_level(CMT_PWM_RECINT_SLICE, PWM_CHAN_B, 1);
    // Mask our slice's IRQ output into the PWM block's single interrupt line,
    // and register our interrupt handler
    pwm_clear_irq(CMT_PWM_RECINT_SLICE);
    pwm_set_irq_enabled(CMT_PWM_RECINT_SLICE, true);
    irq_set_exclusive_handler(PWM_DEFAULT_IRQ_NUM(), _on_recurring_interrupt);

    mutex_enter_blocking(&sm_mutex);
    _scheduled_msg_init();
    mutex_exit(&sm_mutex);

    // Enable the PWM and interrupts from it.
    irq_set_enabled(PWM_DEFAULT_IRQ_NUM(), true);
    pwm_set_enabled(CMT_PWM_RECINT_SLICE, true);
}
