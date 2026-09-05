/* compact_out.h — TOON (Token-Oriented Object Notation) emission helpers.
 *
 * Tool responses are consumed by LLM agents, where every byte is context
 * tokens. TOON encodes the same data as JSON but declares tabular fields
 * once in a header and streams rows line by line, cutting 40-60% of tokens
 * on homogeneous result sets at equal-or-better retrieval accuracy
 * (toonformat.dev/guide/benchmarks). Emitters here cover the subset we
 * emit: scalar key-value lines and flat tables with explicit [N] lengths.
 *
 * Quoting: a cell/scalar is double-quoted iff it is empty, has leading or
 * trailing whitespace, contains a comma, quote, newline, or CR, or would
 * read as a non-string literal (true/false/null/number). Quotes and
 * backslashes are escaped JSON-style; newlines become \n.
 */
#ifndef ANI_MCP_COMPACT_OUT_H
#define ANI_MCP_COMPACT_OUT_H

#include <stdbool.h>
#include <stddef.h>

/* Minimal growing string buffer (OOM-safe: sticky flag, finish returns NULL). */
typedef struct {
    char *buf;
    size_t len;
    size_t cap;
    bool oom;
} ani_sb_t;

void ani_sb_init(ani_sb_t *sb);
void ani_sb_append_n(ani_sb_t *sb, const char *s, size_t n);
void ani_sb_append(ani_sb_t *sb, const char *s);
/* Returns the heap buffer (caller frees) and resets sb. NULL on OOM. */
char *ani_sb_finish(ani_sb_t *sb);
void ani_sb_free(ani_sb_t *sb);

/* `key: value` scalar lines (top-level, no indent). */
void ani_tree_scalar_str(ani_sb_t *sb, const char *key, const char *val);
void ani_tree_scalar_int(ani_sb_t *sb, const char *key, long long v);
void ani_tree_scalar_bool(ani_sb_t *sb, const char *key, bool v);

/* `key[n]{col1,col2,...}:` table header; rows follow at 2-space indent. */
void ani_tree_table_header(ani_sb_t *sb, const char *key, int n, const char *const *cols,
                           int ncols);

/* Row cells: call row_begin, then cell_* per column (first=true for the
 * first cell), then row_end. Empty/NULL strings emit as empty cells. */
void ani_tree_row_begin(ani_sb_t *sb);
void ani_tree_cell_str(ani_sb_t *sb, const char *val, bool first);
void ani_tree_cell_int(ani_sb_t *sb, long long v, bool first);
void ani_tree_cell_real(ani_sb_t *sb, double v, bool first);
void ani_tree_cell_bool(ani_sb_t *sb, bool v, bool first);
void ani_tree_row_end(ani_sb_t *sb);

#endif /* ANI_MCP_COMPACT_OUT_H */
