/*
 * cypher.h — Public API for the Cypher query engine.
 *
 * Provides lexing, parsing, planning, and execution of a subset of
 * Cypher queries against the ani_store graph database.
 *
 * Supported syntax:
 *   MATCH (n:Label)-[:TYPE*1..3]->(m:Label {prop: "val"})
 *   WHERE n.name =~ ".*pattern.*" AND m.label = "Function"
 *   RETURN n.name, COUNT(m) AS cnt ORDER BY cnt DESC LIMIT 10
 */
#ifndef ANI_CYPHER_H
#define ANI_CYPHER_H

#include <stdint.h>
#include <stdbool.h>
#include <store/store.h>

/* ── Token types ────────────────────────────────────────────────── */

typedef enum {
    /* Keywords */
    TOK_MATCH,
    TOK_WHERE,
    TOK_RETURN,
    TOK_ORDER,
    TOK_BY,
    TOK_LIMIT,
    TOK_AND,
    TOK_OR,
    TOK_AS,
    TOK_DISTINCT,
    TOK_COUNT,
    TOK_CONTAINS,
    TOK_STARTS,
    TOK_WITH,
    TOK_NOT,
    TOK_ASC,
    TOK_DESC,
    TOK_NEQ,  /* <> or != */
    TOK_ENDS, /* ENDS (as in ENDS WITH) */
    TOK_IN,
    TOK_IS,
    TOK_NULL_KW, /* NULL keyword */
    TOK_XOR,
    TOK_SKIP,
    TOK_UNION,
    TOK_UNWIND,

    /* Aggregate functions */
    TOK_SUM,
    TOK_AVG,
    TOK_MIN_KW,
    TOK_MAX_KW,
    TOK_COLLECT,

    /* String functions — recognized as keywords */
    TOK_TOLOWER,
    TOK_TOUPPER,
    TOK_TOSTRING,

    /* CASE expression */
    TOK_CASE,
    TOK_WHEN,
    TOK_THEN,
    TOK_ELSE,
    TOK_END,

    /* Recognized-but-unsupported write/admin keywords */
    TOK_CREATE,
    TOK_DELETE,
    TOK_DETACH,
    TOK_SET,
    TOK_REMOVE,
    TOK_MERGE,
    TOK_OPTIONAL,
    TOK_YIELD,
    TOK_CALL,
    TOK_ALL,
    TOK_TRUE,
    TOK_FALSE,
    TOK_EXISTS,
    TOK_MANDATORY,
    TOK_FOREACH,
    TOK_ON,
    TOK_ADD,
    TOK_CONSTRAINT,
    TOK_DO,
    TOK_DROP,
    TOK_FOR,
    TOK_FROM,
    TOK_GRAPH,
    TOK_OF,
    TOK_REQUIRE,
    TOK_SCALAR,
    TOK_UNIQUE,

    /* Symbols */
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_DASH,
    TOK_GT,
    TOK_LT,
    TOK_COLON,
    TOK_DOT,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_STAR,
    TOK_COMMA,
    TOK_EQ,
    TOK_EQTILDE,
    TOK_GTE,
    TOK_LTE,
    TOK_PIPE,
    TOK_DOTDOT,

    /* Literals */
    TOK_IDENT,
    TOK_STRING,
    TOK_NUMBER,

    /* End of input */
    TOK_EOF,

    TOK_COUNT_TYPES /* sentinel for array sizing */
} ani_token_type_t;

typedef struct {
    ani_token_type_t type;
    const char *text; /* owned pointer to token text */
    int pos;          /* byte offset in source */
} ani_token_t;

/* ── Lexer ──────────────────────────────────────────────────────── */

typedef struct {
    ani_token_t *tokens;
    int count;
    int capacity;
    char *error; /* NULL if no error */
} ani_lex_result_t;

/* Tokenize a Cypher query string. Caller must call ani_lex_free(). */
int ani_lex(const char *input, ani_lex_result_t *out);
void ani_lex_free(ani_lex_result_t *r);

/* ── AST ────────────────────────────────────────────────────────── */

/* Inline property filter {key: "value"} */
typedef struct {
    const char *key;
    const char *value;
} ani_prop_filter_t;

/* Node pattern: (variable:Label {props}) */
typedef struct {
    const char *variable; /* NULL if anonymous */
    const char *label;    /* NULL if unlabeled */
    ani_prop_filter_t *props;
    int prop_count;
} ani_node_pattern_t;

/* Relationship pattern: -[:TYPE|TYPE2*min..max]-> */
typedef struct {
    const char *variable; /* NULL if anonymous */
    const char **types;   /* edge type names */
    int type_count;
    const char *direction; /* "outbound", "inbound", "any" */
    int min_hops;          /* default 1 */
    int max_hops;          /* 0 = unbounded */
} ani_rel_pattern_t;

/* A pattern is alternating nodes and relationships:
 * node0 rel0 node1 rel1 node2 ... */
typedef struct {
    ani_node_pattern_t *nodes;
    int node_count;
    ani_rel_pattern_t *rels;
    int rel_count;
} ani_pattern_t;

/* One argument to a multi-argument scalar function (coalesce, substring, ...). */
typedef struct {
    const char *variable; /* variable reference (NULL if a literal) */
    const char *property; /* property of the variable (NULL if whole var / literal) */
    const char *literal;  /* literal string/number text (NULL if a variable ref) */
} ani_func_arg_t;

/* WHERE condition */
typedef struct {
    const char *variable;
    const char *property;
    const char *op; /* "=", "<>", "=~", "CONTAINS", "STARTS WITH", "ENDS WITH",
                       ">", "<", ">=", "<=", "IN", "IS NULL", "IS NOT NULL" */
    const char *value;
    bool negated; /* NOT prefix */
    /* coalesce(var.prop, literal) in WHERE (#874): when set, a missing/empty
     * property value is substituted with this literal before the op runs. */
    const char *coalesce_default;
    const char **in_values; /* IN [...] list */
    int in_value_count;
    /* EXISTS { (var)-[:value]->() } predicate (op=="EXISTS"): `variable` is the
     * anchor, `value` the edge type (NULL = any), `exists_dir` the direction
     * (0 = outbound, 1 = inbound, 2 = any). */
    int exists_dir;
    /* Multi-arg scalar function on the LHS, e.g. coalesce(f.depth, 0) >= 2
     * (#874). NULL func = plain variable/property LHS. */
    const char *func;
    ani_func_arg_t *args;
    int arg_count;
} ani_condition_t;

/* Expression tree for WHERE clause */
typedef enum {
    EXPR_CONDITION, /* leaf: single condition */
    EXPR_AND,
    EXPR_OR,
    EXPR_NOT,
    EXPR_XOR
} ani_expr_type_t;

typedef struct ani_expr ani_expr_t;
struct ani_expr {
    ani_expr_type_t type;
    ani_condition_t cond; /* leaf (EXPR_CONDITION only) */
    ani_expr_t *left;     /* AND/OR/XOR left; NOT child */
    ani_expr_t *right;    /* AND/OR/XOR right; NULL for NOT */
};

typedef struct {
    ani_expr_t *root; /* expression tree (NULL = use legacy conditions) */
    /* Legacy flat model — kept during migration, removed after Phase 2 */
    ani_condition_t *conditions;
    int count;
    const char *op; /* "AND" or "OR" */
} ani_where_clause_t;

/* CASE expression: CASE WHEN expr THEN val [ELSE val] END */
typedef struct {
    ani_expr_t *when_expr; /* condition */
    const char *then_val;  /* result if true */
} ani_case_branch_t;

typedef struct {
    ani_case_branch_t *branches;
    int branch_count;
    const char *else_val; /* NULL if no ELSE */
} ani_case_expr_t;

/* RETURN item */
typedef struct {
    const char *variable;
    const char *property;  /* NULL for whole node */
    const char *alias;     /* NULL if no alias */
    const char *func;      /* "COUNT", "SUM", "AVG", "MIN", "MAX", "COLLECT",
                              "toLower", "toUpper", "toString" or NULL */
    bool distinct;         /* COUNT(DISTINCT x) — count unique values (#239) */
    ani_case_expr_t *kase; /* CASE expression (NULL if not CASE) */
    ani_func_arg_t *args;  /* args for a multi-argument function (NULL if none) */
    int arg_count;
} ani_return_item_t;

/* Upper bound on ORDER BY sort keys. Queries with more keys are rejected at
 * parse time: an unmodeled key must be a loud error, never a silently dropped
 * remainder (#1334 - the unconsumed tail swallowed the LIMIT clause). */
#define ANI_CYPHER_ORDER_KEYS_MAX 8

typedef struct {
    ani_return_item_t *items;
    int count;
    bool distinct;
    bool star; /* RETURN * */
    /* ORDER BY key list, in priority order. Each key is "variable.property",
     * "COUNT(var)" or an alias; direction is per key (Cypher semantics). */
    const char *order_keys[ANI_CYPHER_ORDER_KEYS_MAX];
    bool order_descs[ANI_CYPHER_ORDER_KEYS_MAX]; /* false = ASC (default) */
    int order_key_count;                         /* 0 = no ORDER BY */
    int skip;                                    /* SKIP N, 0 = none */
    int limit;                                   /* 0 = default */
} ani_return_clause_t;

/* Full query AST */
typedef struct ani_query ani_query_t;
struct ani_query {
    ani_pattern_t *patterns; /* array of patterns (first = main MATCH) */
    int pattern_count;
    bool *pattern_optional;              /* pattern_optional[i] = true → OPTIONAL MATCH */
    ani_where_clause_t *where;           /* NULL if no WHERE */
    ani_return_clause_t *with_clause;    /* WITH clause (NULL if none) */
    ani_where_clause_t *post_with_where; /* WHERE after WITH */
    ani_return_clause_t *ret;            /* NULL if no RETURN */
    ani_query_t *union_next;             /* next query in UNION chain (NULL if none) */
    bool union_all;                      /* true = UNION ALL, false = UNION */
    /* UNWIND expr AS var */
    const char *unwind_expr;  /* expression (literal list or var ref) */
    const char *unwind_alias; /* variable name */
};

/* Convenience: access first pattern (backwards compat) */
#define ani_query_pattern(q) ((q)->patterns[0])

/* ── Parser ─────────────────────────────────────────────────────── */

typedef struct {
    ani_query_t *query;
    char *error; /* NULL if no error */
} ani_parse_result_t;

/* Parse tokens into AST. Caller must call ani_parse_free(). */
int ani_parse(const ani_token_t *tokens, int token_count, ani_parse_result_t *out);
void ani_parse_free(ani_parse_result_t *r);

/* ── Executor ───────────────────────────────────────────────────── */

/* Query result: columns + rows */
typedef struct {
    const char **columns;
    int col_count;
    /* rows[row_idx][col_idx] = string value */
    const char ***rows;
    int row_count;
    /* Non-NULL when the query was rejected (e.g. result too large) */
    char *error;
    /* Non-NULL advisory (caller-visible, not an error): e.g. a variable-
     * length hop range was clamped to the engine ceiling (#797) — without
     * this, a clamped expansion is indistinguishable from "no such path". */
    char *warning;
} ani_cypher_result_t;

/* Execute a Cypher query against a store.
 * max_rows: limit on output rows (0 = use virtual ceiling of 100k).
 * project: project name filter (NULL = all projects).
 * Returns -1 on error (check out->error for message). */
int ani_cypher_execute(ani_store_t *store, const char *query, const char *project, int max_rows,
                       ani_cypher_result_t *out);

/* Free a query result. */
void ani_cypher_result_free(ani_cypher_result_t *r);

/* Convenience: lex + parse in one step. */
int ani_cypher_parse(const char *query, ani_query_t **out, char **error);

/* Free a query AST. */
void ani_query_free(ani_query_t *q);

/* Test-only (#601): force the wall-clock execution budget in milliseconds for
 * subsequent queries on the calling thread. 0 = trip on the first hot-loop
 * check; a negative value restores the default budget. */
void ani_cypher_test_set_deadline_ms(int64_t budget_ms);

/* Worst-case binding slot count for a node cross-join. Computes the count in
 * size_t and rejects any that would not fit the int binding counter or would
 * overflow the size_t byte size; returns 0 and writes *out_n on success,
 * ANI_NOT_FOUND on overflow. Exposed for arithmetic-boundary unit tests. */
int ani_cypher_cross_join_alloc(int bind_count, int extra_count, bool opt, size_t *out_n);

#endif /* ANI_CYPHER_H */
