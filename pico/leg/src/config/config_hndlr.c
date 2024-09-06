/**
 * Configuration functionaly
 * Read/Write Handlers for file and command processing operations.
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 */

#include "config_hndlr.h"

#include "config.h"

#include "board.h"
#include "util/util.h"

#include "pico/platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int _cih_config_version_writer(const cfg_item_handler_class_t* self, const config_t* cfg, char* buf, bool full);

/**
 * @brief Array of config item class instances.
 * @ingroup config
 *
 * The entries should be in the order that the config lines should be written to the config file.
 */
const cfg_item_handler_class_t* _cfg_handlers[] = {
    ((const cfg_item_handler_class_t*)0), // NULL last item to signify end
};

const cfg_item_handler_class_t** cfg_handlers() {
    return _cfg_handlers;
}

void config_hndlr_module_init() {
    static bool _module_initialized = false;
    if (_module_initialized) {
        panic("config_hndlr module already initialized.");
    }
    _module_initialized = true;
}
