/**
 * hwctrl Cooperative Multi-Tasking.
 *
 * Contains message loop, scheduled message, and other CMT enablement functions.
 *
 * Copyright 2023-24 AESilky
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


#define SCHEDULED_MESSAGES_MAX 16

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

static proc_status_accum_t _psa[2]; // One Proc Status Accumulator for each core
static proc_status_accum_t _psa_sec[2]; // Proc Status Accumulator per second for each core

void cmt_handle_sleep(cmt_msg_t* msg);

const msg_handler_entry_t cmt_sm_tick_handler_entry = { MSG_CMT_SLEEP, cmt_handle_sleep };

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
    msg->hndlr = NULL_MSG_HNDLR;
    msg->n = 0;
    msg->t = 0;
}

void cmt_msg_init2(cmt_msg_t* msg, msg_id_t id, msg_priority_t priority) {
    msg->id = id;
    msg->priority = priority;
    msg->hndlr = NULL_MSG_HNDLR;
    msg->n = 0;
    msg->t = 0;
}

void cmt_msg_init3(cmt_msg_t* msg, msg_id_t id, msg_priority_t priority, msg_handler_fn hndlr) {
    msg->id = id;
    msg->priority = priority;
    msg->hndlr = hndlr;
    msg->n = 0;
    msg->t = 0;
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
        proc_status_accum_t psa;
        int64_t cs = 0;
        do {
            psa.cs = psa_sec->cs;   // Get the 'per-sec' checksum
            psa.core_temp = psa_sec->core_temp;
            psa.idle = psa_sec->idle;
            cs = psa.idle;
            psa.retrieved = psa_sec->retrieved;
            cs += psa.retrieved;
            psa.t_active = psa_sec->t_active;
            cs += psa.t_active;
            psa.t_idle = psa_sec->t_idle;
            cs += psa.t_idle;
            psa.interrupt_status = psa_sec->interrupt_status;
            cs += psa.interrupt_status;
            psa.ts_psa = psa_sec->ts_psa;

        } while(psa.cs != cs);
        psas->core_temp = psa.core_temp;
        psas->idle = psa.idle;
        psas->retrieved = psa.retrieved;
        psas->t_active = psa.t_active;
        psas->t_idle = psa.t_idle;
        psas->interrupt_status = psa.interrupt_status;
        psas->ts_psa = psa.ts_psa;
        psas->cs = psa.cs;
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
    const idle_fn* idle_functions = loop_context->idle_functions;
    proc_status_accum_t *psa = &_psa[corenum];
    proc_status_accum_t *psa_sec = &_psa_sec[corenum];
    psa->ts_psa = now_ms();

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
            int64_t cs = 0;
            psa_sec->cs = -1;
            psa_sec->idle = psa->idle;
            cs += psa_sec->idle;
            psa->idle = 0;
            psa_sec->retrieved = psa->retrieved;
            cs += psa_sec->retrieved;
            psa->retrieved = 0;
            psa_sec->t_active = psa->t_active;
            cs += psa_sec->t_active;
            psa->t_active = 0;
            psa_sec->t_idle = psa->t_idle;
            cs += psa_sec->t_idle;
            psa->t_idle = 0;
            psa_sec->interrupt_status = *nvic_hw->iser; // On Pico2 this is an array[2]
            cs += psa_sec->interrupt_status;
            psa_sec->core_temp = onboard_temp_c();
            psa_sec->ts_psa = t_start;
            psa->ts_psa = t_start;
            psa_sec->cs = cs;
        }

        if (get_msg_function(&msg)) {
            psa->retrieved++;
            // Find the handler
            //  Does the message designate a handler?
            if (msg.hndlr != NULL_MSG_HNDLR) {
                gpio_put(PICO_DEFAULT_LED_PIN, 1);
                msg.hndlr(&msg);
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
            uint64_t ht = now_us() - t_start;
            psa->t_active += ht;
        }
        else {
            // No message available, allow next idle function to run
            uint64_t is = now_us();
            psa->idle++;
            const idle_fn idle_function = *idle_functions;
            if (idle_function) {
                idle_function();
                idle_functions++; // Next time do the next one...
            }
            else {
                // end of function list
                idle_functions = loop_context->idle_functions; // reset the pointer
            }
            uint64_t it = now_us() - is;
            psa->t_idle += it;
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
