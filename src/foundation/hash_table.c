/*
 * hash_table.c — ANIHashTable backed by Verstable.
 *
 * Public API in hash_table.h is unchanged. Internals are a Verstable
 * template instantiation (const char* → void*). Verstable is a 2024
 * open-addressing hash table using quadratic probing with metadata
 * stored separately from buckets (4-bit hash fragment + 11-bit
 * displacement + 1-bit in-home-bucket flag per uint16_t). Documented
 * in vendored/verstable/verstable.h.
 *
 * Why swap the prior Robin Hood implementation: cumulative profiling
 * showed ani_ht_get is a hot path in resolve_file_calls's per-call
 * registry resolution. Verstable's 4-bit hash-fragment metadata
 * sidesteps most key comparisons during chain walks, which the prior
 * implementation could not.
 *
 * Lifetime: keys are BORROWED pointers (caller owns the strings).
 * Verstable's KEY_TY is const char*; the templated comparison +
 * hash use the standard vt_cmpr_string / vt_hash_string helpers.
 */
#include "foundation/constants.h"
#include "hash_table.h"
#include <stdlib.h>
#include <string.h>

/* Instantiate a Verstable map of (const char* → void*). The single
 * include below generates static inline functions named ani_vt_init,
 * ani_vt_cleanup, ani_vt_get, ani_vt_insert, etc., plus the ani_vt
 * struct itself. */
#define NAME ani_vt
#define KEY_TY const char *
#define VAL_TY void *
#define HASH_FN vt_hash_string
#define CMPR_FN vt_cmpr_string
#include "../../internal/ani/vendored/verstable/verstable.h"

/* The opaque ANIHashTable struct holds the Verstable instance + a
 * count cache (Verstable's _size traversal is O(buckets) so we keep
 * our own atomic-free counter). */
struct ANIHashTable {
    ani_vt vt;
};

ANIHashTable *ani_ht_create(uint32_t initial_capacity) {
    ANIHashTable *ht = (ANIHashTable *)calloc(ANI_ALLOC_ONE, sizeof(*ht));
    if (!ht)
        return NULL;
    ani_vt_init(&ht->vt);
    if (initial_capacity > 0) {
        /* Reserve enough buckets for the requested entries. Verstable
         * computes the minimum bucket count internally. */
        if (!ani_vt_reserve(&ht->vt, (size_t)initial_capacity)) {
            ani_vt_cleanup(&ht->vt);
            free(ht);
            return NULL;
        }
    }
    return ht;
}

void ani_ht_free(ANIHashTable *ht) {
    if (!ht)
        return;
    ani_vt_cleanup(&ht->vt);
    free(ht);
}

void *ani_ht_set(ANIHashTable *ht, const char *key, void *value) {
    if (!ht || !key)
        return NULL;
    /* Capture previous value (if any) before overwriting.
     * Verstable's _insert overwrites silently and returns an iterator
     * to the (now updated) entry — we have to peek first to surface
     * the prior value to the caller (back-compat with our API). */
    void *prev = NULL;
    ani_vt_itr itr = ani_vt_get(&ht->vt, key);
    if (!ani_vt_is_end(itr)) {
        prev = itr.data->val;
    }
    (void)ani_vt_insert(&ht->vt, key, value);
    return prev;
}

void *ani_ht_get(const ANIHashTable *ht, const char *key) {
    if (!ht || !key)
        return NULL;
    ani_vt_itr itr = ani_vt_get(&ht->vt, key);
    if (ani_vt_is_end(itr))
        return NULL;
    return itr.data->val;
}

bool ani_ht_has(const ANIHashTable *ht, const char *key) {
    if (!ht || !key)
        return false;
    ani_vt_itr itr = ani_vt_get(&ht->vt, key);
    return !ani_vt_is_end(itr);
}

const char *ani_ht_get_key(const ANIHashTable *ht, const char *key) {
    if (!ht || !key)
        return NULL;
    ani_vt_itr itr = ani_vt_get(&ht->vt, key);
    if (ani_vt_is_end(itr))
        return NULL;
    return itr.data->key;
}

void *ani_ht_delete(ANIHashTable *ht, const char *key) {
    if (!ht || !key)
        return NULL;
    ani_vt_itr itr = ani_vt_get(&ht->vt, key);
    if (ani_vt_is_end(itr))
        return NULL;
    void *prev = itr.data->val;
    (void)ani_vt_erase(&ht->vt, key);
    return prev;
}

uint32_t ani_ht_count(const ANIHashTable *ht) {
    if (!ht)
        return 0;
    return (uint32_t)ani_vt_size(&ht->vt);
}

void ani_ht_foreach(const ANIHashTable *ht, ani_ht_iter_fn fn, void *userdata) {
    if (!ht || !fn)
        return;
    for (ani_vt_itr itr = ani_vt_first(&ht->vt); !ani_vt_is_end(itr); itr = ani_vt_next(itr)) {
        fn(itr.data->key, itr.data->val, userdata);
    }
}

void ani_ht_clear(ANIHashTable *ht) {
    if (!ht)
        return;
    ani_vt_clear(&ht->vt);
}
