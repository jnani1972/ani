/*
 * config.h — Persistent UI configuration.
 *
 * Stores ui_enabled and ui_port in ~/.cache/ani/config.json.
 * Thread-safe: load/save are independent operations on the filesystem.
 */
#ifndef ANI_UI_CONFIG_H
#define ANI_UI_CONFIG_H

#include <stdbool.h>

/* Default values */
#define ANI_UI_DEFAULT_PORT 9749
#define ANI_UI_DEFAULT_ENABLED false

typedef struct {
    bool ui_enabled;
    int ui_port;
} ani_ui_config_t;

/* Load config from disk. Missing/corrupt file → defaults. */
void ani_ui_config_load(ani_ui_config_t *cfg);

/* Atomically save one complete config generation. Creates the directory if
 * needed and reports write/sync/replace failures. */
bool ani_ui_config_save(const ani_ui_config_t *cfg);

/* Get the config file path. Writes to buf (up to bufsz bytes).
 * Exposed for testing. */
void ani_ui_config_path(char *buf, int bufsz);

#endif /* ANI_UI_CONFIG_H */
