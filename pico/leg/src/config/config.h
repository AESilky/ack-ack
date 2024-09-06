/**
 * Configuration functionality
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 *
 */
#ifndef _CONFIG_H_
#define _CONFIG_H_
#ifdef __cplusplus
 extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define CONFIG_VERSION 1

typedef struct _config_ {
    /** Configuration Version */
    uint16_t cfg_version;
    /** Configuration Name */
} config_t;

/**
 * @brief Clear the values of a configuration instance to the initial values.
 * @ingroup config
 *
 * @param cfg The config instance to clear the values from.
 * @return config_t* Pointer to the destination config instance passed in (for convenience)
 */
extern config_t* config_clear(config_t* cfg);

/**
 * @brief Copy values from one config instance to another.
 * @ingroup config
 *
 * @param cfg_dest An existing config instance to copy values into.
 * @param cfg_source Config instance to copy values from.
 * @return config_t* Pointer to the destination config instance passed in (for convenience)
 */
extern config_t* config_copy(config_t* cfg_dest, const config_t* cfg_source);

/**
 * @brief Get the current configuration.
 * @ingroup config
 *
 * @return config_t* Current configuration.
 */
extern const config_t* config_current();

/**
 * @brief Get the current configuration to be modified.
 * @ingroup config
 *
 * @return config_t* Current configuration.
 */
extern config_t* config_current_for_modification();

/**
 * @brief Free a config_t structure previously allocated with config_new.
 * @ingroup config
 *
 * @see config_new(config_t*)
 *
 * @param config Pointer to the config_t structure to free.
 */
extern void config_free(config_t* config);

/**
 * @brief Post a message to both cores indicating that the configuration has changed.
 */
extern void config_indicate_changed();

/**
 * @brief Make a new config the current config.
 * @ingroup config
 *
 * After setting the new config as the current config, the config changed
 * message will be posted to both cores.
 *
 * @param new_config The config to make current (replacing the current config)
 */
extern void config_make_current(config_t* new_config);

/**
 * @brief Allocate a new config_t structure. Optionally, initialize values.
 * @ingroup config
 *
 * @see config_free(config_t* config)
 * @param init_values Config structure with initial values, or NULL for an empty config.
 * @return config_t* A newly allocated config_t structure. Must be free'ed with `config_free()`.
 */
extern config_t* config_new(const config_t* init_values);

/**
 * @brief Save the current configuration. Optionally set this as the boot configuration.
 * @ingroup config
 *
 * @return bool True if successful.
 */
extern bool config_save(void);

/**
 * @brief Initialize the configuration subsystem
 * @ingroup config
 *
 * @return 0 if succesfull. The 'not initialized' bits on fail.
*/
extern int config_module_init();

#ifdef __cplusplus
}
#endif
#endif // _CONFIG_H_
