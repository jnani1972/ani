/*
 * config_toml_edit.h — Conservative TOML configuration editing helpers.
 */
#ifndef ANI_CONFIG_TOML_EDIT_H
#define ANI_CONFIG_TOML_EDIT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Escape input for the contents of a TOML basic string. The surrounding
 * double quotes are not written. Returns 0 on success and -1 on invalid input
 * or insufficient output space. */
int ani_toml_escape_basic_string(const char *input, char *out, size_t out_size);

/* Editors only rewrite missing paths or regular, single-link config files.
 * POSIX symlinks and Windows reparse points fail closed. Existing POSIX
 * owner/group/mode metadata is retained across the atomic replacement. The
 * destination and synced temporary file are identity/byte revalidated
 * immediately before publication. Portable platforms without a path-identity
 * CAS retain a narrow interval between final verification and replacement.
 *
 * Insert or replace one managed, line-delimited block. block is the body
 * between the two marker lines. Duplicate or unbalanced markers are rejected
 * without changing the file. */
int ani_toml_upsert_managed_block(const char *file_path, const char *begin_marker,
                                  const char *end_marker, const char *block);
int ani_toml_remove_managed_block(const char *file_path, const char *begin_marker,
                                  const char *end_marker);

/* Remove one pre-marker ani table only when it has the known
 * historical schema: one owned command basename, optional empty args, and no
 * unknown assignments or descendant tables. Returns 1 for a syntactically
 * valid same-name foreign table and leaves it byte-identical, 0 for removed or
 * absent owned state, and -1 for malformed input or I/O failure. A supplied
 * managed-marker pair makes this a successful no-op. */
int ani_toml_remove_legacy_table(const char *file_path, const char *table_name,
                                 const char *begin_marker, const char *end_marker);

/* Insert, replace, or remove the array table whose identity assignment equals
 * identity_value. table_name and identity_key must be TOML bare identifiers.
 * table_body excludes the [[table_name]] header. */
int ani_toml_upsert_named_array_table(const char *file_path, const char *table_name,
                                      const char *identity_key, const char *identity_value,
                                      const char *table_body);
int ani_toml_remove_named_array_table(const char *file_path, const char *table_name,
                                      const char *identity_key, const char *identity_value);

/* Ownership-aware variants for installer-managed array tables. canonical_body
 * excludes the [[table_name]] header. A missing identity is created by upsert
 * and is a successful no-op for removal. An existing identity is owned only
 * when its complete direct table body and header equal the canonical rendering
 * and it has no descendant tables. Same-name foreign state is preserved and
 * reported distinctly from malformed input, unsafe filesystem state, I/O, or
 * concurrency failures. */
enum {
    ANI_TOML_OWNED_EDIT_ERROR = -1,
    ANI_TOML_OWNED_EDIT_OK = 0,
    ANI_TOML_OWNED_EDIT_FOREIGN = 1
};
int ani_toml_upsert_owned_named_array_table(const char *file_path, const char *table_name,
                                            const char *identity_key, const char *identity_value,
                                            const char *canonical_body);
int ani_toml_remove_owned_named_array_table(const char *file_path, const char *table_name,
                                            const char *identity_key, const char *identity_value,
                                            const char *canonical_body);
typedef enum {
    ANI_TOML_CODEX_HOOK_UPSERT = 0,
    ANI_TOML_CODEX_HOOK_REMOVE = 1
} ani_toml_codex_hook_action_t;
typedef enum {
    ANI_TOML_CODEX_HOOK_FAILURE_NONE = 0,
    ANI_TOML_CODEX_HOOK_FAILURE_INVALID_ARGUMENT,
    ANI_TOML_CODEX_HOOK_FAILURE_COMMAND_RENDER,
    ANI_TOML_CODEX_HOOK_FAILURE_CONFIG_READ,
    ANI_TOML_CODEX_HOOK_FAILURE_UNSAFE_CONTENT,
    ANI_TOML_CODEX_HOOK_FAILURE_MALFORMED_CONFIG,
    ANI_TOML_CODEX_HOOK_FAILURE_AMBIGUOUS_OWNERSHIP,
    ANI_TOML_CODEX_HOOK_FAILURE_CONFLICTING_HOOKS,
    ANI_TOML_CODEX_HOOK_FAILURE_EDIT_BUILD,
    ANI_TOML_CODEX_HOOK_FAILURE_CONFIG_WRITE
} ani_toml_codex_hook_failure_t;
/* Returns a stable, content-free diagnostic name for a failure category. */
const char *ani_toml_codex_hook_failure_name(ani_toml_codex_hook_failure_t failure);
/* `failure` may be NULL. When non-NULL, it is reset to NONE on entry and remains
 * NONE on success; failure categories never contain configuration content. */
int ani_toml_reconcile_codex_hooks_detailed(const char *file_path, const char *begin_marker,
                                            const char *end_marker, const char *command,
                                            const char *command_windows,
                                            ani_toml_codex_hook_action_t action, int check_only,
                                            ani_toml_codex_hook_failure_t *failure);
int ani_toml_reconcile_codex_hooks(const char *file_path, const char *begin_marker,
                                   const char *end_marker, const char *command,
                                   const char *command_windows, ani_toml_codex_hook_action_t action,
                                   int check_only);

#ifdef ANI_TOML_EDIT_ENABLE_TEST_API
typedef void (*ani_toml_precommit_test_hook_t)(const char *file_path, void *context);
void ani_toml_set_precommit_hook_for_testing(ani_toml_precommit_test_hook_t hook, void *context);
/* Runs after the first stale-snapshot check and before final destination and
 * temporary-file identity revalidation. */
void ani_toml_set_prepublish_hook_for_testing(ani_toml_precommit_test_hook_t hook, void *context);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ANI_CONFIG_TOML_EDIT_H */
