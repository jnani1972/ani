/*
 * str_intern.h — String interning pool.
 *
 * Deduplicates strings: identical strings share a single allocation.
 * Returns stable pointers — safe to compare by pointer equality after interning.
 *
 * Uses an arena for string storage (bulk free) + hash table for dedup lookup.
 */
#ifndef ANI_STR_INTERN_H
#define ANI_STR_INTERN_H

#include <stddef.h>
#include <stdint.h>

typedef struct ANIInternPool ANIInternPool;

/* Create a new intern pool. */
ANIInternPool *ani_intern_create(void);

/* Free the pool and all interned strings. */
void ani_intern_free(ANIInternPool *pool);

/* Intern a NUL-terminated string. Returns a stable pointer.
 * The same input always returns the same pointer. */
const char *ani_intern(ANIInternPool *pool, const char *s);

/* Intern a string of known length. */
const char *ani_intern_n(ANIInternPool *pool, const char *s, size_t len);

/* Number of unique strings in the pool. */
uint32_t ani_intern_count(const ANIInternPool *pool);

/* Total bytes stored (unique strings only). */
size_t ani_intern_bytes(const ANIInternPool *pool);

#endif /* ANI_STR_INTERN_H */
