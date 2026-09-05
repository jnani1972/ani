#ifndef ANI_AC_H
#define ANI_AC_H

#include <stdint.h>

// Forward declaration — full struct in ac.c
typedef struct ANIAutomaton ANIAutomaton;

// Input for batch LZ4 scanning.
typedef struct {
    const char *data;
    int compressed_len;
    int original_len;
} ANILz4Entry;

// Output for batch LZ4 scanning.
typedef struct {
    int file_index;
    uint64_t bitmask;
} ANILz4Match;

// Output for batch name scanning.
typedef struct {
    int name_index;
    int pattern_id;
} ANIMatchResult;

// Build an Aho-Corasick automaton from patterns.
ANIAutomaton *ani_ac_build(const char **patterns, const int *lengths, int count,
                           const uint8_t *alpha_map, int alpha_size);
void ani_ac_free(ANIAutomaton *ac);

// Single-text scanning (returns bitmask of matched pattern IDs).
uint64_t ani_ac_scan_bitmask(const ANIAutomaton *ac, const char *text, int text_len);

// LZ4-compressed scanning.
uint64_t ani_ac_scan_lz4_bitmask(const ANIAutomaton *ac, const char *compressed, int compressed_len,
                                 int original_len);
int ani_ac_scan_lz4_batch(const ANIAutomaton *ac, const ANILz4Entry *entries, int num_entries,
                          ANILz4Match *out_matches, int max_matches);

// Batch name scanning.
int ani_ac_scan_batch(const ANIAutomaton *ac, const char *names_buf, const int *name_offsets,
                      const int *name_lengths, int num_names, ANIMatchResult *out_matches,
                      int max_matches);

// Introspection.
int ani_ac_num_states(const ANIAutomaton *ac);
int ani_ac_num_patterns(const ANIAutomaton *ac);
int ani_ac_table_bytes(const ANIAutomaton *ac);

#endif // ANI_AC_H
