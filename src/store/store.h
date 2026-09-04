/*
 * store.h — Opaque SQLite graph store for code knowledge graphs.
 *
 * All functions are prefixed ani_store_*. The store handle is opaque —
 * callers never touch SQLite internals directly.
 *
 * Thread safety: a single store handle must not be used concurrently.
 * Use one store per thread or external synchronization.
 */
#ifndef ANI_STORE_H
#define ANI_STORE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ── Opaque handle ──────────────────────────────────────────────── */

typedef struct ani_store ani_store_t;

/* ── Result codes ───────────────────────────────────────────────── */

#define ANI_STORE_OK 0
#define ANI_STORE_ERR (-1)
#define ANI_STORE_NOT_FOUND (-2)
#define ANI_INDEX_FORMAT_VERSION 1
#define ANI_STORE_CANCELLED (-3)
#define ANI_STORE_SCAN_LIMIT (-4)
#define ANI_STORE_CALLBACK_ERR (-5)

#define ANI_STORE_FILE_OUTLINE_MAX_LIMIT 200
#define ANI_STORE_FILE_OUTLINE_MAX_LABELS 16
#define ANI_STORE_FILE_OUTLINE_MAX_TEXT_BYTES (256U * 1024U)

/* ── Data structures ────────────────────────────────────────────── */

typedef struct {
    int64_t id;
    const char *project;
    const char *label;          /* Function, Class, Method, Module, File, ... */
    const char *name;           /* short name */
    const char *qualified_name; /* full dotted path */
    const char *file_path;      /* relative file path */
    int start_line;
    int end_line;
    const char *properties_json; /* JSON string, NULL → "{}" */
} ani_node_t;

/* Compact declaration row returned by the bounded file-outline query. */
typedef struct {
    const char *name;
    const char *label;
    const char *qualified_name;
    int start_line;
    int end_line;
} ani_file_outline_row_t;

/* Optional cancellation callback for bounded store queries. */
typedef bool (*ani_store_cancel_fn)(void *context);

typedef struct {
    int64_t id;
    const char *project;
    int64_t source_id;
    int64_t target_id;
    const char *type;            /* CALLS, HTTP_CALLS, IMPORTS, ... */
    const char *properties_json; /* JSON string, NULL → "{}" */
} ani_edge_t;

typedef struct {
    const char *name;
    const char *indexed_at; /* ISO 8601 */
    const char *root_path;
} ani_project_t;

typedef struct {
    const char *project;
    const char *rel_path;
    const char *sha256;
    int64_t mtime_ns;
    int64_t size;
} ani_file_hash_t;

/* One file's persisted LSP surface: the serialized cross-file definition set
 * (exactly what pass_lsp_cross registration consumes) plus the metadata the
 * closure-repair incremental route needs to decide and bound its work. The
 * store treats defs_json/ref_bloom as opaque; the codec lives with
 * pass_lsp_cross, which is the only writer and reader of their contents. */
typedef struct {
    const char *project;
    const char *rel_path;
    const char *surface_sha; /* sha256 hex of defs_json (the early-cutoff key) */
    const char *defs_json;   /* canonical JSON array of the file's LSP defs */
    const void *ref_bloom;   /* referenced-identifier bloom blob (may be NULL) */
    int ref_bloom_len;
    const char *config_ctx; /* governing-config context hash ("" = none) */
} ani_lsp_surface_row_t;

/* Find nodes overlapping a line range in a file (excludes Module/Package). */
int ani_store_find_nodes_by_file_overlap(ani_store_t *s, const char *project, const char *file_path,
                                         int start_line, int end_line, ani_node_t **out,
                                         int *count);

/* Find nodes whose qualified_name ends with the given suffix (dot-boundary). */
int ani_store_find_nodes_by_qn_suffix(ani_store_t *s, const char *project, const char *suffix,
                                      ani_node_t **out, int *count);

/* Get CALLS degree of a node (inbound and outbound). */
void ani_store_node_degree(ani_store_t *s, int64_t node_id, int *in_deg, int *out_deg);

/* Get distinct file paths for a project. Caller must free each out[i] and out itself.
 * Returns ANI_STORE_OK or ANI_STORE_ERR. */
int ani_store_list_files(ani_store_t *s, const char *project, char ***out, int *count);

/* Persisted index-format identity. Bump when a change alters the QN scheme
 * or node identity of an already-written graph, so an old DB is routed
 * through the full-reindex path instead of producing a mixed graph.
 * 1 = File QNs keep the file extension (#769). */
int ani_store_get_format_version(ani_store_t *s, int *out);
int ani_store_set_format_version(ani_store_t *s, int version);

/* Get caller/callee names for a node (CALLS/HTTP_CALLS/ASYNC_CALLS edges).
 * Returns 0 on success. Caller must free each out_callers[i]/out_callees[i]
 * and the arrays themselves. */
int ani_store_node_neighbor_names(ani_store_t *s, int64_t node_id, int limit, char ***out_callers,
                                  int *caller_count, char ***out_callees, int *callee_count);

/* Batch count in/out degree for multiple nodes.
 * edge_type: filter by edge type (e.g. "CALLS"), or NULL/"" for all types.
 * out_in[i] and out_out[i] receive the in/out degree for node_ids[i].
 * Returns ANI_STORE_OK or ANI_STORE_ERR. */
int ani_store_batch_count_degrees(ani_store_t *s, const int64_t *node_ids, int id_count,
                                  const char *edge_type, int *out_in, int *out_out);

/* Upsert file hashes in batch. */
int ani_store_upsert_file_hash_batch(ani_store_t *s, const ani_file_hash_t *hashes, int count);

/* ── LSP surface rows (closure-repair incremental) ───────────────
 * Upsert/get/delete are whole-row, keyed (project, rel_path). get returns
 * heap rows released with ani_store_free_lsp_surfaces. A project with no
 * rows returns OK with *count == 0 — callers treat that as "no surface
 * data" and route to a full rebuild, which is also the upgrade path for
 * databases written before this table existed. */
int ani_store_upsert_lsp_surface_batch(ani_store_t *s, const ani_lsp_surface_row_t *rows,
                                       int count);
int ani_store_get_lsp_surfaces(ani_store_t *s, const char *project, ani_lsp_surface_row_t **out,
                               int *count);
int ani_store_delete_lsp_surfaces(ani_store_t *s, const char *project);
void ani_store_free_lsp_surfaces(ani_lsp_surface_row_t *rows, int count);

/* Reverse-dependency lookup for closure-repair routing: the DISTINCT
 * file_paths of nodes with at least one edge INTO a node of any file in
 * target_files, excluding the target files themselves. This is "who consumed
 * these files' definitions" as recorded by the previous generation — served
 * by idx_edges_target + idx_nodes_file, so cost tracks the result size, not
 * the graph size. out gets a malloc'd array of malloc'd strings; free with
 * ani_store_free_dependent_files. */
int ani_store_get_dependent_files(ani_store_t *s, const char *project,
                                  const char *const *target_files, int target_count, char ***out,
                                  int *out_count);
void ani_store_free_dependent_files(char **files, int count);

/* Find edges whose properties contain a url_path matching the keyword. */
int ani_store_find_edges_by_url_path(ani_store_t *s, const char *project, const char *keyword,
                                     ani_edge_t **out, int *count);

/* Restore database from another store (backup API). */
int ani_store_restore_from(ani_store_t *dst, ani_store_t *src);

/* Copy a transactionally-consistent snapshot, including committed WAL frames,
 * from an existing DB into a same-directory staging path. */
int ani_store_backup_path(const char *source_path, const char *staging_path);

/* Seal a staging DB into one self-contained main file before atomic publish.
 * The store must have no concurrent users. */
int ani_store_prepare_for_publish(ani_store_t *s);

/* Checkpoint and detach sidecars from an existing destination immediately
 * before replacement. Fails closed while another process prevents sealing. */
int ani_store_prepare_path_for_replace(const char *path);

/* ── Search ─────────────────────────────────────────────────────── */

typedef struct {
    const char *project;
    const char *label;        /* NULL = any label */
    const char *name_pattern; /* regex on name, NULL = any */
    const char *qn_pattern;   /* regex on qualified_name, NULL = any */
    const char *file_pattern; /* glob on file_path, NULL = any */
    const char *relationship; /* edge type filter, NULL = any */
    const char *direction;    /* "inbound" / "outbound" / "any", NULL = any */
    int min_degree;           /* -1 = no filter (default), 0+ = minimum */
    int max_degree;           /* -1 = no filter (default), 0+ = maximum */
    int limit;                /* 0 = default (10) */
    int offset;
    bool exclude_entry_points;
    bool include_connected;
    const char *sort_by; /* "relevance" / "name" / "degree", NULL = relevance */
    bool case_sensitive;
    const char **exclude_labels; /* NULL-terminated array, or NULL */
} ani_search_params_t;

typedef struct {
    ani_node_t node;
    int in_degree;
    int out_degree;
    /* connected_names: allocated array of strings, count in connected_count */
    const char **connected_names;
    int connected_count;
} ani_search_result_t;

typedef struct {
    ani_search_result_t *results;
    int count;
    int total; /* total before pagination */
} ani_search_output_t;

/* ── Traversal ──────────────────────────────────────────────────── */

typedef struct {
    ani_node_t node;
    int hop; /* BFS depth from root */
} ani_node_hop_t;

typedef struct {
    const char *from_name;
    const char *to_name;
    const char *type;
    double confidence;
    int64_t source_id; /* edge endpoints — let callers match an edge to a hop node */
    int64_t target_id;
    const char *properties_json; /* raw edge properties (carries CALLS arg expressions) */
} ani_edge_info_t;

typedef struct {
    ani_node_t root;
    ani_node_hop_t *visited;
    int visited_count;
    ani_edge_info_t *edges;
    int edge_count;
    /* True when trail expansion hit its recursive-row safety budget. */
    bool truncated;
} ani_traverse_result_t;

/* ── Schema introspection ───────────────────────────────────────── */

typedef struct {
    const char *label;
    int count;
    char **properties; /* distinct property keys for this label (base + JSON) */
    int property_count;
} ani_label_count_t;

typedef struct {
    const char *type;
    int count;
    char **properties; /* distinct property keys for this edge type (base + JSON) */
    int property_count;
} ani_type_count_t;

typedef struct {
    ani_label_count_t *node_labels;
    int node_label_count;
    ani_type_count_t *edge_types;
    int edge_type_count;
    /* relationship patterns like "(Function)-[CALLS]->(Function) [123x]" */
    const char **rel_patterns;
    int rel_pattern_count;
    const char **sample_func_names;
    int sample_func_count;
    const char **sample_class_names;
    int sample_class_count;
    const char **sample_qns;
    int sample_qn_count;
} ani_schema_info_t;

/* ── Graph comparison ──────────────────────────────────────────── */

/* Stable graph identities deliberately exclude the project name. Strings are
 * borrowed from the active SQLite row and remain valid only for the duration
 * of the callback. */
typedef struct {
    const char *qualified_name;
    const char *label;
    const char *file_path;
} ani_graph_node_identity_t;

typedef struct {
    ani_graph_node_identity_t source;
    ani_graph_node_identity_t target;
    const char *type;
    const char *local_name_gen;
} ani_graph_edge_identity_t;

typedef bool (*ani_graph_compare_cancel_fn)(void *context);
typedef bool (*ani_graph_compare_node_fn)(void *context, bool added,
                                          const ani_graph_node_identity_t *node);
typedef bool (*ani_graph_compare_edge_fn)(void *context, bool added,
                                          const ani_graph_edge_identity_t *edge);

#define ANI_GRAPH_COMPARE_GENERATION_SIZE 128
#define ANI_GRAPH_COMPARE_INDEX_MODE_SIZE 32

typedef struct {
    char generation[ANI_GRAPH_COMPARE_GENERATION_SIZE];
    char index_mode[ANI_GRAPH_COMPARE_INDEX_MODE_SIZE];
    int64_t node_count;
    int64_t edge_count;
} ani_graph_compare_project_t;

typedef struct {
    ani_graph_compare_project_t base;
    ani_graph_compare_project_t target;
    uint64_t nodes_added_total;
    uint64_t nodes_removed_total;
    uint64_t edges_added_total;
    uint64_t edges_removed_total;
} ani_graph_compare_result_t;

/* Compare two independently-owned read-only stores using ordered streaming
 * cursors. Both read transactions and both SQLite progress handlers are owned
 * by this call and released on every exit. scan_limit applies independently to
 * the combined node rows and combined edge rows across the two projects. */
int ani_store_compare_graphs(ani_store_t *base_store, const char *base_project,
                             ani_store_t *target_store, const char *target_project,
                             uint64_t scan_limit, ani_graph_compare_cancel_fn cancel,
                             ani_graph_compare_node_fn on_node, ani_graph_compare_edge_fn on_edge,
                             void *context, ani_graph_compare_result_t *out);

#ifdef ANI_ENABLE_TEST_SEAMS
/* Deterministic one-shot comparison fault seams. Values count successful
 * comparison binds/cancellation checks before the injected event; -1 disables
 * the seam. */
void ani_store_compare_test_fail_bind_after(int successful_binds);
void ani_store_compare_test_cancel_after(int successful_checks);
void ani_store_compare_test_cancel_from_progress(bool enabled);
#endif

/* ── Lifecycle ──────────────────────────────────────────────────── */

/* Open an in-memory database (for testing). */
ani_store_t *ani_store_open_memory(void);

/* Open a file-backed database at the given path. Creates if needed. */
ani_store_t *ani_store_open_path(const char *db_path);

/* Open an existing file-backed database read-write without CREATE. Intended
 * for coordinated mutations where a missing/typo path must never materialize
 * a ghost database. Returns NULL when the file does not exist. */
ani_store_t *ani_store_open_path_existing(const char *db_path);

/* Open an existing file-backed database for querying only. Opened READ-ONLY
 * (no SQLITE_OPEN_CREATE, no write pragmas) so queries never mutate the DB and
 * work on a read-only file / filesystem. Returns NULL if the file does not
 * exist — never creates a new .db file. */
ani_store_t *ani_store_open_path_query(const char *db_path);

/* Validate and seal an existing DB for atomic replacement without creating or
 * migrating its schema. Returns OK when sealed, NOT_FOUND when the bytes are
 * definitely corrupt/incompatible and should be quarantined, or ERR when the
 * file is busy/unavailable and must be left in place. */
int ani_store_seal_existing_path_for_replace(const char *db_path);

/* On-disk path of a file-backed store, or NULL for an in-memory (:memory:)
 * store. The returned pointer is owned by the store. */
const char *ani_store_db_path(const ani_store_t *s);

/* Check database integrity. Returns true if the DB passes basic sanity checks
 * (projects table has correct types, no corruption indicators).
 * Returns false if corruption is detected — caller should delete and re-index. */
bool ani_store_check_integrity(ani_store_t *s);
/* Shallow check + PRAGMA quick_check — catches page-level corruption.
 * O(db size); use on rare paths (artifact import), not hot opens. */
bool ani_store_check_integrity_deep(ani_store_t *s);

/* Outcome of a quarantine-grade integrity check. Used to decide whether a DB
 * that failed the cheap open-time check should be quarantined (renamed to
 * .corrupt and rebuilt) or left alone. See ani_store_check_integrity_verdict. */
typedef enum {
    ANI_INTEGRITY_OK = 0,        /* DB is healthy */
    ANI_INTEGRITY_CORRUPT = 1,   /* DB is structurally damaged — safe to quarantine */
    ANI_INTEGRITY_TRANSIENT = 2, /* SQL/busy/IO error — NOT corruption, do NOT quarantine */
} ani_integrity_verdict_t;

/* Full integrity verdict for the quarantine decision path.
 *
 * The plain ani_store_check_integrity() returns a single bool and cannot
 * distinguish "the projects table has 99 rows" (real corruption) from
 * "sqlite3_prepare_v2 returned SQLITE_BUSY because another instance held the
 * writer lock" (a transient lock contention, #1206). Quarantining on the latter
 * is what makes concurrent MCP instances destroy each other's healthy DBs.
 *
 * This function runs the shallow check, then PRAGMA quick_check, and classifies
 * the failure mode so the caller can quarantine ONLY on confirmed corruption.
 * O(db size); use only on the recovery/quarantine path, not hot opens. */
ani_integrity_verdict_t ani_store_check_integrity_verdict(ani_store_t *s);

/* Open database for a named project in the default cache dir. */
ani_store_t *ani_store_open(const char *project);

/* Close the store and free all resources. NULL-safe. */
void ani_store_close(ani_store_t *s);

/* Get the underlying sqlite3 handle (for testing only). */
struct sqlite3 *ani_store_get_db(ani_store_t *s);

/* Get the last error message (static string, valid until next call). */
const char *ani_store_error(ani_store_t *s);

/* ── Transaction ────────────────────────────────────────────────── */

/* Begin a transaction. Returns ANI_STORE_OK on success. */
int ani_store_begin(ani_store_t *s);

/* Commit the current transaction. */
int ani_store_commit(ani_store_t *s);

/* Rollback the current transaction. */
int ani_store_rollback(ani_store_t *s);

/* ── Bulk write optimization ────────────────────────────────────── */

/* Tune pragmas for bulk write throughput (synchronous=OFF, large cache).
 * WAL journal mode is preserved throughout for crash safety. */
int ani_store_begin_bulk(ani_store_t *s);

/* Restore normal pragmas (synchronous=NORMAL, default cache) after bulk writes. */
int ani_store_end_bulk(ani_store_t *s);

/* Drop user indexes for faster bulk inserts. */
int ani_store_drop_indexes(ani_store_t *s);

/* Recreate user indexes after bulk inserts. */
int ani_store_create_indexes(ani_store_t *s);

/* ── WAL / Checkpoint ───────────────────────────────────────────── */

/* Force WAL checkpoint + PRAGMA optimize. */
int ani_store_checkpoint(ani_store_t *s);

/* #1083: the WAL size limit (journal_size_limit) applied to this write
 * connection, in bytes; -1 = unlimited (SQLite default / pre-fix). */
int64_t ani_store_journal_size_limit(ani_store_t *s);

/* Opaque store generation for pagination-cursor staleness detection:
 * "u<db_uid>g<mutation_gen>" — db_uid is minted per DB file, mutation_gen
 * bumps on every index run. "legacy" for DBs predating store_meta. */
int ani_store_generation(ani_store_t *s, char *buf, size_t bufsz);

/* Seal a fully-written staging database before atomic publication.
 * Raises synchronous to FULL, requires an exclusive TRUNCATE checkpoint to
 * complete, then leaves the database in verified DELETE journal mode so the
 * main file is self-contained (no required -wal/-shm sidecars). This is a
 * fail-closed operation: SQLITE_BUSY and an unconfirmed mode transition are
 * errors. The caller must own the staging database exclusively. */
int ani_store_seal_for_atomic_publish(ani_store_t *s);

/* Resolve the mmap_size pragma value applied to on-disk stores from the
 * ANI_SQLITE_MMAP_SIZE environment variable. Defaults to 67108864 (64 MB)
 * when the variable is unset, malformed, or partially numeric. Negative
 * values clamp to 0 (which disables mmap and reverts to read()/pread()
 * I/O — recoverable SQLITE_IOERR instead of SIGBUS when concurrent
 * processes truncate the DB file under live mappings). Exposed for
 * testability. */
int64_t ani_store_resolve_mmap_size(void);

/* ── Dump / Restore ─────────────────────────────────────────────── */

/* Dump in-memory database to a file. */
int ani_store_dump_to_file(ani_store_t *s, const char *dest_path);

/* ── Project CRUD ───────────────────────────────────────────────── */

int ani_store_upsert_project(ani_store_t *s, const char *name, const char *root_path);
int ani_store_get_project(ani_store_t *s, const char *name, ani_project_t *out);
int ani_store_list_projects(ani_store_t *s, ani_project_t **out, int *count);
int ani_store_delete_project(ani_store_t *s, const char *name);

/* ── Node CRUD ──────────────────────────────────────────────────── */

/* Upsert a single node. Returns node ID (>0) or ANI_STORE_ERR. */
int64_t ani_store_upsert_node(ani_store_t *s, const ani_node_t *n);

/* Upsert nodes in batch. out_ids must have room for count entries. */
int ani_store_upsert_node_batch(ani_store_t *s, const ani_node_t *nodes, int count,
                                int64_t *out_ids);

/* Find node by primary key. Returns ANI_STORE_OK or ANI_STORE_NOT_FOUND. */
int ani_store_find_node_by_id(ani_store_t *s, int64_t id, ani_node_t *out);

/* Find node by project + qualified_name. */
int ani_store_find_node_by_qn(ani_store_t *s, const char *project, const char *qn, ani_node_t *out);

/* Find node by qualified_name only (no project filter — QNs are globally unique). */
int ani_store_find_node_by_qn_any(ani_store_t *s, const char *qn, ani_node_t *out);

/* Find all nodes in a project. Returns allocated array, caller frees. */
int ani_store_find_nodes(ani_store_t *s, const char *project, ani_node_t **out, int *count);

/* Find nodes by name (exact match). Returns allocated array, caller frees. */
int ani_store_find_nodes_by_name(ani_store_t *s, const char *project, const char *name,
                                 ani_node_t **out, int *count);

/* Find nodes by name across all projects. Returns allocated array, caller frees. */
int ani_store_find_nodes_by_name_any(ani_store_t *s, const char *name, ani_node_t **out,
                                     int *count);

/* Find nodes by label. */
int ani_store_find_nodes_by_label(ani_store_t *s, const char *project, const char *label,
                                  ani_node_t **out, int *count);

/* Find nodes by file path. */
int ani_store_find_nodes_by_file(ani_store_t *s, const char *project, const char *file_path,
                                 ani_node_t **out, int *count);

/* Return a stable, paginated outline for one exact repository-relative file.
 * File/folder/container nodes are excluded. labels may be NULL when
 * label_count is zero; otherwise labels are exact-match filters. The query is
 * capped by ANI_STORE_FILE_OUTLINE_MAX_LIMIT and a fixed aggregate text-byte
 * budget, and fails without partial rows when cancelled or over budget.
 * total is the exact filtered count before pagination. */
int ani_store_get_file_outline(ani_store_t *s, const char *project, const char *file_path,
                               const char *const *labels, int label_count, int limit, int offset,
                               ani_store_cancel_fn cancel, void *cancel_context,
                               ani_file_outline_row_t **out, int *count, int *total);
void ani_store_free_file_outline(ani_file_outline_row_t *rows, int count);

/* Batch lookup: map qualified names → node IDs.
 * qns[i] is resolved; out_ids[i] receives the ID or 0 if not found.
 * Returns number of QNs actually found, or ANI_STORE_ERR. */
int ani_store_find_node_ids_by_qns(ani_store_t *s, const char *project, const char **qns,
                                   int qn_count, int64_t *out_ids);

/* Count nodes in project. Returns count or ANI_STORE_ERR. */
int ani_store_count_nodes(ani_store_t *s, const char *project);

int ani_store_count_nodes_scoped(ani_store_t *s, const char *project, const char *path);

int ani_store_count_edges_scoped(ani_store_t *s, const char *project, const char *path);

/* True when path is a non-empty scope after normalization (issue #604). */
bool ani_store_arch_path_scoped(const char *path);

/* When scoped, writes normalized directory prefix into norm_out. Returns false if unscoped. */
bool ani_store_normalize_arch_path(const char *path, char *norm_out, size_t norm_sz);

/* True when architecture aspect `name` belongs to the "overview" subset:
 * every aspect EXCEPT the large per-file listing (file_tree). Shared by both
 * aspect gates — want_aspect (store.c) and aspect_wanted (mcp.c) — so the
 * two sites cannot drift. */
bool ani_store_arch_aspect_in_overview(const char *name);

/* Delete all nodes for a project (cascade deletes edges). */
int ani_store_delete_nodes_by_project(ani_store_t *s, const char *project);

/* Delete nodes by file path. */
int ani_store_delete_nodes_by_file(ani_store_t *s, const char *project, const char *file_path);

/* Delete nodes by label. */
int ani_store_delete_nodes_by_label(ani_store_t *s, const char *project, const char *label);

/* ── Edge CRUD ──────────────────────────────────────────────────── */

/* Insert or update edge. Returns edge ID (>0) or ANI_STORE_ERR. */
int64_t ani_store_insert_edge(ani_store_t *s, const ani_edge_t *e);

/* Insert edges in batch. */
int ani_store_insert_edge_batch(ani_store_t *s, const ani_edge_t *edges, int count);

/* Fetch all CALLS edges among Function/Method nodes for a project as parallel
 * (source_id, target_id) arrays (caller frees both). For SCC / cycle analysis.
 * Stops at max_edges and sets *truncated — never a silent cap. Returns
 * ANI_STORE_OK (or _ERR); *count is the number returned. */
int ani_store_fetch_call_edges(ani_store_t *s, const char *project, int max_edges,
                               int64_t **out_src, int64_t **out_tgt, int *count, bool *truncated);

/* Find edges by source node. */
int ani_store_find_edges_by_source(ani_store_t *s, int64_t source_id, ani_edge_t **out, int *count);

/* Find edges by target node. */
int ani_store_find_edges_by_target(ani_store_t *s, int64_t target_id, ani_edge_t **out, int *count);

/* Find edges by source + type. */
int ani_store_find_edges_by_source_type(ani_store_t *s, int64_t source_id, const char *type,
                                        ani_edge_t **out, int *count);

/* Find edges by target + type. */
int ani_store_find_edges_by_target_type(ani_store_t *s, int64_t target_id, const char *type,
                                        ani_edge_t **out, int *count);

/* Find all edges of a type in project. */
int ani_store_find_edges_by_type(ani_store_t *s, const char *project, const char *type,
                                 ani_edge_t **out, int *count);

/* Count all edges in project. */
int ani_store_count_edges(ani_store_t *s, const char *project);

/* Count edges of given type. */
int ani_store_count_edges_by_type(ani_store_t *s, const char *project, const char *type);

/* Delete all edges for a project. */
int ani_store_delete_edges_by_project(ani_store_t *s, const char *project);

/* Delete edges by type. */
int ani_store_delete_edges_by_type(ani_store_t *s, const char *project, const char *type);

/* ── File hash CRUD ─────────────────────────────────────────────── */

int ani_store_upsert_file_hash(ani_store_t *s, const char *project, const char *rel_path,
                               const char *sha256, int64_t mtime_ns, int64_t size);

int ani_store_get_file_hashes(ani_store_t *s, const char *project, ani_file_hash_t **out,
                              int *count);

/* Fetch one exact file-hash record. The returned strings are heap-owned and
 * must be released with ani_store_clear_file_hash(). */
int ani_store_get_file_hash(ani_store_t *s, const char *project, const char *rel_path,
                            ani_file_hash_t *out);

/* Free heap-owned fields in one exact file-hash record and zero it. */
void ani_store_clear_file_hash(ani_file_hash_t *hash);

int ani_store_delete_file_hash(ani_store_t *s, const char *project, const char *rel_path);

int ani_store_delete_file_hashes(ani_store_t *s, const char *project);

/* ── Index coverage (#963) ──────────────────────────────────────── */

/* One best-effort coverage row: a file the indexer could not fully cover.
 * kind "parse_partial" = indexed but the parse tree had ERROR/MISSING regions
 * (detail = 1-based line ranges "12-40,88-90"); skip kinds "read"/"extract"/
 * "oversized" = not indexed at all (detail = reason). Stored in the separate
 * index_coverage table — coverage is metadata ABOUT the graph, never mixed
 * into the graph itself. */
typedef struct {
    const char *rel_path;
    const char *kind;
    const char *detail;
} ani_coverage_row_t;

/* Metadata describing how completely one index run recorded the best-effort
 * coverage signal. `recording_status` is "complete", "truncated", or
 * "unavailable"; it is deliberately separate from hash_records_complete.
 * Strings returned by ani_store_coverage_meta_get are heap-owned. */
typedef struct {
    const char *project;
    const char *generation;
    const char *index_mode;
    const char *recorded_at;
    const char *recording_status;
    int ignored_files_stored;
    int ignored_files_total;
    int coverage_version;
    bool hash_records_complete;
} ani_coverage_meta_t;

/* Replace the project's coverage rows in one transaction, then prune rows for
 * files absent from file_hashes (deleted from the repo). Call AFTER hashes
 * were persisted for the run. */
int ani_store_coverage_replace(ani_store_t *s, const char *project, const ani_coverage_row_t *rows,
                               int count);

/* Replace coverage rows and their run metadata atomically. Passing NULL meta
 * clears any older metadata so it cannot be mistaken for the new row set. */
int ani_store_coverage_replace_ex(ani_store_t *s, const char *project,
                                  const ani_coverage_row_t *rows, int count,
                                  const ani_coverage_meta_t *meta);

/* Fetch all coverage rows (ordered by rel_path). Caller frees via
 * ani_store_free_coverage. */
int ani_store_coverage_get(ani_store_t *s, const char *project, ani_coverage_row_t **out,
                           int *count);

/* Fetch coverage rows for one path. Exact rows are returned together with any
 * not_indexed_dir ancestor that covers the path. */
int ani_store_coverage_get_path(ani_store_t *s, const char *project, const char *rel_path,
                                ani_coverage_row_t **out, int *count);

/* Fetch coverage rows at/below a directory scope, plus a not_indexed_dir
 * ancestor that covers the scope. Prefix matching is segment-boundary safe. */
int ani_store_coverage_get_scope(ani_store_t *s, const char *project, const char *scope,
                                 ani_coverage_row_t **out, int *count);

/* Fetch/free the metadata paired with the current coverage row set. */
int ani_store_coverage_meta_get(ani_store_t *s, const char *project, ani_coverage_meta_t *out);
void ani_store_coverage_meta_clear(ani_coverage_meta_t *meta);

/* Name of the derived miss-graph shadow project ("<project>::missed").
 * ani_store_coverage_replace materializes the coverage rows as a file-
 * structure graph (Project → Folder → File{kind, detail}) under this project
 * name — queryable via the normal cypher path without touching the real
 * project's graph. */
void ani_store_coverage_shadow_project(char *dst, size_t dstsz, const char *project);

void ani_store_free_coverage(ani_coverage_row_t *rows, int count);

/* ── Search ─────────────────────────────────────────────────────── */

int ani_store_search(ani_store_t *s, const ani_search_params_t *params, ani_search_output_t *out);

/* Free a search output's allocated memory. */
void ani_store_search_free(ani_search_output_t *out);

/* ── Traversal ──────────────────────────────────────────────────── */

int ani_store_bfs(ani_store_t *s, int64_t start_id, const char *direction, const char **edge_types,
                  int edge_type_count, int max_depth, int max_results, ani_traverse_result_t *out);

/* Variable-length Cypher traversal with relationship-trail semantics. */
int ani_store_bfs_trail(ani_store_t *s, int64_t start_id, const char *direction,
                        const char **edge_types, int edge_type_count, int max_depth,
                        int max_results, ani_traverse_result_t *out);

/* Multi-source BFS from ALL seed ids at once (one CTE, temp-table anchored).
 * Seeds are EXCLUDED from the result (impact semantics); MIN(hop) across the
 * seed set; canonical (hop,id) order; *truncated set when the max_results
 * memory-safety ceiling was hit (counting is otherwise uncapped). */
int ani_store_bfs_multi(ani_store_t *s, const int64_t *seed_ids, int seed_count,
                        const char *direction, const char **edge_types, int edge_type_count,
                        int max_depth, int max_results, ani_traverse_result_t *out,
                        bool *truncated);

/* Free a traverse result's allocated memory. */
void ani_store_traverse_free(ani_traverse_result_t *out);

/* ── Impact analysis ────────────────────────────────────────────── */

typedef enum {
    ANI_RISK_CRITICAL = 0,
    ANI_RISK_HIGH = 1,
    ANI_RISK_MEDIUM = 2,
    ANI_RISK_LOW = 3,
} ani_risk_level_t;

/* Map BFS hop depth to risk level. */
ani_risk_level_t ani_hop_to_risk(int hop);

/* String representation of risk level. */
const char *ani_risk_label(ani_risk_level_t level);

typedef struct {
    int critical;
    int high;
    int medium;
    int low;
    int total;
    bool has_cross_service;
} ani_impact_summary_t;

/* Build impact summary from visited hops and edges. */
ani_impact_summary_t ani_build_impact_summary(const ani_node_hop_t *hops, int hop_count,
                                              const ani_edge_info_t *edges, int edge_count);

/* Deduplicate BFS hops, keeping minimum hop per node ID.
 * Returns allocated array and count via out params. Caller frees result. */
int ani_deduplicate_hops(const ani_node_hop_t *hops, int hop_count, ani_node_hop_t **out,
                         int *out_count);

/* ── Schema ─────────────────────────────────────────────────────── */

int ani_store_get_schema(ani_store_t *s, const char *project, ani_schema_info_t *out);

/* Like ani_store_get_schema but skips per-label/per-type JSON property-key
 * discovery (json_each scans over every row) — for callers that only need
 * label/type counts, e.g. get_architecture. */
int ani_store_get_schema_counts(ani_store_t *s, const char *project, ani_schema_info_t *out);

int ani_store_get_schema_counts_scoped(ani_store_t *s, const char *project, const char *path,
                                       ani_schema_info_t *out);

/* Free a schema info's allocated memory. */
void ani_store_schema_free(ani_schema_info_t *out);

/* ── Architecture ───────────────────────────────────────────────── */

typedef struct {
    const char *language;
    int file_count;
} ani_language_count_t;

typedef struct {
    const char *name;
    int node_count;
    int fan_in;
    int fan_out;
} ani_package_summary_t;

typedef struct {
    const char *name;
    const char *qualified_name;
    const char *file;
} ani_entry_point_t;

typedef struct {
    const char *method;
    const char *path;
    const char *handler;
} ani_route_info_t;

typedef struct {
    const char *name;
    const char *qualified_name;
    int fan_in;
} ani_hotspot_t;

typedef struct {
    const char *from;
    const char *to;
    int call_count;
} ani_cross_pkg_boundary_t;

typedef struct {
    const char *from;
    const char *to;
    const char *type;
    int count;
} ani_service_link_t;

typedef struct {
    const char *name;
    const char *layer;
    const char *reason;
} ani_package_layer_t;

typedef struct {
    int id;
    const char *label;
    int members;
    double cohesion;
    const char **top_nodes;
    int top_node_count;
    const char **packages;
    int package_count;
    const char **edge_types;
    int edge_type_count;
} ani_cluster_info_t;

typedef struct {
    const char *path;
    const char *type; /* "dir" or "file" */
    int children;
} ani_file_tree_entry_t;

typedef struct {
    /* Pointers first to minimize padding */
    ani_language_count_t *languages;
    ani_package_summary_t *packages;
    ani_entry_point_t *entry_points;
    ani_route_info_t *routes;
    ani_hotspot_t *hotspots;
    ani_cross_pkg_boundary_t *boundaries;
    ani_service_link_t *services;
    ani_package_layer_t *layers;
    ani_cluster_info_t *clusters;
    ani_file_tree_entry_t *file_tree;
    /* Counts after pointers */
    int language_count;
    int package_count;
    int entry_point_count;
    int route_count;
    int hotspot_count;
    int boundary_count;
    int service_count;
    int layer_count;
    int cluster_count;
    int file_tree_count;
} ani_architecture_info_t;

int ani_store_get_architecture(ani_store_t *s, const char *project, const char *path,
                               const char **aspects, int aspect_count,
                               ani_architecture_info_t *out);
void ani_store_architecture_free(ani_architecture_info_t *out);

/* ── ADR (Architecture Decision Record) ────────────────────────── */

#define ANI_ADR_MAX_LENGTH 8000

typedef struct {
    const char *project;
    const char *content;
    const char *created_at;
    const char *updated_at;
} ani_adr_t;

int ani_store_adr_store(ani_store_t *s, const char *project, const char *content);
int ani_store_adr_get(ani_store_t *s, const char *project, ani_adr_t *out);
int ani_store_adr_delete(ani_store_t *s, const char *project);
int ani_store_adr_update_sections(ani_store_t *s, const char *project, const char **keys,
                                  const char **values, int count, ani_adr_t *out);
void ani_store_adr_free(ani_adr_t *adr);

/* ADR section parsing/rendering (pure functions, no store needed) */

enum { PROPS_MAX = 16 };

typedef struct {
    char *keys[PROPS_MAX];
    char *values[PROPS_MAX];
    int count;
} ani_adr_sections_t;

ani_adr_sections_t ani_adr_parse_sections(const char *content);
char *ani_adr_render(const ani_adr_sections_t *sections);
int ani_adr_validate_content(const char *content, char *errbuf, int errbuf_size);
int ani_adr_validate_section_keys(const char **keys, int count, char *errbuf, int errbuf_size);
void ani_adr_sections_free(ani_adr_sections_t *s);

/* ── ADR section headings (the splice model) ────────────────────
 *
 * A section write must not rebuild the document. ani_adr_parse_sections() +
 * ani_adr_render() is a lossy model — it drops everything before the first
 * heading, drops headings it does not recognise, and reorders what is left —
 * so rebuilding from it silently rewrites text nobody asked to change. These
 * functions locate a heading's byte span instead, so a write replaces that
 * span and leaves every other byte of the document exactly as it was.
 *
 * A heading is a line of the form "## NAME" that is NOT inside a fenced code
 * block. NAME is matched EXACTLY, including case: "## Purpose" and
 * "## PURPOSE" are different sections, because folding them would silently
 * merge two blocks the author chose to keep apart. The six canonical names
 * are a convention (see ADR_EMPTY_HINT), not a privilege: any name that
 * round-trips is a section, and none of them gets ordering priority. */
typedef struct {
    const char *name; /* into the source buffer; NOT NUL-terminated */
    int name_len;
    size_t heading_start; /* offset of the first '#' of the heading line */
    size_t body_start;    /* offset just past the heading line's newline */
    size_t body_end;      /* offset of the next heading, or end of document */
} ani_adr_heading_t;

/* Reports every heading in document order. Returns ANI_STORE_ERR without
 * calling `cb` when the document has an unterminated code fence: its structure
 * is ambiguous, and guessing could splice into a code sample. */
int ani_adr_scan_headings(const char *content, void (*cb)(void *ctx, const ani_adr_heading_t *h),
                          void *ctx);

/* ANI_STORE_ERR + message when the document cannot be spliced safely. */
int ani_adr_check_structure(const char *content, char *errbuf, int errbuf_size);

/* Replaces the body of `name`, or appends the section when it is absent.
 * Returns a new document (caller frees), or NULL on bad input, an unterminated
 * fence, or OOM. Bytes outside the replaced span are preserved exactly, and
 * splicing the same name and body twice is byte-identical to doing it once. */
char *ani_adr_splice_section(const char *content, const char *name, const char *body);

/* Rejects names that could not round-trip through a "## NAME" heading. */
int ani_adr_validate_section_name(const char *name, char *errbuf, int errbuf_size);

/* ── Search helpers (exposed for testing) ───────────────────────── */

/* Convert a glob pattern to SQL LIKE pattern. Caller must free result. */
char *ani_glob_to_like(const char *pattern);

/* Extract literal substrings (>= 3 chars) from a regex pattern for LIKE pre-filtering.
 * Bails on alternation (|). Returns count of hints written to out[].
 * Each out[i] is malloc'd — caller must free each string. */
int ani_extract_like_hints(const char *pattern, char **out, int max_out);

/* Prepend (?i) to a regex pattern if not already present.
 * Returns a static buffer — do NOT free. */
const char *ani_ensure_case_insensitive(const char *pattern);

/* Strip leading (?i) from a regex pattern.
 * Returns a static buffer — do NOT free. */
const char *ani_strip_case_flag(const char *pattern);

/* ── Architecture helpers (exposed for testing) ────────────────── */

const char *ani_qn_to_package(const char *qn);
const char *ani_qn_to_top_package(const char *qn);
bool ani_is_test_file_path(const char *fp);
int ani_store_find_architecture_docs(ani_store_t *s, const char *project, char ***out, int *count);

/* ── Community detection (Leiden) ──────────────────────────────── */

typedef struct {
    int64_t src;
    int64_t dst;
} ani_louvain_edge_t;

typedef struct {
    int64_t node_id;
    int community;
} ani_louvain_result_t;

/* Multi-level Leiden community detection (Traag, Waltman & van Eck 2019,
 * arXiv:1810.08473): local moving + refinement + aggregation, repeated until
 * the partition can no longer be coarsened. Refinement guarantees every
 * reported community is internally connected. The resolution parameter
 * controls granularity (higher -> more, smaller communities); 1.0 is standard.
 * Allocates *out (length *out_count == node_count); the caller frees it. */
int ani_leiden(const int64_t *nodes, int node_count, const ani_louvain_edge_t *edges,
               int edge_count, double resolution, ani_louvain_result_t **out, int *out_count);

/* Convenience wrapper: ani_leiden with resolution 1.0. */
int ani_louvain(const int64_t *nodes, int node_count, const ani_louvain_edge_t *edges,
                int edge_count, ani_louvain_result_t **out, int *out_count);

/* ── Memory management helpers ──────────────────────────────────── */

/* Free heap-allocated strings in a stack-allocated node (does NOT free the node itself). */
void ani_node_free_fields(ani_node_t *n);

/* Free heap-allocated strings in a stack-allocated project (does NOT free the project itself). */
void ani_project_free_fields(ani_project_t *p);

/* Free an array of nodes returned by find_nodes_by_* functions. */
void ani_store_free_nodes(ani_node_t *nodes, int count);

/* Free an array of edges returned by find_edges_by_* functions. */
void ani_store_free_edges(ani_edge_t *edges, int count);

/* Free an array of projects. */
void ani_store_free_projects(ani_project_t *projects, int count);

/* Free an array of file hashes. */
void ani_store_free_file_hashes(ani_file_hash_t *hashes, int count);

/* ── Vector search ───────────────────────────────────────────────── */

/* Result from vector similarity search. */
typedef struct {
    int64_t node_id;
    char *name;
    char *qualified_name;
    char *file_path;
    char *label;
    double score;
} ani_vector_result_t;

/* Search for nodes similar to the given query keywords using stored RI vectors.
 * Builds a merged query vector from the keywords, then does cosine scan via
 * the ani_cosine_i8 SQL function joined with the nodes table.
 * Returns results sorted by score DESC. Caller must free with ani_store_free_vector_results. */
int ani_store_vector_search(ani_store_t *s, const char *project, const char **keywords,
                            int keyword_count, int limit, ani_vector_result_t **out,
                            int *out_count);

/* Free vector search results. */
void ani_store_free_vector_results(ani_vector_result_t *results, int count);

/* Count vectors for a project. */
int ani_store_count_vectors(ani_store_t *s, const char *project);

/* Execute an arbitrary SQL statement (pragmas, FTS5 maintenance, etc).
 * Returns ANI_STORE_OK on success. */
int ani_store_exec(ani_store_t *s, const char *sql);

/* Populate nodes_fts from the `nodes` table — the single writer for the BM25
 * index.  Every backfill site routes through it so the column list is decided
 * in exactly ONE place: a hand-written INSERT that names only the identifier
 * columns silently leaves `body` NULL for every node it writes, which looks
 * perfect after a full reindex and de-indexes prose on the warm path.
 *
 *   project == NULL → wholesale rebuild: clears the index, then reindexes
 *                     every node in the database.
 *   project != NULL → incremental: indexes only that project's nodes with
 *                     id > after_id and leaves existing rows untouched.
 *
 * Returns ANI_STORE_OK on success; ANI_STORE_NOT_FOUND when nodes_fts cannot
 * be written at all (FTS5 compiled out — the caller decides whether that is
 * fatal); ANI_STORE_ERR on a genuine write failure. */
int ani_store_fts_rebuild(ani_store_t *s, const char *project, int64_t after_id);

#endif /* ANI_STORE_H */
