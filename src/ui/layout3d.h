/*
 * layout3d.h — 3D force-directed layout with Barnes-Hut octree + LOD.
 *
 * Computes node positions server-side. Provides hierarchical levels:
 *   - Overview: cluster centroids (packages/folders), ~1K-10K nodes
 *   - Detail: individual nodes within a region, up to max_nodes
 *
 * Layout positions are cached in the project's SQLite database.
 */
#ifndef ANI_UI_LAYOUT3D_H
#define ANI_UI_LAYOUT3D_H

#include "store/store.h"
#include <stdbool.h>

/* ── Layout node (output) ─────────────────────────────────────── */

typedef struct {
    int64_t id;
    float x, y, z;
    const char *label; /* "Function", "File", etc. */
    const char *name;  /* display name */
    const char *qualified_name;
    const char *file_path; /* relative file path for tree reconstruction */
    int start_line;        /* 1-based source range (for code snippet / GitHub link) */
    int end_line;
    float size;     /* visual size */
    uint32_t color; /* 0xRRGGBB */
    int in_calls;   /* incoming CALLS-family degree (full graph, not sampled) */
    /* Dead-code classification (string literal, NOT freed):
     * "dead"|"single"|"entry"|"test"|"exported"|"normal"|"structural". */
    const char *status;
} ani_layout_node_t;

/* ── Layout edge (output) ─────────────────────────────────────── */

typedef struct {
    int64_t source;
    int64_t target;
    const char *type; /* "CALLS", "IMPORTS", etc. */
} ani_layout_edge_t;

/* ── Layout result ────────────────────────────────────────────── */

typedef struct {
    ani_layout_node_t *nodes;
    int node_count;
    ani_layout_edge_t *edges;
    int edge_count;
    int total_nodes; /* total in project (may exceed returned) */
} ani_layout_result_t;

/* ── API ──────────────────────────────────────────────────────── */

typedef enum {
    ANI_LAYOUT_OVERVIEW = 0, /* cluster centroids */
    ANI_LAYOUT_DETAIL = 1    /* individual nodes in region */
} ani_layout_level_t;

/* Compute layout for a project.
 * center_node: QN of center (for detail level), NULL for overview
 * radius: hop distance from center (for detail level)
 * max_nodes: cap on returned nodes */
ani_layout_result_t *ani_layout_compute(ani_store_t *store, const char *project,
                                        ani_layout_level_t level, const char *center_node,
                                        int radius, int max_nodes);

/* Free a layout result. */
void ani_layout_free(ani_layout_result_t *result);

/* Serialize layout result to JSON string. Caller must free(). */
char *ani_layout_to_json(const ani_layout_result_t *result);

#endif /* ANI_UI_LAYOUT3D_H */
