/*
 * minhash.h — MinHash fingerprinting + LSH for near-clone detection.
 *
 * Computes K=64 MinHash signatures from AST node-type trigrams using
 * xxHash with distinct seeds.  No external dependencies — xxHash is
 * vendored.  Pure functions — thread-safe, no shared state.
 *
 * Workflow:
 *   1. ani_minhash_compute()  — during AST extraction (per function)
 *   2. ani_minhash_jaccard()  — pairwise similarity (K=64 agreement)
 *   3. ani_lsh_*              — locality-sensitive hashing for O(n)
 *                               candidate generation
 */
#ifndef ANI_MINHASH_H
#define ANI_MINHASH_H

#include <stdint.h>
#include <stdbool.h>

/* Number of hash permutations (seeds).  Larger K = more accurate Jaccard
 * estimate, but more memory per function.  64 gives ±0.12 standard
 * error on Jaccard — sufficient for a 0.95 threshold. */
#define ANI_MINHASH_K 64

/* Minimum number of leaf AST tokens required to compute a fingerprint.
 * Leaf-only counting is language-agnostic: leaf nodes correspond to
 * actual source tokens (identifiers, literals, keywords, operators),
 * not grammar-internal structure that varies across parsers.
 * 30 leaf tokens ≈ BigCloneBench standard of 50 raw source tokens. */
#define ANI_MINHASH_MIN_NODES 30

/* Default Jaccard threshold for SIMILAR_TO edge emission. */
#define ANI_MINHASH_JACCARD_THRESHOLD 0.95

/* Maximum SIMILAR_TO edges per node (prevents utility function explosion). */
#define ANI_MINHASH_MAX_EDGES_PER_NODE 10

/* LSH parameters: b bands × r rows.  Threshold ≈ (1/b)^(1/r). */
#define ANI_LSH_BANDS 32
#define ANI_LSH_ROWS 2

/* ── MinHash fingerprint ─────────────────────────────────────────── */

/* A MinHash signature: K minimum hash values, one per seed. */
typedef struct {
    uint32_t values[ANI_MINHASH_K];
} ani_minhash_t;

/* Opaque tree-sitter node — forward declared to avoid pulling in
 * tree_sitter/api.h from every consumer. */
typedef struct TSNode TSNode;

/* Compute MinHash fingerprint for a function body's AST.
 *
 * Walks the subtree rooted at `func_body`, normalises leaf node types
 * (identifiers → "I", strings → "S", numbers → "N", type annotations
 * → "T"), builds trigrams, and hashes each trigram with K seeds.
 *
 * Returns true and fills `out` on success.
 * Returns false if the body has fewer than ANI_MINHASH_MIN_NODES
 * normalised tokens (fingerprint not meaningful). */
bool ani_minhash_compute(TSNode func_body, const char *source, int language, ani_minhash_t *out);

/* Compute exact Jaccard similarity between two MinHash signatures.
 * Returns value in [0.0, 1.0]. */
double ani_minhash_jaccard(const ani_minhash_t *a, const ani_minhash_t *b);

/* Hex encoding: K uint32 values × 8 hex chars each = 512 chars. */
enum { ANI_MINHASH_HEX_LEN = 512 };

/* Buffer size for hex-encoded fingerprint including NUL. */
enum { ANI_MINHASH_HEX_BUF = 513 };

/* JSON overhead for ,"fp":"..." wrapper (key + quotes + comma + colon). */
enum { ANI_MINHASH_JSON_OVERHEAD = 10 };
void ani_minhash_to_hex(const ani_minhash_t *fp, char *buf, int bufsize);

/* Decode a hex string back to a MinHash signature.
 * Returns true on success. */
bool ani_minhash_from_hex(const char *hex, ani_minhash_t *out);

/* ── LSH index ───────────────────────────────────────────────────── */

/* Opaque LSH index handle. */
typedef struct ani_lsh_index ani_lsh_index_t;

/* Entry stored in the LSH index. */
typedef struct {
    int64_t node_id;
    const ani_minhash_t *fingerprint;
    const char *file_path;      /* for same-file tagging */
    const char *file_ext;       /* for same-language filtering */
    const char *qualified_name; /* canonical pair-ownership tie-break: node ids
                                   vary run-to-run under parallel extraction,
                                   qualified names do not (determinism) */
} ani_lsh_entry_t;

/* Create a new LSH index. */
ani_lsh_index_t *ani_lsh_new(void);

/* Insert an entry into the LSH index. */
void ani_lsh_insert(ani_lsh_index_t *idx, const ani_lsh_entry_t *entry);

/* Query candidates similar to the given fingerprint.
 * Returns candidate entries via `out` (caller does NOT free the array
 * — it is owned by the index).  Sets `count`.
 * NOT thread-safe: uses index-internal result buffer. */
void ani_lsh_query(const ani_lsh_index_t *idx, const ani_minhash_t *fp,
                   const ani_lsh_entry_t ***out, int *count);

/* Thread-safe variant: writes candidates into caller-provided buffer.
 * `out_buf` must have room for at least `out_cap` pointers.
 * Returns the actual candidate count (may exceed out_cap — result is truncated). */
int ani_lsh_query_into(const ani_lsh_index_t *idx, const ani_minhash_t *fp,
                       const ani_lsh_entry_t **out_buf, int out_cap);

/* Free the LSH index and all internal storage. */
void ani_lsh_free(ani_lsh_index_t *idx);

#endif /* ANI_MINHASH_H */
