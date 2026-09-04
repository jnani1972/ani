#pragma once
#include <stddef.h>
#include "arena.h"

#define ANI_MACRO_MAX_PARAMS 4
#define ANI_MACRO_TABLE_CAP 4096

typedef struct {
    const char *name;
    int param_count;
    const char *param_names[ANI_MACRO_MAX_PARAMS];
    const char *expansion;
    const char *resolved_callee;
} ANIMacroEntry;

typedef struct ANIMacroTable {
    ANIMacroEntry entries[ANI_MACRO_TABLE_CAP];
    int count;
    ANIArena arena;
} ANIMacroTable;

// Add an entry. Silently drops on overflow.
void ani_macro_table_add(ANIMacroTable *t, ANIArena *arena, const char *name, int param_count,
                         const char **param_names, const char *expansion,
                         const char *resolved_callee);

// Look up by name. Returns NULL if not found.
const ANIMacroEntry *ani_macro_table_find(const ANIMacroTable *t, const char *name);

// Parse a single .inc file content into the table (arena-allocated strings).
void ani_parse_inc_file(ANIMacroTable *t, ANIArena *arena, const char *content);

// Expand a macro call: substitute args into expansion text.
// Returns arena-allocated expanded text, or NULL if no expansion.
char *ani_macro_expand(ANIArena *arena, const ANIMacroEntry *entry, const char **args,
                       int arg_count);

// Extract a callee name from expanded text (looks for ##class(X).Method or $$Label^Routine).
// Returns arena-allocated "X.Method" or "Label^Routine", or NULL.
char *ani_macro_extract_callee(ANIArena *arena, const char *expansion);

// Allocate and populate a new table with the hardcoded system macros.
// Caller owns the table (stack or heap).
void ani_macro_table_init_system(ANIMacroTable *t);

// Destroy the arena inside t and free t itself. NULL-safe.
void ani_macro_table_free(ANIMacroTable *t);
