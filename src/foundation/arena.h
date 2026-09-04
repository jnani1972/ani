/*
 * arena.h — Bump allocator with block-based growth.
 *
 * All memory is freed at once via ani_arena_destroy(). Individual frees are
 * not supported — this is by design for per-file extraction where all data
 * has the same lifetime.
 *
 * Restructured from internal/ani/arena.h for the pure C rewrite.
 * New additions: ani_arena_reset() for reuse without realloc.
 */
#ifndef ANI_ARENA_H
#define ANI_ARENA_H

#include <stddef.h>
#include <stdarg.h>

#define ANI_ARENA_MAX_BLOCKS 256
#define ANI_ARENA_DEFAULT_BLOCK_SIZE ((size_t)64 * 1024) /* 64KB */

typedef struct {
    char *blocks[ANI_ARENA_MAX_BLOCKS];
    size_t block_sizes[ANI_ARENA_MAX_BLOCKS]; /* per-block sizes (for stats) */
    int nblocks;
    size_t block_size;  /* current block capacity */
    size_t used;        /* bytes used in current block */
    size_t total_alloc; /* cumulative bytes allocated (for stats) */
} ANIArena;

/* Initialize arena with default block size. */
void ani_arena_init(ANIArena *a);

/* Initialize arena with a custom initial block size. */
void ani_arena_init_sized(ANIArena *a, size_t block_size);

/* Allocate n bytes (8-byte aligned). Returns NULL on OOM. */
void *ani_arena_alloc(ANIArena *a, size_t n);

/* Allocate n bytes, zero-initialized. */
void *ani_arena_calloc(ANIArena *a, size_t n);

/* Duplicate a NUL-terminated string. */
char *ani_arena_strdup(ANIArena *a, const char *s);

/* Duplicate a string of known length, NUL-terminate. */
char *ani_arena_strndup(ANIArena *a, const char *s, size_t len);

/* sprintf into arena memory. */
char *ani_arena_sprintf(ANIArena *a, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

/* Reset arena for reuse: keeps first block, frees the rest. */
void ani_arena_reset(ANIArena *a);

/* Free all blocks. Arena is zeroed after this. */
void ani_arena_destroy(ANIArena *a);

/* Return total bytes allocated (for diagnostics). */
size_t ani_arena_total(const ANIArena *a);

#endif /* ANI_ARENA_H */
