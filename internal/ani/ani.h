#ifndef ANI_H
#define ANI_H

#include <stdint.h>
#include <stdbool.h>
#include "arena.h"
#include "tree_sitter/api.h"

// Language enum mirrors lang.Language in Go.
// Order must match lang_specs.c tables.
typedef enum {
    ANI_LANG_GO = 0,
    ANI_LANG_PYTHON,
    ANI_LANG_JAVASCRIPT,
    ANI_LANG_TYPESCRIPT,
    ANI_LANG_TSX,
    ANI_LANG_RUST,
    ANI_LANG_JAVA,
    ANI_LANG_CPP,
    ANI_LANG_CSHARP,
    ANI_LANG_PHP,
    ANI_LANG_LUA,
    ANI_LANG_SCALA,
    ANI_LANG_KOTLIN,
    ANI_LANG_RUBY,
    ANI_LANG_C,
    ANI_LANG_BASH,
    ANI_LANG_ZIG,
    ANI_LANG_ELIXIR,
    ANI_LANG_HASKELL,
    ANI_LANG_OCAML,
    ANI_LANG_OBJC,
    ANI_LANG_SWIFT,
    ANI_LANG_DART,
    ANI_LANG_PERL,
    ANI_LANG_GROOVY,
    ANI_LANG_ERLANG,
    ANI_LANG_R,
    ANI_LANG_HTML,
    ANI_LANG_CSS,
    ANI_LANG_SCSS,
    ANI_LANG_YAML,
    ANI_LANG_TOML,
    ANI_LANG_HCL,
    ANI_LANG_SQL,
    ANI_LANG_DOCKERFILE,
    // New languages (v0.5 expansion)
    ANI_LANG_CLOJURE,
    ANI_LANG_FSHARP,
    ANI_LANG_JULIA,
    ANI_LANG_VIMSCRIPT,
    ANI_LANG_NIX,
    ANI_LANG_COMMONLISP,
    ANI_LANG_ELM,
    ANI_LANG_FORTRAN,
    ANI_LANG_CUDA,
    ANI_LANG_COBOL,
    ANI_LANG_VERILOG,
    ANI_LANG_EMACSLISP,
    ANI_LANG_JSON,
    ANI_LANG_XML,
    ANI_LANG_MARKDOWN,
    ANI_LANG_MAKEFILE,
    ANI_LANG_CMAKE,
    ANI_LANG_PROTOBUF,
    ANI_LANG_GRAPHQL,
    ANI_LANG_VUE,
    ANI_LANG_SVELTE,
    ANI_LANG_MESON,
    ANI_LANG_GLSL,
    ANI_LANG_INI,
    // Scientific/math languages
    ANI_LANG_MATLAB,
    ANI_LANG_LEAN,
    ANI_LANG_FORM,
    ANI_LANG_MAGMA,
    ANI_LANG_WOLFRAM,
    ANI_LANG_SOLIDITY,
    ANI_LANG_TYPST,
    ANI_LANG_GDSCRIPT,
    ANI_LANG_GLEAM,
    ANI_LANG_POWERSHELL,
    ANI_LANG_PASCAL,
    ANI_LANG_DLANG,
    ANI_LANG_NIM,
    ANI_LANG_SCHEME,
    ANI_LANG_FENNEL,
    ANI_LANG_FISH,
    ANI_LANG_AWK,
    ANI_LANG_ZSH,
    ANI_LANG_TCL,
    ANI_LANG_ADA,
    ANI_LANG_AGDA,
    ANI_LANG_RACKET,
    ANI_LANG_ODIN,
    ANI_LANG_RESCRIPT,
    ANI_LANG_PURESCRIPT,
    ANI_LANG_NICKEL,
    ANI_LANG_CRYSTAL,
    ANI_LANG_TEAL,
    ANI_LANG_HARE,
    ANI_LANG_PONY,
    ANI_LANG_LUAU,
    ANI_LANG_JANET,
    ANI_LANG_SWAY,
    ANI_LANG_NASM,
    ANI_LANG_ASSEMBLY,
    ANI_LANG_ASTRO,
    ANI_LANG_BLADE,
    ANI_LANG_JUST,
    ANI_LANG_GOTEMPLATE,
    ANI_LANG_TEMPL,
    ANI_LANG_LIQUID,
    ANI_LANG_JINJA2,
    ANI_LANG_PRISMA,
    ANI_LANG_HYPRLANG,
    ANI_LANG_DOTENV,
    ANI_LANG_DIFF,
    ANI_LANG_WGSL,
    ANI_LANG_KDL,
    ANI_LANG_JSON5,
    ANI_LANG_JSONNET,
    ANI_LANG_RON,
    ANI_LANG_THRIFT,
    ANI_LANG_CAPNP,
    ANI_LANG_PROPERTIES,
    ANI_LANG_SSHCONFIG,
    ANI_LANG_BIBTEX,
    ANI_LANG_STARLARK,
    ANI_LANG_BICEP,
    ANI_LANG_CSV,
    ANI_LANG_REQUIREMENTS,
    ANI_LANG_HLSL,
    ANI_LANG_VHDL,
    ANI_LANG_SYSTEMVERILOG,
    ANI_LANG_DEVICETREE,
    ANI_LANG_LINKERSCRIPT,
    ANI_LANG_GN,
    ANI_LANG_KCONFIG,
    ANI_LANG_BITBAKE,
    ANI_LANG_SMALI,
    ANI_LANG_TABLEGEN,
    ANI_LANG_ISPC,
    ANI_LANG_CAIRO,
    ANI_LANG_MOVE,
    ANI_LANG_SQUIRREL,
    ANI_LANG_FUNC,
    ANI_LANG_REGEX,
    ANI_LANG_JSDOC,
    ANI_LANG_RST,
    ANI_LANG_BEANCOUNT,
    ANI_LANG_MERMAID,
    ANI_LANG_PUPPET,
    ANI_LANG_PO,
    ANI_LANG_GITATTRIBUTES,
    ANI_LANG_GITIGNORE,
    ANI_LANG_SLANG,
    ANI_LANG_LLVM_IR,
    ANI_LANG_SMITHY,
    ANI_LANG_WIT,
    ANI_LANG_TLAPLUS,
    ANI_LANG_PKL,
    ANI_LANG_GOMOD,
    ANI_LANG_APEX,
    ANI_LANG_SOQL,
    ANI_LANG_SOSL,
    ANI_LANG_KUSTOMIZE,            // kustomization.yaml — Kubernetes overlay tool
    ANI_LANG_K8S,                  // Generic Kubernetes manifest (apiVersion: detected)
    ANI_LANG_PINE,                 // Pine Script (TradingView indicator / strategy language)
    ANI_LANG_QML,                  // Qt QML (Qt Modeling Language — declarative UI + embedded JS)
    ANI_LANG_CFSCRIPT,             // CFML script dialect (.cfc components — Lucee/ColdFusion)
    ANI_LANG_CFML,                 // CFML tag dialect (.cfm templates — Lucee/ColdFusion)
    ANI_LANG_MOJO,                 // Mojo
    ANI_LANG_OBJECTSCRIPT_UDL,     // InterSystems ObjectScript UDL (.cls class files)
    ANI_LANG_OBJECTSCRIPT_ROUTINE, // InterSystems ObjectScript routine (.mac/.int/.rtn/.inc)
    ANI_LANG_OBJECTSCRIPT_EXPORT,  // InterSystems Studio Export XML (<Export generator="Cache">)
    ANI_LANG_ARKTS,    // ArkTS (HarmonyOS/OpenHarmony .ets — TypeScript superset + ArkUI)
    ANI_LANG_PLSQL,    // Oracle PL/SQL
    ANI_LANG_CHIALISP, // Chialisp (.clsp/.clib/.clinc — Chia smart-coin s-expression language)
    ANI_LANG_COUNT
} ANILanguage;

// --- Extraction result structs ---

typedef struct {
    const char *name;           // short name
    const char *qualified_name; // project.path.name
    const char *label;          // "Function", "Method", "Class", "Variable", "Module"
    const char *file_path;      // relative path
    uint32_t start_line;
    uint32_t end_line;
    const char *signature;              // parameter text (NULL if none)
    const char *return_type;            // return type text (NULL if none)
    const char *receiver;               // Go method receiver (NULL if none)
    const char *docstring;              // leading doc comment (NULL if none)
    const char *parent_class;           // enclosing class QN for methods (NULL if none)
    const char **decorators;            // NULL-terminated array (NULL if none)
    const char **base_classes;          // NULL-terminated array (NULL if none)
    const char **param_names;           // NULL-terminated array (NULL if none)
    const char **param_types;           // NULL-terminated array (NULL if none)
    const char **signature_param_types; // ordered internal signature types; "?" means unknown
    int signature_param_count;          // number of entries in signature_param_types
    const char **return_types;          // NULL-terminated array (NULL if none)
    const char *route_path;   // HTTP route path from decorator (e.g., "/api/users") or NULL
    const char *route_method; // HTTP method from decorator (e.g., "POST") or NULL
    int complexity;           // cyclomatic complexity
    int cognitive;            // cognitive complexity (nesting-weighted)
    int loop_count;           // number of loop constructs in the body
    int loop_depth;           // max nested-loop depth (bottleneck proxy)
    bool is_recursive;        // body contains a direct self-call (seed for "recursive")
    int param_count;          // number of parameters (large = complexity smell)
    int max_access_depth;     // deepest chained member/subscript access (a.b.c.d)
    int linear_scan_in_loop;  // count of linear-scan calls (find/contains/indexOf) inside loops
    int alloc_in_loop;        // count of allocation/append calls inside loops
    bool recursion_in_loop;   // a self-call occurs inside a loop body
    bool unguarded_recursion; // recursive with no self-call guarded by a conditional
    int lines;                // body line count
    uint32_t *fingerprint;    // MinHash fingerprint (arena-allocated, K values) or NULL
    int fingerprint_k;        // number of hash values (ANI_MINHASH_K or 0)
    bool is_exported;
    bool is_abstract;
    bool is_test;
    bool is_entry_point;
    const char *structural_profile; // AST structural profile (arena-allocated) or NULL
    const char *body_tokens; // space-separated raw identifier tokens from body (arena) or NULL
    /* Rust only: raw trait path from the exact `impl Trait for Type` block
     * that declared this method.  Kept at the tail so zero-initialised
     * callers in every other language remain ABI/source compatible. */
    const char *impl_trait;
} ANIDefinition;

/* Argument captured from a call expression */
typedef struct {
    const char *expr;    // raw expression text ("payload.info", "MY_URL", "'hello'")
    const char *value;   // resolved string value or NULL (constant propagation)
    const char *keyword; // keyword name if keyword arg ("url", "topic_id"), NULL if positional
    int index;           // positional index (0-based)
} ANICallArg;

#define ANI_MAX_CALL_ARGS 8

/* Byte offsets are meaningful only within the source buffer that produced
 * them. C/C++/CUDA run both raw and preprocessed extraction passes, and those
 * buffers can contain unrelated occurrences at the same numeric span. */
typedef enum {
    ANI_SOURCE_ORIGIN_RAW = 0,
    ANI_SOURCE_ORIGIN_PREPROCESSED,
} ANISourceOrigin;

typedef struct {
    const char *callee_name;            // raw callee text ("pkg.Func", "foo")
    const char *enclosing_func_qn;      // QN of enclosing function (or module QN)
    const char *first_string_arg;       // first string literal argument (URL, topic, key) or NULL
    const char *second_arg_name;        // second argument identifier (handler ref) or NULL
    ANICallArg args[ANI_MAX_CALL_ARGS]; // first N arguments with expressions
    int arg_count;                      // number of captured arguments
    int loop_depth;                     // enclosing loop nesting at the call site
    int branch_depth;                   // enclosing branch nesting at the call site
    int start_line;                     // 1-based source line of the call (for def range-match)
    uint32_t site_start_byte;           // exact AST occurrence span; end > start when present
    uint32_t site_end_byte;             // exclusive byte offset in the source file
    ANISourceOrigin source_origin;      // raw source or C-family preprocessed buffer
    bool is_method;                     // method/member call with an UNRESOLVED receiver. Perl:
                                        // arrow/method call ($obj->m). TS/JS/TSX: member call
                                        // x.foo() whose receiver is not this/super. Python:
                                        // x.foo() where x is not self/cls/super() and is not
                                        // rooted in an imported name. Read by the weak-member
                                        // guard and by the pxc synthetic-carrier dedup key in
                                        // pass_lsp_cross.c. Default false.
    bool requires_lsp_resolution;       // synthetic semantic candidate (for example an implicit
                                        // C++ operator). Never fall back to textual resolution.
    bool callee_is_locally_bound;       // bare call foo() whose callee identifier is bound as a
                                        // parameter of an enclosing function, so it cannot be the
                                        // module-level foo. Python only today. Read by the
                                        // weak-local-binding guard. Default false.
} ANICall;

typedef struct {
    const char *local_name;  // local alias or name
    const char *module_path; // resolved module path / QN
} ANIImport;

typedef enum {
    ANI_USAGE_VALUE = 0,
    ANI_USAGE_CALL_REFERENCE,
} ANIUsageKind;

typedef struct {
    const char *ref_name;            // referenced identifier
    const char *enclosing_func_qn;   // QN of enclosing function (or module QN)
    ANIUsageKind kind;               // ordinary USAGE or explicit callable reference
    bool may_be_call_reference;      // syntactic candidate; exact LSP proof may upgrade its edge
    bool semantic_reference_blocked; // lexical evidence blocks only unproven textual fallback
    bool semantic_reference_local_shadow; // blocker belongs to a non-module lexical scope
    uint32_t lexical_scope_id;            // extraction-local scope instance; never graph identity
    uint32_t site_start_byte;             // exact reference-token span; end > start when present
    uint32_t site_end_byte;               // exclusive byte offset in the source file
    ANISourceOrigin source_origin;        // raw source or C-family preprocessed buffer
    bool is_member_access;                // token is the member half of a selector/attribute
                                          // (Go x.f — field_identifier). The extractor strips
                                          // the receiver, so this is the only surviving record
                                          // of selector shape (#1962). Default false.
} ANIUsage;

typedef struct {
    const char *exception_name;    // exception class/type name
    const char *enclosing_func_qn; // QN of enclosing function
} ANIThrow;

typedef struct {
    const char *var_name;          // variable name
    const char *enclosing_func_qn; // QN of enclosing function
    bool is_write;                 // true = write, false = read
    bool is_member_access;         // var_name is the field half of a selector/member LHS
                                   // (`t.err = x` → "err"); the receiver is stripped here,
                                   // so this is the only record of selector shape (#1962)
} ANIReadWrite;

typedef struct {
    const char *type_name;         // referenced type/class name
    const char *enclosing_func_qn; // QN of enclosing function
} ANITypeRef;

typedef struct {
    const char *env_key;           // environment variable key
    const char *enclosing_func_qn; // QN of enclosing function
} ANIEnvAccess;

typedef struct {
    const char *var_name;          // variable being assigned
    const char *type_name;         // class/type name of RHS constructor
    const char *enclosing_func_qn; // QN of enclosing function
} ANITypeAssign;

// String reference: URL, config key, or async target found in source.
// Extracted from string literals during AST walk.
typedef enum {
    ANI_STRREF_URL = 0,    // REST path or full URL
    ANI_STRREF_CONFIG = 1, // config file path or env var key
} ANIStringRefKind;

typedef struct {
    const char *value;             // the string literal content
    const char *enclosing_func_qn; // QN of enclosing function
    const char *key_path;          // dotted key path from YAML/JSON nesting (NULL if flat)
    ANIStringRefKind kind;         // URL, CONFIG
} ANIStringRef;

/* Infrastructure binding: topic/queue → endpoint URL.
 * Extracted from YAML/HCL/JSON subscription/scheduler configs.
 * Used by pass_route_nodes to connect async Route nodes to handler services. */
typedef struct {
    const char *source_name; // topic, queue, or schedule name
    const char *target_url;  // push_endpoint, uri, or http_target URL
    const char *broker;      // "pubsub", "cloud_tasks", "cloud_scheduler", "sqs", "kafka"
} ANIInfraBinding;

/* Pub/sub channel participation.  One record per emit() or on()/addListener()
 * call detected in source — the receiver (e.g. Socket.IO client, EventEmitter
 * instance) is intentionally NOT identified; matching is by channel_name
 * across files, which captures the common pattern of one logical bus per
 * service.  Transport disambiguates Socket.IO vs EventEmitter vs future
 * detectors (Kafka, Cloud Pub/Sub, etc.). */
typedef enum {
    ANI_CHANNEL_EMIT = 0,
    ANI_CHANNEL_LISTEN = 1,
} ANIChannelDirection;

typedef struct {
    const char *channel_name;      // literal channel name (e.g. "user.created")
    const char *transport;         // "socketio", "event_emitter", ...
    const char *enclosing_func_qn; // QN of the function containing the emit/on call
    ANIChannelDirection direction;
} ANIChannel;

// Rust: impl Trait for Struct
typedef struct {
    const char *trait_name;  // trait name (raw text)
    const char *struct_name; // struct/type name (raw text)
    /* Exact extracted QN of the implementing type.  Unlike struct_name this
     * does not need a later leaf-name guess, and the relation exists even for
     * an empty `impl Trait for Type {}` block. */
    const char *struct_qn;
} ANIImplTrait;

typedef enum {
    ANI_RESOLVED_INVOCATION = 0,
    ANI_RESOLVED_CALL_REFERENCE,
} ANIResolvedKind;

// LSP-resolved invocation/reference: high-confidence type-aware resolution.
typedef struct {
    const char *caller_qn;         // enclosing function QN
    const char *callee_qn;         // resolved target QN (fully qualified)
    const char *strategy;          // "lsp_type_dispatch", "lsp_direct", etc.
    float confidence;              // 0.90-0.95
    const char *reason;            // diagnostic label for unresolved calls (NULL if resolved)
    ANIResolvedKind kind;          // invocation (CALLS) or explicit callable reference
    uint32_t site_start_byte;      // exact source occurrence; end > start when present
    uint32_t site_end_byte;        // exclusive byte offset in the source file
    ANISourceOrigin source_origin; // raw source or C-family preprocessed buffer
} ANIResolvedCall;

typedef struct {
    ANIResolvedCall *items;
    int count;
    int cap;
} ANIResolvedCallArray;

// Growable arrays used during extraction.
typedef struct {
    ANIDefinition *items;
    int count;
    int cap;
} ANIDefArray;

typedef struct {
    ANICall *items;
    int count;
    int cap;
} ANICallArray;

typedef struct {
    ANIImport *items;
    int count;
    int cap;
} ANIImportArray;

typedef struct {
    ANIUsage *items;
    int count;
    int cap;
} ANIUsageArray;

typedef struct {
    ANIThrow *items;
    int count;
    int cap;
} ANIThrowArray;

typedef struct {
    ANIReadWrite *items;
    int count;
    int cap;
} ANIRWArray;

typedef struct {
    ANITypeRef *items;
    int count;
    int cap;
} ANITypeRefArray;

typedef struct {
    ANIEnvAccess *items;
    int count;
    int cap;
} ANIEnvAccessArray;

typedef struct {
    ANITypeAssign *items;
    int count;
    int cap;
} ANITypeAssignArray;

typedef struct {
    ANIStringRef *items;
    int count;
    int cap;
} ANIStringRefArray;

typedef struct {
    ANIInfraBinding *items;
    int count;
    int cap;
} ANIInfraBindingArray;

typedef struct {
    ANIImplTrait *items;
    int count;
    int cap;
} ANIImplTraitArray;

typedef struct {
    ANIChannel *items;
    int count;
    int cap;
} ANIChannelArray;

// Full extraction result for one file.
typedef struct ANIFileResult {
    ANIArena arena; // owns local memory; composites may also retain child arenas below

    ANIDefArray defs;
    ANICallArray calls;
    ANIImportArray imports;
    ANIUsageArray usages;
    ANIThrowArray throws;
    ANIRWArray rw;
    ANITypeRefArray type_refs;
    ANIEnvAccessArray env_accesses;
    ANITypeAssignArray type_assigns;
    ANIImplTraitArray impl_traits;       // Rust: impl Trait for Struct pairs
    ANIResolvedCallArray resolved_calls; // LSP-resolved invocations/references (high confidence)
    ANIStringRefArray string_refs;       // URL/config string literals from AST
    ANIInfraBindingArray infra_bindings; // topic→URL pairs from IaC configs
    ANIChannelArray channels;            // Socket.IO / EventEmitter pub/sub participation

    const char *module_qn;      // module qualified name
    const char *namespace_name; // declared namespace/package (Java/Kotlin/C#/PHP), NULL if none
    const char **exports;       // NULL-terminated (NULL if none)
    const char **constants;     // NULL-terminated (NULL if none)
    const char **global_vars;   // NULL-terminated (NULL if none)
    const char **macros;        // NULL-terminated, C/C++ only (NULL if none)

    bool has_error;
    const char *error_msg;
    /* Best-effort parse-coverage signal (experimental). parse_incomplete is true
     * when the parse tree contains tree-sitter ERROR/MISSING nodes — constructs
     * in those regions are silently absent from the graph. error_ranges is a
     * compact "start-end,start-end" list of 1-based line ranges (arena-owned) or
     * NULL. This only marks what we can DETECT: the absence of a flag is NOT a
     * completeness guarantee. Callers should treat a flagged file as "prefer
     * grep here", never treat an unflagged file as provably complete. */
    bool parse_incomplete;
    const char *error_ranges;
    int error_region_count;
    bool is_test_file;
    int imports_count;
    TSTree *cached_tree;     // retained parse tree (caller frees via ani_free_tree)
    ANILanguage cached_lang; // language of cached tree (for parser selection)

    // Retained source bytes — copied into `arena` by the parallel
    // extract pass so the fused cross-file LSP step in resolve_worker
    // can run without re-reading the file from disk. NULL when the
    // file exceeded the per-file (100 MB) or total (2 GB) retention
    // cap; in that case the cross-file LSP step is skipped for this
    // file (defs/calls already extracted are unaffected).
    const char *source;
    int source_len;

    // Composite extraction results (currently ObjectScript Studio Export)
    // retain their per-unit results so shallow-copied carrier strings remain
    // valid for the composite's full lifetime. Owned and recursively released
    // by ani_free_result(); ordinary single-file results leave these zeroed.
    struct ANIFileResult **owned_results;
    int owned_result_count;
} ANIFileResult;

// --- Enclosing function cache ---
// Avoids repeated parent-chain walks for nodes within the same function body.
// Each entry records a function's byte range and its precomputed QN.
#define EFC_SIZE 64 // power of 2 for fast modulo

typedef struct {
    uint32_t start_byte;
    uint32_t end_byte;
    const char *qn;
} EFCEntry;

typedef struct {
    EFCEntry entries[EFC_SIZE];
    int count;
} EFCache;

// --- Extraction context passed to sub-extractors ---

// Module-level string constant map (for constant propagation)
#define ANI_MAX_STRING_CONSTANTS 256
typedef struct {
    const char *names[ANI_MAX_STRING_CONSTANTS];
    const char *values[ANI_MAX_STRING_CONSTANTS];
    bool is_url_builder[ANI_MAX_STRING_CONSTANTS];
    int count;
} ANIStringConstantMap;

// Forward declaration: ObjectScript macro table (defined in macro_table.h).
typedef struct ANIMacroTable ANIMacroTable;

// Method-return-type table for ObjectScript variable type inference. Populated
// from definition nodes (method QN -> declared return type) so a later
// `Set x = obj.Method()` can resolve x's class.
#define ANI_RETURN_TYPE_TABLE_CAP 2048

typedef struct {
    const char *method_qn;
    const char *return_type;
} ANIReturnTypeEntry;

typedef struct {
    ANIReturnTypeEntry entries[ANI_RETURN_TYPE_TABLE_CAP];
    int count;
} ANIReturnTypeTable;

typedef struct {
    ANIArena *arena;
    /* Scratch for AST traversal, owned by the ani_extract_file_ex call that
     * built this context and destroyed when it returns. Nothing a
     * ANIFileResult points at may be allocated here: `arena` is the result's
     * own, and it outlives extraction by the whole pipeline (#1997). NULL in a
     * context built without one, in which case the stacks fall back to
     * `arena`. */
    ANIArena *scratch;
    ANIFileResult *result;
    const char *source;
    int source_len;
    ANILanguage language;
    const char *project;
    const char *rel_path;
    const char *module_qn;
    TSNode root;
    EFCache ef_cache;                            // enclosing function cache
    const char *enclosing_class_qn;              // for nested class QN computation
    ANIStringConstantMap string_constants;       // module-level NAME = "value" pairs
    const ANIMacroTable *macro_table;            // ObjectScript $$$macro table (NULL if none)
    const ANIReturnTypeTable *return_type_table; // ObjectScript method return types (NULL if none)
    /* Set by extract_class_variables around its extract_var_names calls, so a
     * class-body variable def records which class declares it (parent_class)
     * without changing its module-level qualified name. NULL elsewhere. */
    const char *var_parent_class;
} ANIExtractCtx;

// --- Public API ---

// Bind third-party allocators (tree-sitter, sqlite3) to mimalloc as
// defense-in-depth, so they never depend on the fragile MI_OVERRIDE symbol
// override (#424). MUST be called as the very first statement of main(), before
// any sqlite3_open*/sqlite3_initialize (SQLITE_CONFIG_MALLOC returns
// SQLITE_MISUSE once sqlite has initialized).
// Idempotent (static guard); intended for single-threaded startup. ani_init()
// also calls it so non-main entry points (pipeline passes) still get the binds.
// In the test build (no ANI_BIND_TS_ALLOCATOR) this is a no-op.
void ani_alloc_init(void);

// Initialize the library. Call once at startup. Returns 0 on success.
int ani_init(void);

// True when rel_path is in the crash-quarantine set — the newline-delimited list
// of files (ANI_INDEX_QUARANTINE_FILE) the crash supervisor pinned as crashers
// during its single-threaded recovery re-run. Loaded once, lazily; read-only
// after load. ani_extract_file short-circuits such files to an empty result so no
// pass can crash on them; the pipeline extract loops call this to also REPORT the
// skip as phase="crash". Always false (cheap no-op) when the env var is unset.
bool ani_index_is_quarantined(const char *rel_path);

// Phase a quarantined file was pinned under: "crash" (a fault signal) or "hang"
// (killed for making no progress). Returns NULL when rel_path is not quarantined.
// Drives the same lazy once-load as ani_index_is_quarantined. Used by the pipeline
// extract loops to report the skip's phase in skipped[] (falls back to "crash").
const char *ani_index_quarantine_phase(const char *rel_path);

// Crash-supervisor marker journal (parallel-safe): appends "S <rel_path>" /
// "D <rel_path>" to ANI_INDEX_MARKER_FILE. Files with an S but no D form the
// parent's crash/hang suspect set. No-ops when the env var is unset.
// ani_extract_file journals its own start/done; long-running per-file phases
// (cross-LSP resolve) call these around their per-file work so a hang there
// is attributed to the RIGHT file instead of a stale extraction marker.
void ani_index_mark_start(const char *rel_path);
void ani_index_mark_done(const char *rel_path);

// Extract all data from one file. Caller must call ani_free_result().
// source must remain valid for the duration of the call.
// timeout_micros: per-file parse timeout in microseconds (0 = no timeout).
ANIFileResult *ani_extract_file(const char *source, int source_len, ANILanguage language,
                                const char *project, const char *rel_path, int64_t timeout_micros,
                                const char **extra_defines, // NULL-terminated, or NULL
                                const char **include_paths  // NULL-terminated, or NULL
);

// Pipeline-internal variant of ani_extract_file() carrying ObjectScript
// per-project tables (macro table + method-return-type table). The public
// ani_extract_file() is a thin wrapper that passes NULL, NULL for both.
ANIFileResult *ani_extract_file_ex(
    const char *source, int source_len, ANILanguage language, const char *project,
    const char *rel_path, int64_t timeout_micros,
    const char **extra_defines,                 // NULL-terminated, or NULL
    const char **include_paths,                 // NULL-terminated, or NULL
    const ANIMacroTable *macro_table,           // ObjectScript macros, or NULL
    const ANIReturnTypeTable *return_type_table // OS return types, or NULL
);

// Free all memory associated with a result.
void ani_free_result(ANIFileResult *result);

// Free only the cached tree from a result (caller retained it for reuse).
void ani_free_tree(ANIFileResult *result);

// Free a standalone TSTree pointer (for Go layer cleanup).
void ani_free_tree_ptr(TSTree *tree);

// Reset the thread-local parser's internal state, releasing slab-allocated
// subtrees. Must be called BEFORE ani_slab_reset_thread() so the slab rebuild
// doesn't corrupt live parser state.
void ani_reset_thread_parser(void);

// Destroy the thread-local parser. Call on worker thread exit.
void ani_destroy_thread_parser(void);

// Shutdown the library. Call once at exit.
void ani_shutdown(void);

// Profiling: get accumulated parse/extraction times and file count.
typedef struct {
    uint64_t *parse_ns;
    uint64_t *extract_ns;
    uint64_t *files;
} ani_profile_out_t;
void ani_get_profile(ani_profile_out_t out);
uint64_t ani_get_lsp_ns(void);
uint64_t ani_get_preprocess_ns(void);
uint64_t ani_get_files_preprocessed(void);
void ani_reset_profile(void);

#if defined(ANI_KOTLIN_DEDUP_TEST_API) && ANI_KOTLIN_DEDUP_TEST_API
// Test-build-only operation counter for Kotlin operator-carrier deduplication.
// Production builds do not expose or retain this instrumentation.
void ani_kotlin_operator_dedup_test_reset(void);
uint64_t ani_kotlin_operator_dedup_test_comparisons(void);
#endif

#if defined(ANI_CALL_REFERENCE_LOOKUP_TEST_API) && ANI_CALL_REFERENCE_LOOKUP_TEST_API
// Test-build-only work counter for resolving a node's field role while
// classifying value references. Production builds retain no instrumentation.
void ani_usage_field_lookup_test_reset(void);
uint64_t ani_usage_field_lookup_test_work(void);
uint64_t ani_usage_slow_parent_fallback_test_count(void);
#endif

// Toggle C/C++ preprocessor Macro-node extraction (#375). The pipeline enables
// it only for full/advanced index modes (it dominates extraction on macro-dense
// codebases). Default ON. Set before extraction; read-only during.
void ani_set_macro_extraction(int enabled);
int ani_macro_extraction_enabled(void);

// --- Internal helpers used by extractors ---

// Growable array push functions (arena-allocated, no individual free needed).
void ani_defs_push(ANIDefArray *arr, ANIArena *a, ANIDefinition def);
void ani_calls_push(ANICallArray *arr, ANIArena *a, ANICall call);
void ani_imports_push(ANIImportArray *arr, ANIArena *a, ANIImport imp);
void ani_usages_push(ANIUsageArray *arr, ANIArena *a, ANIUsage usage);
void ani_throws_push(ANIThrowArray *arr, ANIArena *a, ANIThrow thr);
void ani_rw_push(ANIRWArray *arr, ANIArena *a, ANIReadWrite rw);
void ani_typerefs_push(ANITypeRefArray *arr, ANIArena *a, ANITypeRef tr);
void ani_envaccess_push(ANIEnvAccessArray *arr, ANIArena *a, ANIEnvAccess ea);
void ani_typeassign_push(ANITypeAssignArray *arr, ANIArena *a, ANITypeAssign ta);
void ani_stringref_push(ANIStringRefArray *arr, ANIArena *a, ANIStringRef sr);
void ani_infrabinding_push(ANIInfraBindingArray *arr, ANIArena *a, ANIInfraBinding ib);
void ani_impltrait_push(ANIImplTraitArray *arr, ANIArena *a, ANIImplTrait it);
void ani_resolvedcall_push(ANIResolvedCallArray *arr, ANIArena *a, ANIResolvedCall rc);
void ani_channels_push(ANIChannelArray *arr, ANIArena *a, ANIChannel ch);

// --- Sub-extractor entry points ---

void ani_extract_definitions(ANIExtractCtx *ctx);
/* Internal companion for embedded-language trees that contribute definitions
 * to an existing host-file Module rather than minting a second Module. */
void ani_extract_definitions_without_module(ANIExtractCtx *ctx);
// dbt lineage for Jinja-templated SQL models: emits a Model def plus one usage
// per ref()/source() call. No-op unless the file parses as SQL and actually
// contains a dbt builtin call. Defined in extract_dbt.c.
void ani_extract_dbt(ANIExtractCtx *ctx);
void ani_extract_imports(ANIExtractCtx *ctx);
void ani_extract_usages(ANIExtractCtx *ctx);
void ani_extract_semantic(ANIExtractCtx *ctx);
void ani_extract_type_refs(ANIExtractCtx *ctx);
void ani_extract_env_accesses(ANIExtractCtx *ctx);
void ani_extract_type_assigns(ANIExtractCtx *ctx);
void ani_extract_channels(ANIExtractCtx *ctx);

// Single-pass unified extraction (replaces the 7 calls above except defs+imports).
void ani_extract_unified(ANIExtractCtx *ctx);

// K8s / Kustomize semantic extractor (called when language is ANI_LANG_K8S or ANI_LANG_KUSTOMIZE).
void ani_extract_k8s(ANIExtractCtx *ctx);

// --- Label predicates ---

// True when `label` names a TYPE-LIKE container definition — a node that can own
// methods/fields, be a base/embedded type, satisfy/declare an interface, and be a
// target of name→type resolution. The canonical set is:
//   Class, Struct, Interface, Enum, Type, Trait.
// Single source of truth for every type-resolution / registry-seeding /
// INHERITS·IMPLEMENTS / LSP-type-registrar consumer, so adding a new type-like
// label (e.g. "Struct" for Rust/Go/Swift/D structs) updates them all at once
// instead of scattering `|| strcmp(label,"Struct")==0` across the tree.
// `label` may be NULL (returns false). Defined in helpers.c.
bool ani_label_is_type_like(const char *label);

// True for data-relation labels (Table, View — SQL DDL). Relations resolve as
// lineage targets only: registry members, but never type-like and never valid
// CALLS/THROWS/READS/WRITES targets. `label` may be NULL. Defined in helpers.c.
bool ani_label_is_relation(const char *label);

// True for labels admitted to the cross-file name registry: Function, Method,
// every type-like container, Variable, Field, and the relation labels. Single
// source of truth for registry seeding — the full (pass_definitions.c),
// parallel (pass_parallel.c) and incremental (pipeline_incremental.c) pipelines
// all seed through this predicate so their registries never diverge.
// `label` may be NULL (returns false). Defined in helpers.c.
bool ani_label_is_registry_symbol(const char *label);

#endif // ANI_H
