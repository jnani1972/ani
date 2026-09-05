#ifndef ANI_LSP_JAVA_LSP_H
#define ANI_LSP_JAVA_LSP_H

/*
 * java_lsp.h — Pure-C Java semantic resolver.
 *
 * Reverse-engineered from JLS §6 (Names) and §15 (Expressions) plus the
 * algorithm shape used by Eclipse JDT-LS / java-language-server (which both
 * delegate to javac.com.sun.source). The goal is parity with what JDT-LS
 * exposes through textDocument/definition + textDocument/references for
 * call-site resolution, *without* shelling out to javac.
 *
 * Mirrors the structure of go_lsp.h / c_lsp.h / php_lsp.h / py_lsp.h.
 *
 * Resolution scheme (single file):
 *   1. Tree-sitter parses Java source into AST.
 *   2. Build a ANITypeRegistry from this file's ANIDefinitions + Java
 *      stdlib (java.lang.*, java.util.*, java.io.*, java.util.function.*,
 *      java.util.stream.*).
 *   3. Walk the AST and for every method_invocation / object_creation /
 *      field_access expression, evaluate the expression's type using
 *      JLS-style scope chains (block → method params → class members →
 *      superclass chain → outer class → import single → import on-demand →
 *      java.lang → same package).
 *   4. Match the textual call (callee name + arity) against the resolved
 *      receiver type's method set, walking superclasses and interfaces.
 *      Best-overload resolution falls back to argument count match.
 *   5. Emit ANIResolvedCall entries with confidence ≥ 0.6 (the LSP floor
 *      enforced by lsp_resolve.h).
 */

#include "type_rep.h"
#include "scope.h"
#include "type_registry.h"
#include "../ani.h"
#include "go_lsp.h" /* ANILSPDef, ANIResolvedCallArray reused across languages */

/* Java `use`-style import kinds. Mirrors PHP's enum with Java semantics. */
enum {
    ANI_JAVA_IMPORT_TYPE = 0,      /* import com.foo.Bar; — type import */
    ANI_JAVA_IMPORT_STATIC = 1,    /* import static com.foo.Bar.method; */
    ANI_JAVA_IMPORT_ON_DEMAND = 2, /* import com.foo.*; — package on-demand */
    ANI_JAVA_IMPORT_STATIC_OD = 3, /* import static com.foo.Bar.*; */
};

/* Per-file resolution context. */
typedef struct {
    ANIArena *arena;
    const char *source;
    int source_len;
    const ANITypeRegistry *registry;
    ANIScope *current_scope;

    /* Java package — the package declared in the file ("com.example"), or
     * empty string for the unnamed package. Stored in dotted form. */
    const char *package_name;

    /* The path-derived module QN for this file (passed in from the caller),
     * used as the QN prefix for types defined here. */
    const char *module_qn;

    /* Import map. Each entry is one of ANI_JAVA_IMPORT_*.
     *   - TYPE:      local_name="Bar",   target_qn="com.foo.Bar"
     *   - STATIC:    local_name="sqrt",  target_qn="java.lang.Math.sqrt"
     *   - ON_DEMAND: local_name="*",     target_qn="com.foo"
     *   - STATIC_OD: local_name="*",     target_qn="java.lang.Math"
     */
    const char **import_local_names;
    const char **import_target_qns;
    int *import_kinds;
    int import_count;
    int import_cap;

    /* Current enclosing context. */
    const char *enclosing_method_qn;    /* QN of the nearest method/ctor */
    const char *enclosing_class_qn;     /* QN of the nearest class/interface/enum */
    const char *enclosing_super_qn;     /* QN of the immediate superclass (NULL ⇒ Object) */
    const char *enclosing_class_short;  /* short name of enclosing class — for "this" + ctor */
    const char **enclosing_class_stack; /* nested-class stack (enclosing_class_qn at each depth) */
    int enclosing_class_depth;
    int enclosing_class_cap;

    /* Output: resolved + diagnostic call edges. */
    ANIResolvedCallArray *resolved_calls;

    /* Recursion guards. */
    int eval_depth;
    int statement_depth;
    int walk_depth; /* java_resolve_calls_in_node self-recursion (AST nesting) */

    /* Debug mode (ANI_LSP_DEBUG env). */
    bool debug;
} JavaLSPContext;

/* ── Initialization / configuration ───────────────────────────────── */

void java_lsp_init(JavaLSPContext *ctx, ANIArena *arena, const char *source, int source_len,
                   const ANITypeRegistry *registry, const char *package_name, const char *module_qn,
                   ANIResolvedCallArray *out);

void java_lsp_add_import(JavaLSPContext *ctx, const char *local_name, const char *target_qn,
                         int kind);

/* ── Walking / resolution ─────────────────────────────────────────── */

void java_lsp_process_file(JavaLSPContext *ctx, TSNode root);

/* Evaluate the type of an arbitrary Java expression node. May return
 * ani_type_unknown(); never returns NULL. */
const ANIType *java_eval_expr_type(JavaLSPContext *ctx, TSNode node);

/* Convert a Java type AST node (type_identifier, generic_type, array_type,
 * scoped_type_identifier, void_type, integral_type, ...) into a ANIType. */
const ANIType *java_parse_type_node(JavaLSPContext *ctx, TSNode node);

/* Process a Java statement, binding any declared variables/parameters into
 * the current scope. */
void java_process_statement(JavaLSPContext *ctx, TSNode node);

/* Resolve a Java type name (bare or qualified) against the current scope
 * (imports + java.lang + same package). Returns the registered FQN or NULL. */
const char *java_resolve_type_name(JavaLSPContext *ctx, const char *name);

/* Lookup a method on a class, walking the super-chain and implemented
 * interfaces. Returns the matched function or NULL. */
const ANIRegisteredFunc *java_lookup_method(JavaLSPContext *ctx, const char *class_qn,
                                            const char *method_name, int arg_count);

/* Lookup a field on a class, walking the super-chain. Returns the field's
 * type, or ani_type_unknown() on miss. */
const ANIType *java_lookup_field_type(JavaLSPContext *ctx, const char *class_qn,
                                      const char *field_name);

/* ── Top-level entry points ───────────────────────────────────────── */

/* Single-file LSP: build registry from file defs + stdlib, walk and resolve. */
void ani_run_java_lsp(ANIArena *arena, ANIFileResult *result, const char *source, int source_len,
                      TSNode root);

/* Cross-file LSP: build registry from defs + stdlib, re-parse if needed,
 * walk and resolve. defs include both local + cross-file definitions. */
void ani_java_register_lsp_defs(ANIArena *arena, ANITypeRegistry *reg, const ANILSPDef *defs,
                                int def_count);

/* Tier 2 (#1669): build the shared JVM cross registry ONCE per run, then
 * resolve each file against it with a small own-module overlay. Skips the
 * per-file registry build that made cross-file LSP O(files x corpus_defs). */
ANITypeRegistry *ani_java_build_cross_registry(ANIArena *arena, ANILSPDef *defs, int def_count);
void ani_run_java_lsp_cross_with_registry(ANIArena *arena, ANIFileResult *result,
                                          const char *source, int source_len, const char *module_qn,
                                          ANITypeRegistry *reg, const char **import_names,
                                          const char **import_qns, int import_count,
                                          TSTree *cached_tree, ANIResolvedCallArray *out);

void ani_run_java_lsp_cross(ANIArena *arena, const char *source, int source_len,
                            const char *module_qn, ANILSPDef *defs, int def_count,
                            const char **import_names, const char **import_qns, int import_count,
                            TSTree *cached_tree, ANIResolvedCallArray *out);

/* Register the Java standard library (java.lang/util/io/etc.) into reg.
 * Implementation lives in generated/java_stdlib_data.c. */
void ani_java_stdlib_register(ANITypeRegistry *reg, ANIArena *arena);

/* ── Batch cross-file LSP ─────────────────────────────────────────── */

typedef struct {
    const char *source;
    int source_len;
    const char *module_qn;
    TSTree *cached_tree;
    ANILSPDef *defs;
    int def_count;
    const char **import_names;
    const char **import_qns;
    int import_count;
} ANIBatchJavaLSPFile;

void ani_batch_java_lsp_cross(ANIArena *arena, ANIBatchJavaLSPFile *files, int file_count,
                              ANIResolvedCallArray *out);

#endif /* ANI_LSP_JAVA_LSP_H */
