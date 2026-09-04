/*
 * test_store_edges.c — Tests for edge CRUD operations.
 *
 * Ported from internal/store/store_test.go (TestEdgeCRUD, TestInsertEdgeBatch,
 * TestFindEdgesByURLPath, etc.)
 */
#include "test_framework.h"
#include <store/store.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Helper: create a store with project + N nodes (A, B, C, ...) */
static ani_store_t *setup_store_with_nodes(int n, int64_t *ids) {
    ani_store_t *s = ani_store_open_memory();
    ani_store_upsert_project(s, "test", "/tmp/test");

    char name[8], qn[32];
    for (int i = 0; i < n; i++) {
        snprintf(name, sizeof(name), "%c", 'A' + i);
        snprintf(qn, sizeof(qn), "test.%c", 'A' + i);
        ani_node_t node = {
            .project = "test", .label = "Function", .name = name, .qualified_name = qn};
        ids[i] = ani_store_upsert_node(s, &node);
    }
    return s;
}

/* ── Edge CRUD ──────────────────────────────────────────────────── */

TEST(store_edge_insert_find) {
    int64_t ids[2];
    ani_store_t *s = setup_store_with_nodes(2, ids);

    /* Insert edge */
    ani_edge_t e = {.project = "test", .source_id = ids[0], .target_id = ids[1], .type = "CALLS"};
    int64_t eid = ani_store_insert_edge(s, &e);
    ASSERT_GT(eid, 0);

    /* Find by source */
    ani_edge_t *edges = NULL;
    int count = 0;
    int rc = ani_store_find_edges_by_source(s, ids[0], &edges, &count);
    ASSERT_EQ(rc, ANI_STORE_OK);
    ASSERT_EQ(count, 1);
    ASSERT_STR_EQ(edges[0].type, "CALLS");
    ASSERT_EQ(edges[0].source_id, ids[0]);
    ASSERT_EQ(edges[0].target_id, ids[1]);
    ani_store_free_edges(edges, count);

    /* Find by target */
    rc = ani_store_find_edges_by_target(s, ids[1], &edges, &count);
    ASSERT_EQ(rc, ANI_STORE_OK);
    ASSERT_EQ(count, 1);
    ani_store_free_edges(edges, count);

    /* Count */
    int ecnt = ani_store_count_edges(s, "test");
    ASSERT_EQ(ecnt, 1);

    ani_store_close(s);
    PASS();
}

TEST(store_edge_dedup) {
    int64_t ids[2];
    ani_store_t *s = setup_store_with_nodes(2, ids);

    /* Insert same edge twice — should not duplicate */
    ani_edge_t e = {.project = "test", .source_id = ids[0], .target_id = ids[1], .type = "CALLS"};
    ani_store_insert_edge(s, &e);
    ani_store_insert_edge(s, &e);

    int ecnt = ani_store_count_edges(s, "test");
    ASSERT_EQ(ecnt, 1);

    ani_store_close(s);
    PASS();
}

/* #768 schema half: the edges UNIQUE constraint must discriminate IMPORTS
 * edges by local_name so two named imports from the same specifier coexist
 * as two rows, while re-inserting the SAME (src,tgt,IMPORTS,local_name)
 * still upserts onto one row. Non-IMPORTS uniqueness is unchanged. */
TEST(store_imports_edge_local_name_coexist) {
    int64_t ids[2];
    ani_store_t *s = setup_store_with_nodes(2, ids);

    ani_edge_t ea = {.project = "test",
                     .source_id = ids[0],
                     .target_id = ids[1],
                     .type = "IMPORTS",
                     .properties_json = "{\"local_name\":\"A\"}"};
    ani_edge_t eb = {.project = "test",
                     .source_id = ids[0],
                     .target_id = ids[1],
                     .type = "IMPORTS",
                     .properties_json = "{\"local_name\":\"B\"}"};

    int64_t ida = ani_store_insert_edge(s, &ea);
    int64_t idb = ani_store_insert_edge(s, &eb);
    ASSERT_GT(ida, 0);
    ASSERT_GT(idb, 0);
    ASSERT_NEQ(ida, idb); /* distinct local_name -> distinct rows */

    /* Same (src,tgt,IMPORTS,local_name) again -> upsert onto the same row. */
    int64_t ida_again = ani_store_insert_edge(s, &ea);
    ASSERT_EQ(ida_again, ida);

    ani_edge_t *edges = NULL;
    int count = 0;
    int rc = ani_store_find_edges_by_type(s, "test", "IMPORTS", &edges, &count);
    ASSERT_EQ(rc, ANI_STORE_OK);
    ASSERT_EQ(count, 2);
    ASSERT_TRUE(strstr(edges[0].properties_json, "\"local_name\":\"A\"") != NULL ||
                strstr(edges[1].properties_json, "\"local_name\":\"A\"") != NULL);
    ASSERT_TRUE(strstr(edges[0].properties_json, "\"local_name\":\"B\"") != NULL ||
                strstr(edges[1].properties_json, "\"local_name\":\"B\"") != NULL);
    ani_store_free_edges(edges, count);

    /* Non-IMPORTS dedup is unchanged: differing properties (no local_name)
     * must still upsert onto one row, not fan out via a NULL discriminator. */
    ani_edge_t c1 = {.project = "test",
                     .source_id = ids[0],
                     .target_id = ids[1],
                     .type = "CALLS",
                     .properties_json = "{}"};
    ani_edge_t c2 = {.project = "test",
                     .source_id = ids[0],
                     .target_id = ids[1],
                     .type = "CALLS",
                     .properties_json = "{\"weight\":5}"};
    int64_t idc1 = ani_store_insert_edge(s, &c1);
    int64_t idc2 = ani_store_insert_edge(s, &c2);
    ASSERT_GT(idc1, 0);
    ASSERT_EQ(idc1, idc2);

    ani_store_close(s);
    PASS();
}

TEST(store_edge_find_by_source_type) {
    int64_t ids[3];
    ani_store_t *s = setup_store_with_nodes(3, ids);

    ani_edge_t e1 = {.project = "test", .source_id = ids[0], .target_id = ids[1], .type = "CALLS"};
    ani_edge_t e2 = {
        .project = "test", .source_id = ids[0], .target_id = ids[2], .type = "IMPORTS"};
    ani_store_insert_edge(s, &e1);
    ani_store_insert_edge(s, &e2);

    ani_edge_t *edges = NULL;
    int count = 0;
    int rc = ani_store_find_edges_by_source_type(s, ids[0], "CALLS", &edges, &count);
    ASSERT_EQ(rc, ANI_STORE_OK);
    ASSERT_EQ(count, 1);
    ASSERT_STR_EQ(edges[0].type, "CALLS");
    ani_store_free_edges(edges, count);

    ani_store_close(s);
    PASS();
}

TEST(store_edge_find_by_target_type) {
    int64_t ids[3];
    ani_store_t *s = setup_store_with_nodes(3, ids);

    ani_edge_t e1 = {.project = "test", .source_id = ids[0], .target_id = ids[2], .type = "CALLS"};
    ani_edge_t e2 = {
        .project = "test", .source_id = ids[1], .target_id = ids[2], .type = "IMPORTS"};
    ani_store_insert_edge(s, &e1);
    ani_store_insert_edge(s, &e2);

    ani_edge_t *edges = NULL;
    int count = 0;
    int rc = ani_store_find_edges_by_target_type(s, ids[2], "CALLS", &edges, &count);
    ASSERT_EQ(rc, ANI_STORE_OK);
    ASSERT_EQ(count, 1);
    ani_store_free_edges(edges, count);

    ani_store_close(s);
    PASS();
}

TEST(store_edge_find_by_type) {
    int64_t ids[3];
    ani_store_t *s = setup_store_with_nodes(3, ids);

    ani_edge_t e1 = {.project = "test", .source_id = ids[0], .target_id = ids[1], .type = "CALLS"};
    ani_edge_t e2 = {.project = "test", .source_id = ids[1], .target_id = ids[2], .type = "CALLS"};
    ani_edge_t e3 = {
        .project = "test", .source_id = ids[0], .target_id = ids[2], .type = "IMPORTS"};
    ani_store_insert_edge(s, &e1);
    ani_store_insert_edge(s, &e2);
    ani_store_insert_edge(s, &e3);

    ani_edge_t *edges = NULL;
    int count = 0;
    int rc = ani_store_find_edges_by_type(s, "test", "CALLS", &edges, &count);
    ASSERT_EQ(rc, ANI_STORE_OK);
    ASSERT_EQ(count, 2);
    ani_store_free_edges(edges, count);

    ani_store_close(s);
    PASS();
}

TEST(store_edge_count_by_type) {
    int64_t ids[3];
    ani_store_t *s = setup_store_with_nodes(3, ids);

    ani_edge_t e1 = {.project = "test", .source_id = ids[0], .target_id = ids[1], .type = "CALLS"};
    ani_edge_t e2 = {.project = "test", .source_id = ids[1], .target_id = ids[2], .type = "CALLS"};
    ani_edge_t e3 = {
        .project = "test", .source_id = ids[0], .target_id = ids[2], .type = "IMPORTS"};
    ani_store_insert_edge(s, &e1);
    ani_store_insert_edge(s, &e2);
    ani_store_insert_edge(s, &e3);

    int cnt = ani_store_count_edges_by_type(s, "test", "CALLS");
    ASSERT_EQ(cnt, 2);

    cnt = ani_store_count_edges_by_type(s, "test", "IMPORTS");
    ASSERT_EQ(cnt, 1);

    cnt = ani_store_count_edges_by_type(s, "test", "NONEXISTENT");
    ASSERT_EQ(cnt, 0);

    ani_store_close(s);
    PASS();
}

TEST(store_edge_delete_by_type) {
    int64_t ids[3];
    ani_store_t *s = setup_store_with_nodes(3, ids);

    ani_edge_t e1 = {.project = "test", .source_id = ids[0], .target_id = ids[1], .type = "CALLS"};
    ani_edge_t e2 = {
        .project = "test", .source_id = ids[0], .target_id = ids[2], .type = "IMPORTS"};
    ani_store_insert_edge(s, &e1);
    ani_store_insert_edge(s, &e2);

    ani_store_delete_edges_by_type(s, "test", "CALLS");
    int ecnt = ani_store_count_edges(s, "test");
    ASSERT_EQ(ecnt, 1);

    ani_store_close(s);
    PASS();
}

/* ── Edge properties ────────────────────────────────────────────── */

TEST(store_edge_properties_json) {
    int64_t ids[2];
    ani_store_t *s = setup_store_with_nodes(2, ids);

    ani_edge_t e = {.project = "test",
                    .source_id = ids[0],
                    .target_id = ids[1],
                    .type = "HTTP_CALLS",
                    .properties_json = "{\"url_path\":\"/api/orders/create\",\"confidence\":0.8}"};
    ani_store_insert_edge(s, &e);

    ani_edge_t *edges = NULL;
    int count = 0;
    ani_store_find_edges_by_source(s, ids[0], &edges, &count);
    ASSERT_EQ(count, 1);
    ASSERT_NOT_NULL(edges[0].properties_json);
    ASSERT(strstr(edges[0].properties_json, "url_path") != NULL);
    ASSERT(strstr(edges[0].properties_json, "/api/orders/create") != NULL);
    ani_store_free_edges(edges, count);

    ani_store_close(s);
    PASS();
}

TEST(store_edge_null_properties) {
    int64_t ids[2];
    ani_store_t *s = setup_store_with_nodes(2, ids);

    ani_edge_t e = {.project = "test",
                    .source_id = ids[0],
                    .target_id = ids[1],
                    .type = "CALLS",
                    .properties_json = NULL};
    ani_store_insert_edge(s, &e);

    ani_edge_t *edges = NULL;
    int count = 0;
    ani_store_find_edges_by_source(s, ids[0], &edges, &count);
    ASSERT_EQ(count, 1);
    ASSERT_NOT_NULL(edges[0].properties_json);
    ASSERT_STR_EQ(edges[0].properties_json, "{}");
    ani_store_free_edges(edges, count);

    ani_store_close(s);
    PASS();
}

/* ── Batch edge insert ──────────────────────────────────────────── */

TEST(store_edge_batch_insert) {
    int64_t ids[10];
    ani_store_t *s = setup_store_with_nodes(10, ids);

    /* Create edges: each node calls the next */
    ani_edge_t edges[9];
    for (int i = 0; i < 9; i++) {
        edges[i] = (ani_edge_t){
            .project = "test", .source_id = ids[i], .target_id = ids[i + 1], .type = "CALLS"};
    }

    int rc = ani_store_insert_edge_batch(s, edges, 9);
    ASSERT_EQ(rc, ANI_STORE_OK);

    int ecnt = ani_store_count_edges(s, "test");
    ASSERT_EQ(ecnt, 9);

    /* Re-insert should not duplicate */
    rc = ani_store_insert_edge_batch(s, edges, 9);
    ASSERT_EQ(rc, ANI_STORE_OK);
    ecnt = ani_store_count_edges(s, "test");
    ASSERT_EQ(ecnt, 9);

    ani_store_close(s);
    PASS();
}

TEST(store_edge_batch_empty) {
    ani_store_t *s = ani_store_open_memory();
    int rc = ani_store_insert_edge_batch(s, NULL, 0);
    ASSERT_EQ(rc, ANI_STORE_OK);
    ani_store_close(s);
    PASS();
}

/* ── Edge cascade on node delete ────────────────────────────────── */

TEST(store_edge_cascade_on_node_delete) {
    int64_t ids[3];
    ani_store_t *s = setup_store_with_nodes(3, ids);

    /* A→B, A→C */
    ani_edge_t e1 = {.project = "test", .source_id = ids[0], .target_id = ids[1], .type = "CALLS"};
    ani_edge_t e2 = {.project = "test", .source_id = ids[0], .target_id = ids[2], .type = "CALLS"};
    ani_store_insert_edge(s, &e1);
    ani_store_insert_edge(s, &e2);

    /* Delete node A — edges should cascade */
    ani_store_delete_nodes_by_project(s, "test");
    int ecnt = ani_store_count_edges(s, "test");
    ASSERT_EQ(ecnt, 0);

    ani_store_close(s);
    PASS();
}

/* ── Batch insert with count=0 (non-NULL array) ────────────────── */

TEST(store_edge_batch_insert_zero_count) {
    ani_store_t *s = ani_store_open_memory();
    ani_store_upsert_project(s, "test", "/tmp/test");

    ani_edge_t dummy = {.project = "test", .source_id = 1, .target_id = 2, .type = "CALLS"};
    int rc = ani_store_insert_edge_batch(s, &dummy, 0);
    ASSERT_EQ(rc, ANI_STORE_OK);

    int ecnt = ani_store_count_edges(s, "test");
    ASSERT_EQ(ecnt, 0);

    ani_store_close(s);
    PASS();
}

/* ── Batch insert stress: 50 edges ─────────────────────────────── */

TEST(store_edge_batch_insert_50) {
    /* Create 51 nodes so we can have 50 A→B edges with distinct targets */
    int64_t ids[51];
    ani_store_t *s = ani_store_open_memory();
    ani_store_upsert_project(s, "test", "/tmp/test");

    for (int i = 0; i < 51; i++) {
        char name[8], qn[32];
        snprintf(name, sizeof(name), "N%d", i);
        snprintf(qn, sizeof(qn), "test.N%d", i);
        ani_node_t node = {
            .project = "test", .label = "Function", .name = name, .qualified_name = qn};
        ids[i] = ani_store_upsert_node(s, &node);
    }

    /* 50 edges: N0→N1, N1→N2, ..., N49→N50 */
    ani_edge_t edges[50];
    for (int i = 0; i < 50; i++) {
        edges[i] = (ani_edge_t){
            .project = "test", .source_id = ids[i], .target_id = ids[i + 1], .type = "CALLS"};
    }

    int rc = ani_store_insert_edge_batch(s, edges, 50);
    ASSERT_EQ(rc, ANI_STORE_OK);

    int ecnt = ani_store_count_edges(s, "test");
    ASSERT_EQ(ecnt, 50);

    ani_store_close(s);
    PASS();
}

/* ── find_edges_by_source with non-existent source ─────────────── */

TEST(store_edge_find_source_nonexistent) {
    ani_store_t *s = ani_store_open_memory();
    ani_store_upsert_project(s, "test", "/tmp/test");

    ani_edge_t *edges = NULL;
    int count = 0;
    int rc = ani_store_find_edges_by_source(s, 999999, &edges, &count);
    ASSERT_EQ(rc, ANI_STORE_OK);
    ASSERT_EQ(count, 0);
    ani_store_free_edges(edges, count);

    ani_store_close(s);
    PASS();
}

/* ── find_edges_by_target with non-existent target ─────────────── */

TEST(store_edge_find_target_nonexistent) {
    ani_store_t *s = ani_store_open_memory();
    ani_store_upsert_project(s, "test", "/tmp/test");

    ani_edge_t *edges = NULL;
    int count = 0;
    int rc = ani_store_find_edges_by_target(s, 999999, &edges, &count);
    ASSERT_EQ(rc, ANI_STORE_OK);
    ASSERT_EQ(count, 0);
    ani_store_free_edges(edges, count);

    ani_store_close(s);
    PASS();
}

/* ── find_edges_by_type for non-existent type ──────────────────── */

TEST(store_edge_find_type_nonexistent) {
    int64_t ids[2];
    ani_store_t *s = setup_store_with_nodes(2, ids);

    ani_edge_t e = {.project = "test", .source_id = ids[0], .target_id = ids[1], .type = "CALLS"};
    ani_store_insert_edge(s, &e);

    ani_edge_t *edges = NULL;
    int count = 0;
    int rc = ani_store_find_edges_by_type(s, "test", "DOES_NOT_EXIST", &edges, &count);
    ASSERT_EQ(rc, ANI_STORE_OK);
    ASSERT_EQ(count, 0);
    ani_store_free_edges(edges, count);

    ani_store_close(s);
    PASS();
}

/* ── count_edges on empty project ──────────────────────────────── */

TEST(store_edge_count_empty_project) {
    ani_store_t *s = ani_store_open_memory();
    ani_store_upsert_project(s, "empty", "/tmp/empty");

    int ecnt = ani_store_count_edges(s, "empty");
    ASSERT_EQ(ecnt, 0);

    ani_store_close(s);
    PASS();
}

/* ── count_edges_by_type for missing type ──────────────────────── */

TEST(store_edge_count_by_type_missing) {
    int64_t ids[2];
    ani_store_t *s = setup_store_with_nodes(2, ids);

    ani_edge_t e = {.project = "test", .source_id = ids[0], .target_id = ids[1], .type = "CALLS"};
    ani_store_insert_edge(s, &e);

    int cnt = ani_store_count_edges_by_type(s, "test", "NEVER_EXISTS");
    ASSERT_EQ(cnt, 0);

    ani_store_close(s);
    PASS();
}

/* ── delete_edges_by_type preserves other types ────────────────── */

TEST(store_edge_delete_by_type_preserves_others) {
    int64_t ids[3];
    ani_store_t *s = setup_store_with_nodes(3, ids);

    ani_edge_t e1 = {.project = "test", .source_id = ids[0], .target_id = ids[1], .type = "CALLS"};
    ani_edge_t e2 = {
        .project = "test", .source_id = ids[0], .target_id = ids[2], .type = "IMPORTS"};
    ani_edge_t e3 = {
        .project = "test", .source_id = ids[1], .target_id = ids[2], .type = "HTTP_CALLS"};
    ani_store_insert_edge(s, &e1);
    ani_store_insert_edge(s, &e2);
    ani_store_insert_edge(s, &e3);
    ASSERT_EQ(ani_store_count_edges(s, "test"), 3);

    /* Delete only CALLS */
    ani_store_delete_edges_by_type(s, "test", "CALLS");
    ASSERT_EQ(ani_store_count_edges(s, "test"), 2);

    /* IMPORTS and HTTP_CALLS still exist */
    ASSERT_EQ(ani_store_count_edges_by_type(s, "test", "IMPORTS"), 1);
    ASSERT_EQ(ani_store_count_edges_by_type(s, "test", "HTTP_CALLS"), 1);
    ASSERT_EQ(ani_store_count_edges_by_type(s, "test", "CALLS"), 0);

    ani_store_close(s);
    PASS();
}

/* ── delete_edges_by_project preserves other projects ──────────── */

TEST(store_edge_delete_by_project_preserves_others) {
    ani_store_t *s = ani_store_open_memory();
    ani_store_upsert_project(s, "alpha", "/tmp/alpha");
    ani_store_upsert_project(s, "beta", "/tmp/beta");

    /* Create nodes in both projects */
    ani_node_t na = {
        .project = "alpha", .label = "Function", .name = "A", .qualified_name = "alpha.A"};
    ani_node_t nb = {
        .project = "alpha", .label = "Function", .name = "B", .qualified_name = "alpha.B"};
    ani_node_t nc = {
        .project = "beta", .label = "Function", .name = "C", .qualified_name = "beta.C"};
    ani_node_t nd = {
        .project = "beta", .label = "Function", .name = "D", .qualified_name = "beta.D"};
    int64_t idA = ani_store_upsert_node(s, &na);
    int64_t idB = ani_store_upsert_node(s, &nb);
    int64_t idC = ani_store_upsert_node(s, &nc);
    int64_t idD = ani_store_upsert_node(s, &nd);

    ani_edge_t e1 = {.project = "alpha", .source_id = idA, .target_id = idB, .type = "CALLS"};
    ani_edge_t e2 = {.project = "beta", .source_id = idC, .target_id = idD, .type = "CALLS"};
    ani_store_insert_edge(s, &e1);
    ani_store_insert_edge(s, &e2);

    ASSERT_EQ(ani_store_count_edges(s, "alpha"), 1);
    ASSERT_EQ(ani_store_count_edges(s, "beta"), 1);

    /* Delete alpha edges only */
    ani_store_delete_edges_by_project(s, "alpha");
    ASSERT_EQ(ani_store_count_edges(s, "alpha"), 0);
    ASSERT_EQ(ani_store_count_edges(s, "beta"), 1);

    ani_store_close(s);
    PASS();
}

/* ── Edge with special chars in properties_json ────────────────── */

TEST(store_edge_properties_special_chars) {
    int64_t ids[2];
    ani_store_t *s = setup_store_with_nodes(2, ids);

    /* JSON with unicode, quotes, backslashes */
    const char *json = "{\"desc\":\"line1\\nline2\",\"path\":\"/api/v1/users?q=foo&bar=baz\","
                       "\"emoji\":\"\\u2603\"}";
    ani_edge_t e = {.project = "test",
                    .source_id = ids[0],
                    .target_id = ids[1],
                    .type = "HTTP_CALLS",
                    .properties_json = json};
    ani_store_insert_edge(s, &e);

    ani_edge_t *edges = NULL;
    int count = 0;
    ani_store_find_edges_by_source(s, ids[0], &edges, &count);
    ASSERT_EQ(count, 1);
    ASSERT_NOT_NULL(edges[0].properties_json);
    /* Round-trip should preserve the JSON string */
    ASSERT(strstr(edges[0].properties_json, "line1\\nline2") != NULL);
    ASSERT(strstr(edges[0].properties_json, "/api/v1/users") != NULL);
    ani_store_free_edges(edges, count);

    ani_store_close(s);
    PASS();
}

/* ── Edge with very long type string ───────────────────────────── */

TEST(store_edge_long_type_string) {
    int64_t ids[2];
    ani_store_t *s = setup_store_with_nodes(2, ids);

    /* 200-char type string */
    char long_type[201];
    memset(long_type, 'X', 200);
    long_type[200] = '\0';

    ani_edge_t e = {.project = "test", .source_id = ids[0], .target_id = ids[1], .type = long_type};
    int64_t eid = ani_store_insert_edge(s, &e);
    ASSERT_GT(eid, 0);

    /* Verify it round-trips */
    ani_edge_t *edges = NULL;
    int count = 0;
    int rc = ani_store_find_edges_by_source(s, ids[0], &edges, &count);
    ASSERT_EQ(rc, ANI_STORE_OK);
    ASSERT_EQ(count, 1);
    ASSERT_STR_EQ(edges[0].type, long_type);
    ani_store_free_edges(edges, count);

    ani_store_close(s);
    PASS();
}

/* ── find_edges_by_source_type with non-existent type ──────────── */

TEST(store_edge_find_source_type_nonexistent) {
    int64_t ids[2];
    ani_store_t *s = setup_store_with_nodes(2, ids);

    ani_edge_t e = {.project = "test", .source_id = ids[0], .target_id = ids[1], .type = "CALLS"};
    ani_store_insert_edge(s, &e);

    ani_edge_t *edges = NULL;
    int count = 0;
    int rc = ani_store_find_edges_by_source_type(s, ids[0], "NOPE", &edges, &count);
    ASSERT_EQ(rc, ANI_STORE_OK);
    ASSERT_EQ(count, 0);
    ani_store_free_edges(edges, count);

    ani_store_close(s);
    PASS();
}

SUITE(store_edges) {
    RUN_TEST(store_edge_insert_find);
    RUN_TEST(store_edge_dedup);
    RUN_TEST(store_imports_edge_local_name_coexist);
    RUN_TEST(store_edge_find_by_source_type);
    RUN_TEST(store_edge_find_by_target_type);
    RUN_TEST(store_edge_find_by_type);
    RUN_TEST(store_edge_count_by_type);
    RUN_TEST(store_edge_delete_by_type);
    RUN_TEST(store_edge_properties_json);
    RUN_TEST(store_edge_null_properties);
    RUN_TEST(store_edge_batch_insert);
    RUN_TEST(store_edge_batch_empty);
    RUN_TEST(store_edge_cascade_on_node_delete);
    /* Edge case tests */
    RUN_TEST(store_edge_batch_insert_zero_count);
    RUN_TEST(store_edge_batch_insert_50);
    RUN_TEST(store_edge_find_source_nonexistent);
    RUN_TEST(store_edge_find_target_nonexistent);
    RUN_TEST(store_edge_find_type_nonexistent);
    RUN_TEST(store_edge_count_empty_project);
    RUN_TEST(store_edge_count_by_type_missing);
    RUN_TEST(store_edge_delete_by_type_preserves_others);
    RUN_TEST(store_edge_delete_by_project_preserves_others);
    RUN_TEST(store_edge_properties_special_chars);
    RUN_TEST(store_edge_long_type_string);
    RUN_TEST(store_edge_find_source_type_nonexistent);
}
