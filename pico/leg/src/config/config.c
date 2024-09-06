/**
 * Configuration functionaly
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 */
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include <stdlib.h>

#include "config.h"
#include "config_hndlr.h"

#include "board.h"
#include "os/os.h"
#include "cmt/cmt.h"
#include "util/util.h"

#include "string.h"

#define _CFG_VERSION_KEY "cfg_version"

#define _CFG_MEM_MARKER_ 3224 // *Magic* & *Air*

/**
 * @brief Holds the 'magic' marker and a config structure to safeguard free'ing.
 *
 * A config object holds values that are also malloc'ed, and therefore
 * need to be free'ed when the config object is free'ed. To safeguard
 * against a client simply free'ing the config object this structure is malloc'ed
 * and initialized, and then the contained config object is made available to clients.
 * This prevents clients from accidentally free'ing the config object
 * (using `free(config*)`), as it will cause a fault. Rather, the `config_free()`
 * method must be used. It correctly free's the additional malloc'ed objects as well
 * as the main object.
 */
typedef struct _CFG_W_MARKER_ {
    uint16_t marker;
    config_t config;
} _cfg_w_marker_t;

static config_t* _current_cfg;


// ============================================================================
// Public
// ============================================================================

extern config_t* config_clear(config_t* cfg) {
    if (cfg) {
        cfg->cfg_version = CONFIG_VERSION;
    }
    return (cfg);
}

config_t* config_copy(config_t* cfg_dest, const config_t* cfg_source) {
    if (cfg_dest && cfg_source) {
        config_clear(cfg_dest); // Assure that alloc'ed values are freed
    }
    return (cfg_dest);
}

const config_t* config_current() {
    return _current_cfg;
}

config_t* config_current_for_modification() {
    return _current_cfg;
}

void config_free(config_t* cfg) {
    if (cfg) {
        // If this is a config object there is a marker one byte before the beginning.
        // This is to keep it from accidentally being freed directly by `free`, as there
        // are contained structures that also need to be freed.
        _cfg_w_marker_t* cfgwm = (_cfg_w_marker_t*)((uint8_t*)cfg - (sizeof(_cfg_w_marker_t) - sizeof(config_t)));
        if (cfgwm->marker == _CFG_MEM_MARKER_) {
            // Okay, we can free things up...
            // First, free allocated values
            config_clear(cfg);
            // Now free up the main config structure
            free(cfgwm);
        }
    }
}

void config_indicate_changed() {
    postBothMsgIDBlocking(MSG_CONFIG_CHANGED, MSG_PRI_NORMAL);
}

extern bool config_load(int config_num) {
    config_t* cfg = config_new(NULL);
    config_copy(_current_cfg, cfg);
    config_free(cfg);
    config_indicate_changed();

    return (true);
}

void config_make_current(config_t* new_config) {
    if (new_config) {
        config_t* old_current = _current_cfg;
        _current_cfg = new_config;
        config_free(old_current);
        config_indicate_changed();
    }
}

config_t* config_new(const config_t* init_values) {
    // Allocate memory for a config structure, 'mark' it, and
    // set initial values.
    size_t size = sizeof(_cfg_w_marker_t);
    _cfg_w_marker_t* cfgwm = calloc(1, size);
    config_t* cfg = NULL;
    if (NULL != cfgwm) {
        cfgwm->marker = _CFG_MEM_MARKER_;
        cfg = &(cfgwm->config);
        config_clear(cfg);
        if (NULL != init_values) {
            cfg->cfg_version = init_values->cfg_version;
        }
    }

    return (cfg);
}

extern bool config_save(void) {
    return (true);
}

// ============================================================================
// Initialization
// ============================================================================

int config_module_init() {
    static bool _module_initialized = false;
    if (_module_initialized) {
        panic("config module already initialized.");
    }
    _module_initialized = true;

    // Create a config object to use as the current
    config_t* cfg = config_new(NULL);
    _current_cfg = cfg;

    // Initialize the item handlers and file operations modules
    config_hndlr_module_init();

    return (0);
}
