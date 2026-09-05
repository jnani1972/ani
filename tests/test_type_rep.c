/*
 * test_type_rep.c — Phase 1 type-rep extensions for Python LSP.
 *
 * Covers the five new kinds (UNION, LITERAL, PROTOCOL, MODULE, CALLABLE),
 * structural equality, union normalization (flatten + dedup + collapse),
 * Optional[T] sugar, and protocol structural matching.
 */
#include "test_framework.h"
#include "ani.h"
#include "lsp/type_rep.h"

/* ── UNION: flatten, dedup, collapse ──────────────────────────── */

TEST(typerep_union_two_distinct) {
    ANIArena a;
    ani_arena_init(&a);
    const ANIType *i = ani_type_builtin(&a, "int");
    const ANIType *s = ani_type_builtin(&a, "str");
    const ANIType *m[2] = {i, s};
    const ANIType *u = ani_type_union(&a, m, 2);
    ASSERT(ani_type_is_union(u));
    ASSERT_EQ(u->data.union_type.count, 2);
    ani_arena_destroy(&a);
    PASS();
}

TEST(typerep_union_dedupes_duplicates) {
    ANIArena a;
    ani_arena_init(&a);
    const ANIType *i1 = ani_type_builtin(&a, "int");
    const ANIType *i2 = ani_type_builtin(&a, "int");
    const ANIType *m[2] = {i1, i2};
    const ANIType *u = ani_type_union(&a, m, 2);
    /* dedup collapses to a single int type, not a union */
    ASSERT(!ani_type_is_union(u));
    ASSERT_EQ(u->kind, ANI_TYPE_BUILTIN);
    ASSERT_STR_EQ(u->data.builtin.name, "int");
    ani_arena_destroy(&a);
    PASS();
}

TEST(typerep_union_flattens_nested) {
    ANIArena a;
    ani_arena_init(&a);
    const ANIType *i = ani_type_builtin(&a, "int");
    const ANIType *s = ani_type_builtin(&a, "str");
    const ANIType *b = ani_type_builtin(&a, "bytes");
    const ANIType *inner_pair[2] = {i, s};
    const ANIType *inner = ani_type_union(&a, inner_pair, 2);
    const ANIType *outer_pair[2] = {inner, b};
    const ANIType *u = ani_type_union(&a, outer_pair, 2);
    /* flattening yields 3 distinct members */
    ASSERT(ani_type_is_union(u));
    ASSERT_EQ(u->data.union_type.count, 3);
    ani_arena_destroy(&a);
    PASS();
}

TEST(typerep_union_single_member_collapses) {
    ANIArena a;
    ani_arena_init(&a);
    const ANIType *i = ani_type_builtin(&a, "int");
    const ANIType *m[1] = {i};
    const ANIType *u = ani_type_union(&a, m, 1);
    ASSERT(!ani_type_is_union(u));
    ASSERT_EQ(u->kind, ANI_TYPE_BUILTIN);
    ani_arena_destroy(&a);
    PASS();
}

TEST(typerep_union_empty_is_unknown) {
    ANIArena a;
    ani_arena_init(&a);
    const ANIType *u = ani_type_union(&a, NULL, 0);
    ASSERT(ani_type_is_unknown(u));
    ani_arena_destroy(&a);
    PASS();
}

TEST(typerep_optional_is_union_with_none) {
    ANIArena a;
    ani_arena_init(&a);
    const ANIType *i = ani_type_builtin(&a, "int");
    const ANIType *opt = ani_type_optional(&a, i);
    ASSERT(ani_type_is_union(opt));
    ASSERT_EQ(opt->data.union_type.count, 2);
    /* membership: int + None */
    bool has_int = false, has_none = false;
    for (int k = 0; k < opt->data.union_type.count; k++) {
        const ANIType *m = opt->data.union_type.members[k];
        if (m->kind == ANI_TYPE_BUILTIN && strcmp(m->data.builtin.name, "int") == 0)
            has_int = true;
        if (m->kind == ANI_TYPE_BUILTIN && strcmp(m->data.builtin.name, "None") == 0)
            has_none = true;
    }
    ASSERT(has_int);
    ASSERT(has_none);
    ani_arena_destroy(&a);
    PASS();
}

/* ── LITERAL ───────────────────────────────────────────────────── */

TEST(typerep_literal_int_3) {
    ANIArena a;
    ani_arena_init(&a);
    const ANIType *base = ani_type_builtin(&a, "int");
    const ANIType *lit = ani_type_literal(&a, base, "3");
    ASSERT_EQ(lit->kind, ANI_TYPE_LITERAL);
    ASSERT(ani_type_equal(lit->data.literal.base, base));
    ASSERT_STR_EQ(lit->data.literal.literal_text, "3");
    ani_arena_destroy(&a);
    PASS();
}

TEST(typerep_literal_equality_distinguishes_text) {
    ANIArena a;
    ani_arena_init(&a);
    const ANIType *base = ani_type_builtin(&a, "str");
    const ANIType *foo = ani_type_literal(&a, base, "\"foo\"");
    const ANIType *bar = ani_type_literal(&a, base, "\"bar\"");
    const ANIType *foo2 = ani_type_literal(&a, base, "\"foo\"");
    ASSERT(!ani_type_equal(foo, bar));
    ASSERT(ani_type_equal(foo, foo2));
    ani_arena_destroy(&a);
    PASS();
}

/* ── PROTOCOL ──────────────────────────────────────────────────── */

TEST(typerep_protocol_method_set) {
    ANIArena a;
    ani_arena_init(&a);
    const char *methods[] = {"read", "close", NULL};
    const ANIType *proto = ani_type_protocol(&a, "typing.IO", methods, NULL);
    ASSERT(ani_type_is_protocol(proto));
    ASSERT_STR_EQ(proto->data.protocol.qualified_name, "typing.IO");
    ASSERT_NOT_NULL(proto->data.protocol.method_names);
    ASSERT_STR_EQ(proto->data.protocol.method_names[0], "read");
    ASSERT_STR_EQ(proto->data.protocol.method_names[1], "close");
    ani_arena_destroy(&a);
    PASS();
}

TEST(typerep_protocol_satisfied_by_protocol_with_superset) {
    ANIArena a;
    ani_arena_init(&a);
    const char *needed[] = {"read", "close", NULL};
    const char *have[] = {"read", "write", "close", "flush", NULL};
    const ANIType *proto = ani_type_protocol(&a, "P1", needed, NULL);
    const ANIType *cand = ani_type_protocol(&a, "P2", have, NULL);
    ASSERT(ani_type_protocol_satisfied_by(proto, cand));
    ani_arena_destroy(&a);
    PASS();
}

TEST(typerep_protocol_unsatisfied_when_method_missing) {
    ANIArena a;
    ani_arena_init(&a);
    const char *needed[] = {"read", "close", NULL};
    const char *have[] = {"read", NULL};
    const ANIType *proto = ani_type_protocol(&a, "P1", needed, NULL);
    const ANIType *cand = ani_type_protocol(&a, "P2", have, NULL);
    ASSERT(!ani_type_protocol_satisfied_by(proto, cand));
    ani_arena_destroy(&a);
    PASS();
}

/* ── MODULE ────────────────────────────────────────────────────── */

TEST(typerep_module_carries_qn) {
    ANIArena a;
    ani_arena_init(&a);
    const ANIType *m = ani_type_module(&a, "os.path");
    ASSERT(ani_type_is_module(m));
    ASSERT_STR_EQ(m->data.module.module_qn, "os.path");
    ani_arena_destroy(&a);
    PASS();
}

TEST(typerep_module_equality_by_qn) {
    ANIArena a;
    ani_arena_init(&a);
    const ANIType *a1 = ani_type_module(&a, "os");
    const ANIType *a2 = ani_type_module(&a, "os");
    const ANIType *b = ani_type_module(&a, "sys");
    ASSERT(ani_type_equal(a1, a2));
    ASSERT(!ani_type_equal(a1, b));
    ani_arena_destroy(&a);
    PASS();
}

/* ── CALLABLE ──────────────────────────────────────────────────── */

TEST(typerep_callable_with_args_and_return) {
    ANIArena a;
    ani_arena_init(&a);
    const ANIType *i = ani_type_builtin(&a, "int");
    const ANIType *s = ani_type_builtin(&a, "str");
    const ANIType *params[2] = {i, s};
    const ANIType *c = ani_type_callable(&a, params, 2, i);
    ASSERT_EQ(c->kind, ANI_TYPE_CALLABLE);
    ASSERT_EQ(c->data.callable.param_count, 2);
    ASSERT(ani_type_equal(c->data.callable.return_type, i));
    ani_arena_destroy(&a);
    PASS();
}

TEST(typerep_callable_elliptic_arity_minus_one) {
    /* Callable[..., R] — variadic in Python type-hint sense. */
    ANIArena a;
    ani_arena_init(&a);
    const ANIType *r = ani_type_builtin(&a, "int");
    const ANIType *c = ani_type_callable(&a, NULL, -1, r);
    ASSERT_EQ(c->data.callable.param_count, -1);
    ani_arena_destroy(&a);
    PASS();
}

TEST(typerep_callable_equality) {
    ANIArena a;
    ani_arena_init(&a);
    const ANIType *i = ani_type_builtin(&a, "int");
    const ANIType *s = ani_type_builtin(&a, "str");
    const ANIType *p1[1] = {i};
    const ANIType *c1 = ani_type_callable(&a, p1, 1, s);
    const ANIType *c2 = ani_type_callable(&a, p1, 1, s);
    const ANIType *c3 = ani_type_callable(&a, p1, 1, i); /* different return */
    ASSERT(ani_type_equal(c1, c2));
    ASSERT(!ani_type_equal(c1, c3));
    ani_arena_destroy(&a);
    PASS();
}

/* ── SUBSTITUTION ──────────────────────────────────────────────── */

TEST(typerep_substitute_unbound_param_preserved) {
    ANIArena a;
    ani_arena_init(&a);
    const ANIType *t = ani_type_type_param(&a, "T");
    const char *params[] = {"T", NULL};
    const ANIType *args[] = {NULL, NULL};
    const ANIType *sub = ani_type_substitute(&a, t, params, args);
    ASSERT(sub == t);
    ani_arena_destroy(&a);
    PASS();
}

/* #427: type_args may be SHORTER than type_params — a class template
 * instantiated with fewer args than declared params (e.g. `Box<Widget>` for
 * `template<class T, class U, class V>`) or trailing default template args.
 * Matching a param whose index exceeds the args length must NOT index past the
 * args array's NULL terminator. Pre-fix, this read args[2] one element past the
 * 2-slot stack array (ASan stack-buffer-overflow) and returned a bogus ANIType*
 * that was later dereferenced -> SEGV in type_to_qn (c_lsp.c). The unbound
 * param must be preserved as-is. */
TEST(typerep_substitute_short_args_no_oob_issue427) {
    ANIArena a;
    ani_arena_init(&a);
    const ANIType *t = ani_type_type_param(&a, "V");                   /* the 3rd declared param */
    const char *params[] = {"T", "U", "V", NULL};                      /* 3 declared params */
    const ANIType *args[] = {ani_type_type_param(&a, "Widget"), NULL}; /* 1 arg */
    const ANIType *sub = ani_type_substitute(&a, t, params, args);
    ASSERT(sub == t); /* "V" (index 2) has no supplied arg -> preserved, no OOB */
    ani_arena_destroy(&a);
    PASS();
}

/* A caller that forgets to NULL-terminate type_args (a stack array) makes the
 * bounded walk read uninitialized memory — and a garbage "pointer" within the
 * param count would be BOUND to a type param and woven into the resulting type
 * graph (bitcoin serialize.h: Using<Fmt>(v) with explicit args bound T to stack
 * garbage; the corrupt graph was dereferenced later -> SIGSEGV). Implausible
 * values (misaligned / null-page) must act as the terminator instead. */
TEST(typerep_substitute_rejects_garbage_args_entries) {
    ANIArena a;
    ani_arena_init(&a);
    const ANIType *t =
        ani_type_reference(&a, ani_type_type_param(&a, "T")); /* T& as in Wrapper<F, T&> */
    const char *params[] = {"F", "T", NULL};
    const ANIType *args[2];
    args[0] = ani_type_named(&a, "proj.Fmt"); /* explicit arg for F */
    args[1] = (const ANIType *)0x37;          /* simulated uninitialized stack garbage */
    const ANIType *sub = ani_type_substitute(&a, t, params, args);
    ASSERT_NOT_NULL(sub);
    ASSERT_EQ(sub->kind, ANI_TYPE_REFERENCE);
    /* T has no real binding: it must be preserved, never the garbage value. */
    ASSERT_TRUE(sub->data.reference.elem != (const ANIType *)0x37);
    ASSERT_EQ(sub->data.reference.elem->kind, ANI_TYPE_TYPE_PARAM);
    ani_arena_destroy(&a);
    PASS();
}

/* ── Suite ─────────────────────────────────────────────────────── */

SUITE(type_rep) {
    /* UNION */
    RUN_TEST(typerep_union_two_distinct);
    RUN_TEST(typerep_union_dedupes_duplicates);
    RUN_TEST(typerep_union_flattens_nested);
    RUN_TEST(typerep_union_single_member_collapses);
    RUN_TEST(typerep_union_empty_is_unknown);
    RUN_TEST(typerep_optional_is_union_with_none);
    /* LITERAL */
    RUN_TEST(typerep_literal_int_3);
    RUN_TEST(typerep_literal_equality_distinguishes_text);
    /* PROTOCOL */
    RUN_TEST(typerep_protocol_method_set);
    RUN_TEST(typerep_protocol_satisfied_by_protocol_with_superset);
    RUN_TEST(typerep_protocol_unsatisfied_when_method_missing);
    /* MODULE */
    RUN_TEST(typerep_module_carries_qn);
    RUN_TEST(typerep_module_equality_by_qn);
    /* CALLABLE */
    RUN_TEST(typerep_callable_with_args_and_return);
    RUN_TEST(typerep_callable_elliptic_arity_minus_one);
    RUN_TEST(typerep_callable_equality);
    /* SUBSTITUTION */
    RUN_TEST(typerep_substitute_unbound_param_preserved);
    RUN_TEST(typerep_substitute_short_args_no_oob_issue427);
    RUN_TEST(typerep_substitute_rejects_garbage_args_entries);
}
