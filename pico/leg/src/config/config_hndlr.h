/**
 * KOB Configuration functionaly
 * Read/Write Handlers for file and command processing operations.
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 *
 */
#ifndef _KOB_CONFIG_HNDLR_H_
#define _KOB_CONFIG_HNDLR_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "config.h"

struct _CFG_ITEM_HANDLER_CLASS_;
typedef struct _CFG_ITEM_HANDLER_CLASS_ cfg_item_handler_class_t;

/**
 * @brief Config item handler type. Functions of this type used to process config lines.
 * @ingroup config
 *
 * Defines the signature of config item handlers.
 */
typedef int(*cfg_item_reader_fn)(const struct _CFG_ITEM_HANDLER_CLASS_* self, config_t* cfg, const char* value);
typedef int(*cfg_item_writer_fn)(const struct _CFG_ITEM_HANDLER_CLASS_* self, const config_t* cfg, char* buf, bool full);

struct _CFG_ITEM_HANDLER_CLASS_ {
    const char* key;
    const char short_opt;
    const char* long_opt;
    const char* label;
    const cfg_item_reader_fn reader;
    const cfg_item_writer_fn writer;
};

/**
 * @brief Get the collection of Config Handlers.
 * @ingroup config
 *
 * @return const cfg_item_handler_class_t** NULL terminated list of CFG Item Handlers.
 */
extern const cfg_item_handler_class_t** cfg_handlers();

/**
 * @brief Initialize the Config Item Handler module.
 * @ingroup config
 */
extern void config_hndlr_module_init();

#ifdef __cplusplus
}
#endif
#endif // _KOB_CONFIG_HNDLR_H_
