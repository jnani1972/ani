/*
 * userconfig.h — User-defined file extension → language mappings.
 *
 * Reads extra_extensions from two optional JSON config files:
 *   Global:  $XDG_CONFIG_HOME/ani/config.json
 *            (falls back to ~/.config/ani/config.json)
 *   Project: {repo_root}/.ani.json
 *
 * Project config wins over global. Unknown language values warn and are
 * skipped (fail-open). Missing files are silently ignored.
 *
 * Format:
 *   {"extra_extensions": {".blade.php": "php", ".mjs": "javascript"}}
 *
 * The language string matching is case-insensitive.
 */
#ifndef ANI_USERCONFIG_H
#define ANI_USERCONFIG_H

#include "ani.h" /* ANILanguage */
#include "foundation/sha256.h"

/* ── Types ──────────────────────────────────────────────────────── */

typedef struct {
    char *ext;        /* file extension including dot, e.g. ".blade.php" */
    ANILanguage lang; /* resolved language enum */
} ani_userext_t;

typedef struct {
    ani_userext_t *entries; /* heap-allocated array */
    int count;              /* number of entries */
    /* Digests of the exact bytes/state consumed by ani_userconfig_load(). */
    char global_source_sha256[ANI_SHA256_HEX_LEN + 1];
    char project_source_sha256[ANI_SHA256_HEX_LEN + 1];
} ani_userconfig_t;

/* ── API ────────────────────────────────────────────────────────── */

/*
 * Load user config from global + project files, merge (project wins).
 * repo_path: absolute path to the repository root (for project config).
 * Returns a heap-allocated ani_userconfig_t (caller must free via
 * ani_userconfig_free). Returns NULL only on allocation failure.
 * Missing config files are silently ignored.
 */
ani_userconfig_t *ani_userconfig_load(const char *repo_path);

/*
 * Look up a file extension in the user config.
 * ext: extension including dot, e.g. ".blade.php"
 * Returns the mapped ANILanguage, or ANI_LANG_COUNT if not found.
 */
ANILanguage ani_userconfig_lookup(const ani_userconfig_t *cfg, const char *ext);

/* Free a ani_userconfig_t returned by ani_userconfig_load. NULL-safe. */
void ani_userconfig_free(ani_userconfig_t *cfg);

/* ── Integration hook ───────────────────────────────────────────── */

/*
 * Set the process-global user config that ani_language_for_extension()
 * will consult before the built-in table.
 * cfg may be NULL to clear the override.
 * Not thread-safe — call before spawning worker threads.
 */
void ani_set_user_lang_config(const ani_userconfig_t *cfg);

/*
 * Get the currently active process-global user config.
 * Returns NULL if none has been set.
 * Called internally by ani_language_for_extension().
 */
const ani_userconfig_t *ani_get_user_lang_config(void);

#endif /* ANI_USERCONFIG_H */
