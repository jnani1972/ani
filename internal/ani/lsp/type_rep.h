#ifndef ANI_LSP_TYPE_REP_H
#define ANI_LSP_TYPE_REP_H

#include "../arena.h"
#include <stdbool.h>
#include <stdint.h>

// ANITypeKind enumerates all type representations.
typedef enum {
    ANI_TYPE_UNKNOWN = 0,
    ANI_TYPE_NAMED,       // named type: "Database", "http.Request"
    ANI_TYPE_POINTER,     // *T
    ANI_TYPE_SLICE,       // []T
    ANI_TYPE_MAP,         // map[K]V
    ANI_TYPE_CHANNEL,     // chan T
    ANI_TYPE_FUNC,        // func(params) returns
    ANI_TYPE_INTERFACE,   // interface{...}
    ANI_TYPE_STRUCT,      // struct{...}
    ANI_TYPE_BUILTIN,     // int, string, bool, error, etc.
    ANI_TYPE_TUPLE,       // multi-return (T1, T2) / TS tuple [T,U]
    ANI_TYPE_TYPE_PARAM,  // generic type parameter: T, K, V
    ANI_TYPE_REFERENCE,   // T& (C++ lvalue reference)
    ANI_TYPE_RVALUE_REF,  // T&& (C++ rvalue reference)
    ANI_TYPE_TEMPLATE,    // Parameterized type: vector<T> — stores template name + args
    ANI_TYPE_ALIAS,       // Type alias: using/typedef — stores alias name + underlying type
    ANI_TYPE_UNION,       // Python: A | B; TS: A | B | C — sorted-canonical list (shared)
    ANI_TYPE_LITERAL,     // Python: Literal["foo", 3] — wraps a base type + literal value text
    ANI_TYPE_PROTOCOL,    // Python: typing.Protocol — like INTERFACE but matched structurally
    ANI_TYPE_MODULE,      // Python: import os; os is a module-typed binding
    ANI_TYPE_CALLABLE,    // Python: Callable[[A, B], R] — untyped-named callable variant of FUNC

    // --- TS-specific kinds (added in TS LSP integration) ---
    ANI_TYPE_INTERSECTION,  // TS: A & B — intersection type
    ANI_TYPE_TS_LITERAL,    // TS: "foo" / 42 / true literal types (tag+value layout, distinct
                            // from Python's ANI_TYPE_LITERAL which uses base+literal_text)
    ANI_TYPE_INDEXED,       // TS: T[K] — indexed access type
    ANI_TYPE_KEYOF,         // TS: keyof T
    ANI_TYPE_TYPEOF_QUERY,  // TS: typeof x in type position
    ANI_TYPE_CONDITIONAL,   // TS: T extends U ? X : Y
    ANI_TYPE_OBJECT_LIT,    // TS: { a: T1; b: T2 } anonymous object type
    ANI_TYPE_INFER,         // TS: `infer X` placeholder inside conditional
    ANI_TYPE_MAPPED,        // TS: {[K in keyof T]: ...} — v1 stub, members may be NULL
} ANITypeKind;

// Forward declaration
typedef struct ANIType ANIType;

// Language-specific adapter used to parse one ordered signature type spelling.
typedef const ANIType *(*ANITypeTextParser)(ANIArena *arena, const char *text, void *parser_ctx);

// ANITypeParam represents a generic type parameter with optional constraint.
typedef struct {
    const char* name;        // "T", "K", "V"
    const ANIType* constraint; // interface constraint, or NULL for "any"
} ANITypeParam;

// ANIType is a tagged union representing Go types.
struct ANIType {
    ANITypeKind kind;
    union {
        struct { const char* qualified_name; } named;      // NAMED
        struct { const ANIType* elem; } pointer;            // POINTER
        struct { const ANIType* elem; } slice;              // SLICE
        struct { const ANIType* key; const ANIType* value; } map;  // MAP
        struct { const ANIType* elem; int direction; } channel;    // CHANNEL (0=bidi, 1=send, 2=recv)
        struct {
            const char** param_names;  // NULL-terminated
            const ANIType** param_types; // NULL-terminated
            const ANIType** return_types; // NULL-terminated
        } func;                                             // FUNC
        struct {
            const char** method_names;  // NULL-terminated
            const ANIType** method_sigs; // NULL-terminated (each is FUNC)
        } interface_type;                                   // INTERFACE
        struct {
            const char** field_names;   // NULL-terminated
            const ANIType** field_types; // NULL-terminated
        } struct_type;                                      // STRUCT
        struct { const char* name; } builtin;               // BUILTIN
        struct {
            const ANIType** elems;      // NULL-terminated
            int count;
        } tuple;                                            // TUPLE
        struct { const char* name; } type_param;            // TYPE_PARAM
        struct { const ANIType* elem; } reference;            // REFERENCE / RVALUE_REF
        struct {
            const char* template_name;      // "std::vector", "std::map"
            const ANIType** template_args;  // NULL-terminated
            int arg_count;
        } template_type;                                      // TEMPLATE
        struct {
            const char* alias_qn;          // "proj.ns.MyAlias"
            const ANIType* underlying;     // the actual type it aliases
        } alias;                                              // ALIAS
        struct {
            const ANIType** members;       // NULL-terminated, deduplicated, sorted by kind/qn
            int count;
        } union_type;                                         // UNION / INTERSECTION (shared)
        struct {
            const ANIType* base;           // base type (e.g. BUILTIN("int"), BUILTIN("str"))
            const char* literal_text;      // canonical text: "3", "\"foo\"", "True"
        } literal;                                            // LITERAL (Python)
        struct {
            const char* qualified_name;    // e.g. "typing.Iterable"
            const char** method_names;     // NULL-terminated method names — structural matching
            const ANIType** method_sigs;   // NULL-terminated signatures (each is FUNC/CALLABLE)
        } protocol;                                           // PROTOCOL
        struct {
            const char* module_qn;         // module qualified name (matches ANIImport.module_path)
        } module;                                             // MODULE
        struct {
            const ANIType** param_types;   // NULL-terminated; NULL element means "Any" / unknown
            const ANIType* return_type;    // single return; for tuples wrap in ANI_TYPE_TUPLE
            int param_count;               // -1 = elliptic / Callable[..., R]
        } callable;                                           // CALLABLE

        // --- TS-specific data ---
        struct {
            // Tag distinguishes string / number / boolean / bigint / null / undefined literals.
            // For boolean literals, value points to "true" or "false".
            const char* tag;               // "string" | "number" | "boolean" | "bigint" | "null" | "undefined"
            const char* value;             // textual representation; arena-owned
        } literal_ts;                                         // TS_LITERAL
        struct {
            const ANIType* object;         // T in T[K]
            const ANIType* index;          // K in T[K]
        } indexed;                                            // INDEXED
        struct { const ANIType* operand; } keyof;             // KEYOF
        struct { const char* expr; } typeof_query;            // TYPEOF_QUERY (referenced expression text)
        struct {
            const ANIType* check;          // T
            const ANIType* extends;        // U
            const ANIType* true_branch;    // X
            const ANIType* false_branch;   // Y
        } conditional;                                        // CONDITIONAL
        struct {
            const char** prop_names;       // NULL-terminated
            const ANIType** prop_types;    // NULL-terminated, parallel to prop_names
            const ANIType* call_signature; // FUNC type or NULL
            const ANIType* index_value;    // type produced by string/number index, or NULL
        } object_lit;                                         // OBJECT_LIT
        struct { const char* name; } infer;                   // INFER (e.g., `infer R`)
        struct {
            const char* key_name;          // "K" in {[K in keyof T]: V}
            const ANIType* key_constraint; // `keyof T`
            const ANIType* value;          // V (may reference key_name as TYPE_PARAM)
        } mapped;                                             // MAPPED (v1 stub-friendly)
    } data;
};

// Constructors (arena-allocated)
const ANIType* ani_type_unknown(void);
const ANIType* ani_type_named(ANIArena* a, const char* qualified_name);
const ANIType* ani_type_pointer(ANIArena* a, const ANIType* elem);
const ANIType* ani_type_slice(ANIArena* a, const ANIType* elem);
const ANIType* ani_type_map(ANIArena* a, const ANIType* key, const ANIType* value);
const ANIType* ani_type_channel(ANIArena* a, const ANIType* elem, int direction);
const ANIType* ani_type_func(ANIArena* a, const char** param_names, const ANIType** param_types, const ANIType** return_types);
// Materialize exactly count positional parameter slots. NULL, empty, exact "?",
// and parser failures become UNKNOWN; the returned vector is NULL-terminated.
const ANIType **ani_type_materialize_signature_params(ANIArena *a, const char *const *type_texts,
                                                      int count, ANITypeTextParser parser,
                                                      void *parser_ctx);
// Rebuild a FUNC with new returns while preserving its parameter names/types.
const ANIType *ani_type_func_replace_returns(ANIArena *a, const ANIType *old_signature,
                                             const ANIType *const *new_return_types);
const ANIType* ani_type_builtin(ANIArena* a, const char* name);
const ANIType* ani_type_tuple(ANIArena* a, const ANIType** elems, int count);
const ANIType* ani_type_type_param(ANIArena* a, const char* name);
const ANIType* ani_type_reference(ANIArena* a, const ANIType* elem);
const ANIType* ani_type_rvalue_ref(ANIArena* a, const ANIType* elem);
const ANIType* ani_type_template(ANIArena* a, const char* name, const ANIType** args, int arg_count);
const ANIType* ani_type_alias(ANIArena* a, const char* alias_qn, const ANIType* underlying);

// Python-flavored constructors. UNION normalizes input: nested unions are
// flattened, duplicates removed, single-member unions collapse to that
// member, and the empty union is UNKNOWN. Members must be arena-allocated.
// Shared with TS LSP — both call this same constructor for `A | B`.
const ANIType* ani_type_union(ANIArena* a, const ANIType** members, int count);
const ANIType* ani_type_optional(ANIArena* a, const ANIType* t);  // Optional[T] == Union[T, None]
const ANIType* ani_type_literal(ANIArena* a, const ANIType* base, const char* literal_text);
const ANIType* ani_type_protocol(ANIArena* a, const char* qualified_name,
    const char** method_names, const ANIType** method_sigs);
const ANIType* ani_type_module(ANIArena* a, const char* module_qn);
const ANIType* ani_type_callable(ANIArena* a, const ANIType** param_types, int param_count,
    const ANIType* return_type);

// --- TS-specific constructors ---
const ANIType* ani_type_intersection(ANIArena* a, const ANIType** members, int count);
// tag is one of "string"|"number"|"boolean"|"bigint"|"null"|"undefined".
// Distinct from ani_type_literal (Python) which uses base+literal_text.
const ANIType* ani_type_ts_literal(ANIArena* a, const char* tag, const char* value);
const ANIType* ani_type_indexed(ANIArena* a, const ANIType* object, const ANIType* index);
const ANIType* ani_type_keyof(ANIArena* a, const ANIType* operand);
const ANIType* ani_type_typeof_query(ANIArena* a, const char* expr);
const ANIType* ani_type_conditional(ANIArena* a,
    const ANIType* check, const ANIType* extends,
    const ANIType* true_branch, const ANIType* false_branch);
// prop_names and prop_types are NULL-terminated parallel arrays; either may be NULL for empty.
const ANIType* ani_type_object_lit(ANIArena* a,
    const char** prop_names, const ANIType** prop_types,
    const ANIType* call_signature, const ANIType* index_value);
const ANIType* ani_type_infer(ANIArena* a, const char* name);
const ANIType* ani_type_mapped(ANIArena* a,
    const char* key_name, const ANIType* key_constraint, const ANIType* value);

// Operations
const ANIType* ani_type_deref(const ANIType* t);         // remove one pointer level
const ANIType* ani_type_elem(const ANIType* t);           // get element type (slice/chan/pointer)
bool ani_type_is_unknown(const ANIType* t);
bool ani_type_is_interface(const ANIType* t);
bool ani_type_is_pointer(const ANIType* t);
bool ani_type_is_reference(const ANIType* t);
bool ani_type_is_union(const ANIType* t);
bool ani_type_is_protocol(const ANIType* t);
bool ani_type_is_module(const ANIType* t);

// Structural equality on type representation (used by union dedup and
// protocol-method-set matching). Two types are equal if their kinds match
// and their structural members match recursively.
bool ani_type_equal(const ANIType* a, const ANIType* b);

// Test whether `candidate` satisfies the structural protocol `proto`.
// Walks proto.method_names against candidate's method set (NAMED → registry
// lookup is the caller's job; this helper only matches existing method
// signatures stored on a PROTOCOL).
bool ani_type_protocol_satisfied_by(const ANIType* proto, const ANIType* candidate);

// Follow alias chain with cycle detection (max 16 levels).
const ANIType* ani_type_resolve_alias(const ANIType* t);

// Generic type substitution: replace type params in t with concrete types.
// type_params: NULL-terminated array of param names
// type_args: corresponding concrete types
const ANIType* ani_type_substitute(ANIArena* a, const ANIType* t,
    const char** type_params, const ANIType** type_args);

#endif // ANI_LSP_TYPE_REP_H
