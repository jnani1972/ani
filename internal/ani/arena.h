#ifndef ANI_ARENA_H
#define ANI_ARENA_H

#include <stddef.h>

// ANIArena is a simple bump allocator that allocates from fixed-size blocks.
// All memory is freed at once via ani_arena_destroy(). Individual frees are not
// supported — this is by design for per-file extraction where all data has the
// same lifetime.
#define ANI_ARENA_MAX_BLOCKS 256
#define ANI_ARENA_DEFAULT_BLOCK_SIZE (64 * 1024) // 64KB initial

typedef struct {
    char *blocks[ANI_ARENA_MAX_BLOCKS];
    size_t block_sizes[ANI_ARENA_MAX_BLOCKS]; // per-block sizes (for stats)
    int nblocks;
    size_t block_size;
    size_t used;        // bytes used in current block
    size_t total_alloc; // cumulative bytes allocated (for stats)
} ANIArena;

// Initialize an arena with the default block size.
void ani_arena_init(ANIArena *a);

// Allocate n bytes from the arena. Returns NULL on OOM or block exhaustion.
// All returned pointers are 8-byte aligned.
void *ani_arena_alloc(ANIArena *a, size_t n);

// Duplicate a string into arena memory. Returns arena-owned copy.
char *ani_arena_strdup(ANIArena *a, const char *s);

// Duplicate a string of known length into arena memory. NUL-terminates.
char *ani_arena_strndup(ANIArena *a, const char *s, size_t len);

// sprintf into arena memory. Returns arena-owned string.
char *ani_arena_sprintf(ANIArena *a, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

// Free all blocks. Arena is invalid after this call.
void ani_arena_destroy(ANIArena *a);

#endif // ANI_ARENA_H
