/*
 * java_stdlib_data.c — Curated Java standard-library type/method registry.
 *
 * Strategy:
 *   - java.lang.* — fully covered (the implicit-import package).
 *     Object, String, StringBuilder, StringBuffer, CharSequence, Class,
 *     Throwable + the common subclass tree, Number + boxed primitives,
 *     Math, System, Thread, Iterable, Comparable, Cloneable, Enum, Record,
 *     AutoCloseable, the common Exception types.
 *   - java.util.* — collections + iterators + Optional + Date/Calendar +
 *     Arrays/Collections + Scanner/Random/UUID + Map.Entry.
 *   - java.io.* — streams, readers, writers, File, IOException family.
 *   - java.nio.file.* — Path, Paths, Files (often-used helpers).
 *   - java.util.function — the 21 functional interfaces.
 *   - java.util.stream  — Stream + Collectors entry points.
 *   - java.util.concurrent — ExecutorService, Future, CompletableFuture,
 *     ConcurrentHashMap, the concurrent collection set.
 *   - java.time — LocalDate/LocalTime/LocalDateTime/Duration/Instant.
 *
 * Method signatures use registry-level fidelity: receiver, short name,
 * return type. Param types are intentionally unmodeled (the resolver
 * chooses overloads by arity, with type compatibility scoring breaking
 * ties — see ani_registry_lookup_method_by_args).
 *
 * This is the JLS-spec-aligned slice of the stdlib that 90%+ of real-world
 * Java code touches.
 */

#include "../type_rep.h"
#include "../type_registry.h"
#include "../../arena.h"
#include "../java_lsp.h"
#include <string.h>

#define REG_TYPE(qn_, short_, is_iface_, parents_)            \
    do {                                                      \
        memset(&rt, 0, sizeof(rt));                           \
        rt.qualified_name = (qn_);                            \
        rt.short_name = (short_);                             \
        rt.is_interface = (is_iface_);                        \
        rt.embedded_types = (parents_);                       \
        ani_registry_add_type(reg, rt);                       \
    } while (0)

#define REG_METHOD(class_qn_, method_name_, ret_type_)                                          \
    do {                                                                                        \
        memset(&rf, 0, sizeof(rf));                                                             \
        rf.min_params = -1;                                                                     \
        rf.qualified_name =                                                                     \
            ani_arena_sprintf(arena, "%s.%s", (class_qn_), (method_name_));                     \
        rf.short_name = (method_name_);                                                         \
        rf.receiver_type = (class_qn_);                                                         \
        {                                                                                       \
            const ANIType **rets =                                                              \
                (const ANIType **)ani_arena_alloc(arena, 2 * sizeof(*rets));                    \
            rets[0] = (ret_type_);                                                              \
            rets[1] = NULL;                                                                     \
            rf.signature = ani_type_func(arena, NULL, NULL, rets);                              \
        }                                                                                       \
        ani_registry_add_func(reg, rf);                                                         \
    } while (0)

#define REG_CTOR(class_qn_, short_name_)                                              \
    do {                                                                              \
        memset(&rf, 0, sizeof(rf));                                                   \
        rf.min_params = -1;                                                           \
        rf.qualified_name =                                                           \
            ani_arena_sprintf(arena, "%s.%s", (class_qn_), (short_name_));            \
        rf.short_name = (short_name_);                                                \
        rf.receiver_type = (class_qn_);                                               \
        {                                                                             \
            const ANIType **rets =                                                    \
                (const ANIType **)ani_arena_alloc(arena, 2 * sizeof(*rets));          \
            rets[0] = ani_type_named(arena, (class_qn_));                             \
            rets[1] = NULL;                                                           \
            rf.signature = ani_type_func(arena, NULL, NULL, rets);                    \
        }                                                                             \
        ani_registry_add_func(reg, rf);                                               \
    } while (0)

#define REG_FIELD(class_qn_, name_, type_)                                            \
    do {                                                                              \
        const ANIRegisteredType *_existing =                                          \
            ani_registry_lookup_type(reg, (class_qn_));                               \
        (void)_existing;                                                              \
        /* Field append handled by REG_TYPE_FIELDS below. */                          \
        /* Placeholder for future per-field appends. */                               \
    } while (0)

void ani_java_stdlib_register(ANITypeRegistry *reg, ANIArena *arena) {
    ANIRegisteredType rt;
    ANIRegisteredFunc rf;

    /* ── Type-parent lists (must be static so addresses outlive the call) ── */
    static const char *no_parents[] = {NULL};
    static const char *parents_object[] = {"java.lang.Object", NULL};
    static const char *parents_throwable[] = {"java.lang.Object", NULL};
    static const char *parents_exception[] = {"java.lang.Throwable", NULL};
    static const char *parents_error[] = {"java.lang.Throwable", NULL};
    static const char *parents_runtime_exc[] = {"java.lang.Exception", NULL};
    static const char *parents_io_exc[] = {"java.lang.Exception", NULL};
    static const char *parents_number[] = {"java.lang.Object", NULL};
    static const char *parents_integer[] = {"java.lang.Number", NULL};
    static const char *parents_long[] = {"java.lang.Number", NULL};
    static const char *parents_double[] = {"java.lang.Number", NULL};
    static const char *parents_float[] = {"java.lang.Number", NULL};
    static const char *parents_short[] = {"java.lang.Number", NULL};
    static const char *parents_byte[] = {"java.lang.Number", NULL};
    static const char *parents_string[] = {"java.lang.Object", NULL};
    static const char *parents_charseq[] = {NULL};
    static const char *parents_iterable[] = {NULL};
    static const char *parents_collection[] = {"java.lang.Iterable", NULL};
    static const char *parents_list[] = {"java.util.Collection", NULL};
    static const char *parents_set[] = {"java.util.Collection", NULL};
    static const char *parents_queue[] = {"java.util.Collection", NULL};
    static const char *parents_deque[] = {"java.util.Queue", NULL};
    static const char *parents_map[] = {NULL};
    static const char *parents_map_entry[] = {NULL};
    static const char *parents_iterator[] = {NULL};
    static const char *parents_arraylist[] = {"java.util.List", NULL};
    static const char *parents_linkedlist[] = {"java.util.List", NULL};
    static const char *parents_hashset[] = {"java.util.Set", NULL};
    static const char *parents_treeset[] = {"java.util.Set", NULL};
    static const char *parents_linkedhashset[] = {"java.util.Set", NULL};
    static const char *parents_hashmap[] = {"java.util.Map", NULL};
    static const char *parents_treemap[] = {"java.util.Map", NULL};
    static const char *parents_linkedhashmap[] = {"java.util.Map", NULL};
    static const char *parents_concurrent_hashmap[] = {"java.util.Map", NULL};

    static const char *parents_inputstream[] = {"java.lang.AutoCloseable", NULL};
    static const char *parents_outputstream[] = {"java.lang.AutoCloseable", NULL};
    static const char *parents_reader[] = {"java.lang.AutoCloseable", NULL};
    static const char *parents_writer[] = {"java.lang.AutoCloseable", NULL};
    static const char *parents_buffered_reader[] = {"java.io.Reader", NULL};
    static const char *parents_buffered_writer[] = {"java.io.Writer", NULL};
    static const char *parents_print_stream[] = {"java.io.OutputStream", NULL};
    static const char *parents_print_writer[] = {"java.io.Writer", NULL};
    static const char *parents_file_input_stream[] = {"java.io.InputStream", NULL};
    static const char *parents_file_output_stream[] = {"java.io.OutputStream", NULL};
    static const char *parents_file_reader[] = {"java.io.Reader", NULL};
    static const char *parents_file_writer[] = {"java.io.Writer", NULL};
    static const char *parents_io_exception[] = {"java.lang.Exception", NULL};
    static const char *parents_runtime_exc_chain[] = {"java.lang.RuntimeException", NULL};
    /* Parent lists for types previously registered with inline compound
     * literals. A compound literal has automatic (block) storage duration,
     * so storing its address into the registry left a dangling stack pointer
     * once the REG_TYPE statement's block ended — an AddressSanitizer
     * stack-use-after-scope when the inheritance walk later read
     * rt->embedded_types[0]. These must be static so their addresses outlive
     * the call, exactly like the parent lists above. */
    static const char *parents_gregorian_calendar[] = {"java.util.Calendar", NULL};
    static const char *parents_file_not_found_exc[] = {"java.io.IOException", NULL};
    static const char *parents_closeable[] = {"java.lang.AutoCloseable", NULL};
    static const char *parents_unary_operator[] = {"java.util.function.Function", NULL};
    static const char *parents_binary_operator[] = {"java.util.function.BiFunction", NULL};
    static const char *parents_completable_future[] = {"java.util.concurrent.Future", NULL};
    static const char *parents_reentrant_lock[] = {"java.util.concurrent.locks.Lock", NULL};

    /* ── java.lang ─────────────────────────────────────────────── */
    REG_TYPE("java.lang.Object", "Object", false, no_parents);
    REG_TYPE("java.lang.Class", "Class", false, parents_object);
    REG_TYPE("java.lang.ClassLoader", "ClassLoader", false, parents_object);
    REG_TYPE("java.lang.CharSequence", "CharSequence", true, parents_charseq);
    REG_TYPE("java.lang.String", "String", false, parents_string);
    REG_TYPE("java.lang.StringBuilder", "StringBuilder", false, parents_object);
    REG_TYPE("java.lang.StringBuffer", "StringBuffer", false, parents_object);
    REG_TYPE("java.lang.Number", "Number", false, parents_number);
    REG_TYPE("java.lang.Integer", "Integer", false, parents_integer);
    REG_TYPE("java.lang.Long", "Long", false, parents_long);
    REG_TYPE("java.lang.Short", "Short", false, parents_short);
    REG_TYPE("java.lang.Byte", "Byte", false, parents_byte);
    REG_TYPE("java.lang.Float", "Float", false, parents_float);
    REG_TYPE("java.lang.Double", "Double", false, parents_double);
    REG_TYPE("java.lang.Boolean", "Boolean", false, parents_object);
    REG_TYPE("java.lang.Character", "Character", false, parents_object);
    REG_TYPE("java.lang.Void", "Void", false, parents_object);
    REG_TYPE("java.lang.Iterable", "Iterable", true, parents_iterable);
    REG_TYPE("java.lang.Comparable", "Comparable", true, no_parents);
    REG_TYPE("java.lang.Cloneable", "Cloneable", true, no_parents);
    REG_TYPE("java.lang.Runnable", "Runnable", true, no_parents);
    REG_TYPE("java.lang.AutoCloseable", "AutoCloseable", true, no_parents);
    REG_TYPE("java.lang.Math", "Math", false, parents_object);
    REG_TYPE("java.lang.System", "System", false, parents_object);
    REG_TYPE("java.lang.Thread", "Thread", false, parents_object);
    REG_TYPE("java.lang.Process", "Process", false, parents_object);
    REG_TYPE("java.lang.ProcessBuilder", "ProcessBuilder", false, parents_object);
    REG_TYPE("java.lang.StackTraceElement", "StackTraceElement", false, parents_object);
    REG_TYPE("java.lang.Enum", "Enum", false, parents_object);
    REG_TYPE("java.lang.Record", "Record", false, parents_object);
    REG_TYPE("java.lang.Throwable", "Throwable", false, parents_throwable);
    REG_TYPE("java.lang.Exception", "Exception", false, parents_exception);
    REG_TYPE("java.lang.Error", "Error", false, parents_error);
    REG_TYPE("java.lang.RuntimeException", "RuntimeException", false, parents_runtime_exc);
    REG_TYPE("java.lang.NullPointerException", "NullPointerException", false,
             parents_runtime_exc_chain);
    REG_TYPE("java.lang.IllegalArgumentException", "IllegalArgumentException", false,
             parents_runtime_exc_chain);
    REG_TYPE("java.lang.IllegalStateException", "IllegalStateException", false,
             parents_runtime_exc_chain);
    REG_TYPE("java.lang.IndexOutOfBoundsException", "IndexOutOfBoundsException", false,
             parents_runtime_exc_chain);
    REG_TYPE("java.lang.ArrayIndexOutOfBoundsException", "ArrayIndexOutOfBoundsException", false,
             parents_runtime_exc_chain);
    REG_TYPE("java.lang.ArithmeticException", "ArithmeticException", false,
             parents_runtime_exc_chain);
    REG_TYPE("java.lang.ClassCastException", "ClassCastException", false,
             parents_runtime_exc_chain);
    REG_TYPE("java.lang.ClassNotFoundException", "ClassNotFoundException", false,
             parents_exception);
    REG_TYPE("java.lang.NumberFormatException", "NumberFormatException", false,
             parents_runtime_exc_chain);
    REG_TYPE("java.lang.UnsupportedOperationException", "UnsupportedOperationException", false,
             parents_runtime_exc_chain);
    REG_TYPE("java.lang.InterruptedException", "InterruptedException", false, parents_exception);
    REG_TYPE("java.lang.SecurityException", "SecurityException", false,
             parents_runtime_exc_chain);
    REG_TYPE("java.lang.NoSuchMethodException", "NoSuchMethodException", false, parents_exception);
    REG_TYPE("java.lang.NoSuchFieldException", "NoSuchFieldException", false, parents_exception);

    /* Annotation-marker types. */
    REG_TYPE("java.lang.Override", "Override", true, no_parents);
    REG_TYPE("java.lang.Deprecated", "Deprecated", true, no_parents);
    REG_TYPE("java.lang.SuppressWarnings", "SuppressWarnings", true, no_parents);
    REG_TYPE("java.lang.FunctionalInterface", "FunctionalInterface", true, no_parents);

    /* ── Object methods ───────────────────────────────────────── */
    REG_METHOD("java.lang.Object", "toString", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.Object", "hashCode", ani_type_builtin(arena, "int"));
    REG_METHOD("java.lang.Object", "equals", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.lang.Object", "getClass", ani_type_named(arena, "java.lang.Class"));
    REG_METHOD("java.lang.Object", "wait", ani_type_builtin(arena, "void"));
    REG_METHOD("java.lang.Object", "notify", ani_type_builtin(arena, "void"));
    REG_METHOD("java.lang.Object", "notifyAll", ani_type_builtin(arena, "void"));
    REG_METHOD("java.lang.Object", "clone", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.lang.Object", "finalize", ani_type_builtin(arena, "void"));

    /* ── String methods ───────────────────────────────────────── */
    REG_METHOD("java.lang.String", "length", ani_type_builtin(arena, "int"));
    REG_METHOD("java.lang.String", "isEmpty", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.lang.String", "isBlank", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.lang.String", "charAt", ani_type_builtin(arena, "char"));
    REG_METHOD("java.lang.String", "codePointAt", ani_type_builtin(arena, "int"));
    REG_METHOD("java.lang.String", "equals", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.lang.String", "equalsIgnoreCase", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.lang.String", "compareTo", ani_type_builtin(arena, "int"));
    REG_METHOD("java.lang.String", "compareToIgnoreCase", ani_type_builtin(arena, "int"));
    REG_METHOD("java.lang.String", "indexOf", ani_type_builtin(arena, "int"));
    REG_METHOD("java.lang.String", "lastIndexOf", ani_type_builtin(arena, "int"));
    REG_METHOD("java.lang.String", "contains", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.lang.String", "startsWith", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.lang.String", "endsWith", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.lang.String", "matches", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.lang.String", "concat", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.String", "substring", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.String", "trim", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.String", "strip", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.String", "stripLeading", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.String", "stripTrailing", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.String", "toLowerCase", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.String", "toUpperCase", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.String", "replace", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.String", "replaceAll", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.String", "replaceFirst", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.String", "split",
               ani_type_slice(arena, ani_type_named(arena, "java.lang.String")));
    REG_METHOD("java.lang.String", "toCharArray", ani_type_slice(arena, ani_type_builtin(arena, "char")));
    REG_METHOD("java.lang.String", "getBytes", ani_type_slice(arena, ani_type_builtin(arena, "byte")));
    REG_METHOD("java.lang.String", "intern", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.String", "format", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.String", "valueOf", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.String", "join", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.String", "repeat", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.String", "lines", ani_type_named(arena, "java.util.stream.Stream"));
    REG_METHOD("java.lang.String", "chars", ani_type_named(arena, "java.util.stream.IntStream"));
    REG_METHOD("java.lang.String", "codePoints",
               ani_type_named(arena, "java.util.stream.IntStream"));
    REG_METHOD("java.lang.String", "hashCode", ani_type_builtin(arena, "int"));
    REG_METHOD("java.lang.String", "toString", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.String", "toCharArray", ani_type_slice(arena, ani_type_builtin(arena, "char")));
    REG_CTOR("java.lang.String", "String");

    /* ── StringBuilder / StringBuffer ─────────────────────────── */
    REG_METHOD("java.lang.StringBuilder", "append",
               ani_type_named(arena, "java.lang.StringBuilder"));
    REG_METHOD("java.lang.StringBuilder", "insert",
               ani_type_named(arena, "java.lang.StringBuilder"));
    REG_METHOD("java.lang.StringBuilder", "delete",
               ani_type_named(arena, "java.lang.StringBuilder"));
    REG_METHOD("java.lang.StringBuilder", "deleteCharAt",
               ani_type_named(arena, "java.lang.StringBuilder"));
    REG_METHOD("java.lang.StringBuilder", "replace",
               ani_type_named(arena, "java.lang.StringBuilder"));
    REG_METHOD("java.lang.StringBuilder", "reverse",
               ani_type_named(arena, "java.lang.StringBuilder"));
    REG_METHOD("java.lang.StringBuilder", "toString",
               ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.StringBuilder", "length", ani_type_builtin(arena, "int"));
    REG_METHOD("java.lang.StringBuilder", "charAt", ani_type_builtin(arena, "char"));
    REG_METHOD("java.lang.StringBuilder", "setLength", ani_type_builtin(arena, "void"));
    REG_METHOD("java.lang.StringBuilder", "indexOf", ani_type_builtin(arena, "int"));
    REG_METHOD("java.lang.StringBuilder", "substring",
               ani_type_named(arena, "java.lang.String"));
    REG_CTOR("java.lang.StringBuilder", "StringBuilder");

    REG_METHOD("java.lang.StringBuffer", "append",
               ani_type_named(arena, "java.lang.StringBuffer"));
    REG_METHOD("java.lang.StringBuffer", "toString",
               ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.StringBuffer", "length", ani_type_builtin(arena, "int"));
    REG_CTOR("java.lang.StringBuffer", "StringBuffer");

    /* ── CharSequence ─────────────────────────────────────────── */
    REG_METHOD("java.lang.CharSequence", "length", ani_type_builtin(arena, "int"));
    REG_METHOD("java.lang.CharSequence", "charAt", ani_type_builtin(arena, "char"));
    REG_METHOD("java.lang.CharSequence", "subSequence",
               ani_type_named(arena, "java.lang.CharSequence"));
    REG_METHOD("java.lang.CharSequence", "toString",
               ani_type_named(arena, "java.lang.String"));

    /* ── Number + boxed types ─────────────────────────────────── */
    REG_METHOD("java.lang.Number", "intValue", ani_type_builtin(arena, "int"));
    REG_METHOD("java.lang.Number", "longValue", ani_type_builtin(arena, "long"));
    REG_METHOD("java.lang.Number", "doubleValue", ani_type_builtin(arena, "double"));
    REG_METHOD("java.lang.Number", "floatValue", ani_type_builtin(arena, "float"));
    REG_METHOD("java.lang.Number", "shortValue", ani_type_builtin(arena, "short"));
    REG_METHOD("java.lang.Number", "byteValue", ani_type_builtin(arena, "byte"));

    REG_METHOD("java.lang.Integer", "intValue", ani_type_builtin(arena, "int"));
    REG_METHOD("java.lang.Integer", "parseInt", ani_type_builtin(arena, "int"));
    REG_METHOD("java.lang.Integer", "valueOf", ani_type_named(arena, "java.lang.Integer"));
    REG_METHOD("java.lang.Integer", "toString", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.Integer", "compare", ani_type_builtin(arena, "int"));
    REG_METHOD("java.lang.Integer", "compareTo", ani_type_builtin(arena, "int"));
    REG_METHOD("java.lang.Integer", "equals", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.lang.Integer", "hashCode", ani_type_builtin(arena, "int"));
    REG_METHOD("java.lang.Integer", "max", ani_type_builtin(arena, "int"));
    REG_METHOD("java.lang.Integer", "min", ani_type_builtin(arena, "int"));
    REG_METHOD("java.lang.Integer", "sum", ani_type_builtin(arena, "int"));
    REG_METHOD("java.lang.Integer", "bitCount", ani_type_builtin(arena, "int"));
    REG_METHOD("java.lang.Integer", "toBinaryString", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.Integer", "toHexString", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.Integer", "toOctalString", ani_type_named(arena, "java.lang.String"));

    REG_METHOD("java.lang.Long", "longValue", ani_type_builtin(arena, "long"));
    REG_METHOD("java.lang.Long", "parseLong", ani_type_builtin(arena, "long"));
    REG_METHOD("java.lang.Long", "valueOf", ani_type_named(arena, "java.lang.Long"));
    REG_METHOD("java.lang.Long", "toString", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.Long", "compareTo", ani_type_builtin(arena, "int"));

    REG_METHOD("java.lang.Double", "doubleValue", ani_type_builtin(arena, "double"));
    REG_METHOD("java.lang.Double", "parseDouble", ani_type_builtin(arena, "double"));
    REG_METHOD("java.lang.Double", "valueOf", ani_type_named(arena, "java.lang.Double"));
    REG_METHOD("java.lang.Double", "toString", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.Double", "isNaN", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.lang.Double", "isInfinite", ani_type_builtin(arena, "boolean"));

    REG_METHOD("java.lang.Float", "floatValue", ani_type_builtin(arena, "float"));
    REG_METHOD("java.lang.Float", "parseFloat", ani_type_builtin(arena, "float"));
    REG_METHOD("java.lang.Float", "valueOf", ani_type_named(arena, "java.lang.Float"));

    REG_METHOD("java.lang.Boolean", "booleanValue", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.lang.Boolean", "parseBoolean", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.lang.Boolean", "valueOf", ani_type_named(arena, "java.lang.Boolean"));
    REG_METHOD("java.lang.Boolean", "toString", ani_type_named(arena, "java.lang.String"));

    REG_METHOD("java.lang.Character", "charValue", ani_type_builtin(arena, "char"));
    REG_METHOD("java.lang.Character", "isDigit", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.lang.Character", "isLetter", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.lang.Character", "isLetterOrDigit", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.lang.Character", "isWhitespace", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.lang.Character", "isUpperCase", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.lang.Character", "isLowerCase", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.lang.Character", "toUpperCase", ani_type_builtin(arena, "char"));
    REG_METHOD("java.lang.Character", "toLowerCase", ani_type_builtin(arena, "char"));
    REG_METHOD("java.lang.Character", "getNumericValue", ani_type_builtin(arena, "int"));
    REG_METHOD("java.lang.Character", "valueOf", ani_type_named(arena, "java.lang.Character"));

    REG_METHOD("java.lang.Byte", "byteValue", ani_type_builtin(arena, "byte"));
    REG_METHOD("java.lang.Byte", "parseByte", ani_type_builtin(arena, "byte"));
    REG_METHOD("java.lang.Byte", "valueOf", ani_type_named(arena, "java.lang.Byte"));

    REG_METHOD("java.lang.Short", "shortValue", ani_type_builtin(arena, "short"));
    REG_METHOD("java.lang.Short", "parseShort", ani_type_builtin(arena, "short"));
    REG_METHOD("java.lang.Short", "valueOf", ani_type_named(arena, "java.lang.Short"));

    /* ── Math ─────────────────────────────────────────────────── */
    REG_METHOD("java.lang.Math", "abs", ani_type_builtin(arena, "double"));
    REG_METHOD("java.lang.Math", "min", ani_type_builtin(arena, "double"));
    REG_METHOD("java.lang.Math", "max", ani_type_builtin(arena, "double"));
    REG_METHOD("java.lang.Math", "sqrt", ani_type_builtin(arena, "double"));
    REG_METHOD("java.lang.Math", "cbrt", ani_type_builtin(arena, "double"));
    REG_METHOD("java.lang.Math", "pow", ani_type_builtin(arena, "double"));
    REG_METHOD("java.lang.Math", "exp", ani_type_builtin(arena, "double"));
    REG_METHOD("java.lang.Math", "log", ani_type_builtin(arena, "double"));
    REG_METHOD("java.lang.Math", "log10", ani_type_builtin(arena, "double"));
    REG_METHOD("java.lang.Math", "sin", ani_type_builtin(arena, "double"));
    REG_METHOD("java.lang.Math", "cos", ani_type_builtin(arena, "double"));
    REG_METHOD("java.lang.Math", "tan", ani_type_builtin(arena, "double"));
    REG_METHOD("java.lang.Math", "asin", ani_type_builtin(arena, "double"));
    REG_METHOD("java.lang.Math", "acos", ani_type_builtin(arena, "double"));
    REG_METHOD("java.lang.Math", "atan", ani_type_builtin(arena, "double"));
    REG_METHOD("java.lang.Math", "atan2", ani_type_builtin(arena, "double"));
    REG_METHOD("java.lang.Math", "floor", ani_type_builtin(arena, "double"));
    REG_METHOD("java.lang.Math", "ceil", ani_type_builtin(arena, "double"));
    REG_METHOD("java.lang.Math", "round", ani_type_builtin(arena, "long"));
    REG_METHOD("java.lang.Math", "random", ani_type_builtin(arena, "double"));
    REG_METHOD("java.lang.Math", "signum", ani_type_builtin(arena, "double"));
    REG_METHOD("java.lang.Math", "hypot", ani_type_builtin(arena, "double"));
    REG_METHOD("java.lang.Math", "floorDiv", ani_type_builtin(arena, "long"));
    REG_METHOD("java.lang.Math", "floorMod", ani_type_builtin(arena, "long"));
    REG_METHOD("java.lang.Math", "addExact", ani_type_builtin(arena, "long"));
    REG_METHOD("java.lang.Math", "subtractExact", ani_type_builtin(arena, "long"));
    REG_METHOD("java.lang.Math", "multiplyExact", ani_type_builtin(arena, "long"));
    REG_METHOD("java.lang.Math", "toRadians", ani_type_builtin(arena, "double"));
    REG_METHOD("java.lang.Math", "toDegrees", ani_type_builtin(arena, "double"));

    /* ── System ───────────────────────────────────────────────── */
    REG_METHOD("java.lang.System", "currentTimeMillis", ani_type_builtin(arena, "long"));
    REG_METHOD("java.lang.System", "nanoTime", ani_type_builtin(arena, "long"));
    REG_METHOD("java.lang.System", "exit", ani_type_builtin(arena, "void"));
    REG_METHOD("java.lang.System", "getenv", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.System", "getProperty", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.System", "setProperty", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.System", "lineSeparator", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.System", "arraycopy", ani_type_builtin(arena, "void"));
    REG_METHOD("java.lang.System", "identityHashCode", ani_type_builtin(arena, "int"));
    REG_METHOD("java.lang.System", "gc", ani_type_builtin(arena, "void"));

    /* ── Thread ───────────────────────────────────────────────── */
    REG_METHOD("java.lang.Thread", "start", ani_type_builtin(arena, "void"));
    REG_METHOD("java.lang.Thread", "run", ani_type_builtin(arena, "void"));
    REG_METHOD("java.lang.Thread", "join", ani_type_builtin(arena, "void"));
    REG_METHOD("java.lang.Thread", "interrupt", ani_type_builtin(arena, "void"));
    REG_METHOD("java.lang.Thread", "isAlive", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.lang.Thread", "sleep", ani_type_builtin(arena, "void"));
    REG_METHOD("java.lang.Thread", "currentThread", ani_type_named(arena, "java.lang.Thread"));
    REG_METHOD("java.lang.Thread", "yield", ani_type_builtin(arena, "void"));
    REG_METHOD("java.lang.Thread", "getName", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.Thread", "setName", ani_type_builtin(arena, "void"));
    REG_METHOD("java.lang.Thread", "getId", ani_type_builtin(arena, "long"));
    REG_METHOD("java.lang.Thread", "isInterrupted", ani_type_builtin(arena, "boolean"));

    /* ── Class ────────────────────────────────────────────────── */
    REG_METHOD("java.lang.Class", "getName", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.Class", "getSimpleName", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.Class", "getCanonicalName", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.Class", "isInterface", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.lang.Class", "isArray", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.lang.Class", "isAssignableFrom", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.lang.Class", "isInstance", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.lang.Class", "newInstance", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.lang.Class", "forName", ani_type_named(arena, "java.lang.Class"));
    REG_METHOD("java.lang.Class", "getSuperclass", ani_type_named(arena, "java.lang.Class"));
    REG_METHOD("java.lang.Class", "getInterfaces",
               ani_type_slice(arena, ani_type_named(arena, "java.lang.Class")));

    /* ── Iterable / Iterator ──────────────────────────────────── */
    REG_METHOD("java.lang.Iterable", "iterator", ani_type_named(arena, "java.util.Iterator"));
    REG_METHOD("java.lang.Iterable", "forEach", ani_type_builtin(arena, "void"));

    /* ── Throwable methods ────────────────────────────────────── */
    REG_METHOD("java.lang.Throwable", "getMessage", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.Throwable", "getLocalizedMessage",
               ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.Throwable", "getCause", ani_type_named(arena, "java.lang.Throwable"));
    REG_METHOD("java.lang.Throwable", "initCause",
               ani_type_named(arena, "java.lang.Throwable"));
    REG_METHOD("java.lang.Throwable", "printStackTrace", ani_type_builtin(arena, "void"));
    REG_METHOD("java.lang.Throwable", "toString", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.lang.Throwable", "getStackTrace",
               ani_type_slice(arena, ani_type_named(arena, "java.lang.StackTraceElement")));

    /* ── AutoCloseable ────────────────────────────────────────── */
    REG_METHOD("java.lang.AutoCloseable", "close", ani_type_builtin(arena, "void"));

    /* ── Comparable ───────────────────────────────────────────── */
    REG_METHOD("java.lang.Comparable", "compareTo", ani_type_builtin(arena, "int"));

    /* ── Runnable ─────────────────────────────────────────────── */
    REG_METHOD("java.lang.Runnable", "run", ani_type_builtin(arena, "void"));

    /* ── java.util ────────────────────────────────────────────── */
    REG_TYPE("java.util.Collection", "Collection", true, parents_collection);
    REG_TYPE("java.util.List", "List", true, parents_list);
    REG_TYPE("java.util.Set", "Set", true, parents_set);
    REG_TYPE("java.util.Queue", "Queue", true, parents_queue);
    REG_TYPE("java.util.Deque", "Deque", true, parents_deque);
    REG_TYPE("java.util.Map", "Map", true, parents_map);
    REG_TYPE("java.util.Map.Entry", "Entry", true, parents_map_entry);
    REG_TYPE("java.util.Iterator", "Iterator", true, parents_iterator);
    REG_TYPE("java.util.ListIterator", "ListIterator", true, parents_iterator);
    REG_TYPE("java.util.Spliterator", "Spliterator", true, no_parents);
    REG_TYPE("java.util.Comparator", "Comparator", true, no_parents);

    REG_TYPE("java.util.ArrayList", "ArrayList", false, parents_arraylist);
    REG_TYPE("java.util.LinkedList", "LinkedList", false, parents_linkedlist);
    REG_TYPE("java.util.Vector", "Vector", false, parents_arraylist);
    REG_TYPE("java.util.Stack", "Stack", false, parents_arraylist);
    REG_TYPE("java.util.HashSet", "HashSet", false, parents_hashset);
    REG_TYPE("java.util.TreeSet", "TreeSet", false, parents_treeset);
    REG_TYPE("java.util.LinkedHashSet", "LinkedHashSet", false, parents_linkedhashset);
    REG_TYPE("java.util.HashMap", "HashMap", false, parents_hashmap);
    REG_TYPE("java.util.TreeMap", "TreeMap", false, parents_treemap);
    REG_TYPE("java.util.LinkedHashMap", "LinkedHashMap", false, parents_linkedhashmap);
    REG_TYPE("java.util.ArrayDeque", "ArrayDeque", false, parents_deque);
    REG_TYPE("java.util.PriorityQueue", "PriorityQueue", false, parents_queue);

    REG_TYPE("java.util.Optional", "Optional", false, parents_object);
    REG_TYPE("java.util.OptionalInt", "OptionalInt", false, parents_object);
    REG_TYPE("java.util.OptionalLong", "OptionalLong", false, parents_object);
    REG_TYPE("java.util.OptionalDouble", "OptionalDouble", false, parents_object);
    REG_TYPE("java.util.Date", "Date", false, parents_object);
    REG_TYPE("java.util.Calendar", "Calendar", false, parents_object);
    REG_TYPE("java.util.GregorianCalendar", "GregorianCalendar", false,
             parents_gregorian_calendar);
    REG_TYPE("java.util.TimeZone", "TimeZone", false, parents_object);
    REG_TYPE("java.util.Locale", "Locale", false, parents_object);
    REG_TYPE("java.util.UUID", "UUID", false, parents_object);
    REG_TYPE("java.util.Random", "Random", false, parents_object);
    REG_TYPE("java.util.Scanner", "Scanner", false, parents_object);
    REG_TYPE("java.util.Arrays", "Arrays", false, parents_object);
    REG_TYPE("java.util.Collections", "Collections", false, parents_object);
    REG_TYPE("java.util.Objects", "Objects", false, parents_object);
    REG_TYPE("java.util.Properties", "Properties", false, parents_hashmap);
    REG_TYPE("java.util.regex.Pattern", "Pattern", false, parents_object);
    REG_TYPE("java.util.regex.Matcher", "Matcher", false, parents_object);

    /* ── Collection methods ───────────────────────────────────── */
    REG_METHOD("java.util.Collection", "size", ani_type_builtin(arena, "int"));
    REG_METHOD("java.util.Collection", "isEmpty", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Collection", "contains", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Collection", "containsAll", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Collection", "iterator", ani_type_named(arena, "java.util.Iterator"));
    REG_METHOD("java.util.Collection", "toArray",
               ani_type_slice(arena, ani_type_named(arena, "java.lang.Object")));
    REG_METHOD("java.util.Collection", "add", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Collection", "addAll", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Collection", "remove", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Collection", "removeAll", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Collection", "retainAll", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Collection", "clear", ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.Collection", "stream",
               ani_type_named(arena, "java.util.stream.Stream"));
    REG_METHOD("java.util.Collection", "parallelStream",
               ani_type_named(arena, "java.util.stream.Stream"));
    REG_METHOD("java.util.Collection", "forEach", ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.Collection", "removeIf", ani_type_builtin(arena, "boolean"));

    /* ── List methods ─────────────────────────────────────────── */
    REG_METHOD("java.util.List", "get", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.List", "set", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.List", "add", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.List", "remove", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.List", "indexOf", ani_type_builtin(arena, "int"));
    REG_METHOD("java.util.List", "lastIndexOf", ani_type_builtin(arena, "int"));
    REG_METHOD("java.util.List", "subList", ani_type_named(arena, "java.util.List"));
    REG_METHOD("java.util.List", "of", ani_type_named(arena, "java.util.List"));
    REG_METHOD("java.util.List", "copyOf", ani_type_named(arena, "java.util.List"));
    REG_METHOD("java.util.List", "size", ani_type_builtin(arena, "int"));
    REG_METHOD("java.util.List", "isEmpty", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.List", "contains", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.List", "iterator", ani_type_named(arena, "java.util.Iterator"));
    REG_METHOD("java.util.List", "stream",
               ani_type_named(arena, "java.util.stream.Stream"));
    REG_METHOD("java.util.List", "forEach", ani_type_builtin(arena, "void"));

    /* ── ArrayList ────────────────────────────────────────────── */
    REG_METHOD("java.util.ArrayList", "get", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.ArrayList", "set", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.ArrayList", "add", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.ArrayList", "remove", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.ArrayList", "size", ani_type_builtin(arena, "int"));
    REG_METHOD("java.util.ArrayList", "isEmpty", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.ArrayList", "indexOf", ani_type_builtin(arena, "int"));
    REG_METHOD("java.util.ArrayList", "iterator", ani_type_named(arena, "java.util.Iterator"));
    REG_METHOD("java.util.ArrayList", "clear", ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.ArrayList", "stream",
               ani_type_named(arena, "java.util.stream.Stream"));
    REG_METHOD("java.util.ArrayList", "toArray",
               ani_type_slice(arena, ani_type_named(arena, "java.lang.Object")));
    REG_METHOD("java.util.ArrayList", "subList", ani_type_named(arena, "java.util.List"));
    REG_METHOD("java.util.ArrayList", "trimToSize", ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.ArrayList", "ensureCapacity", ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.ArrayList", "forEach", ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.ArrayList", "removeIf", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.List", "removeIf", ani_type_builtin(arena, "boolean"));
    REG_CTOR("java.util.ArrayList", "ArrayList");

    REG_METHOD("java.util.LinkedList", "addFirst", ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.LinkedList", "addLast", ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.LinkedList", "removeFirst", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.LinkedList", "removeLast", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.LinkedList", "getFirst", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.LinkedList", "getLast", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.LinkedList", "peek", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.LinkedList", "poll", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.LinkedList", "offer", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.LinkedList", "size", ani_type_builtin(arena, "int"));
    REG_METHOD("java.util.LinkedList", "iterator", ani_type_named(arena, "java.util.Iterator"));
    REG_CTOR("java.util.LinkedList", "LinkedList");

    /* ── Set methods ──────────────────────────────────────────── */
    REG_METHOD("java.util.Set", "size", ani_type_builtin(arena, "int"));
    REG_METHOD("java.util.Set", "isEmpty", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Set", "contains", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Set", "add", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Set", "remove", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Set", "iterator", ani_type_named(arena, "java.util.Iterator"));
    REG_METHOD("java.util.Set", "of", ani_type_named(arena, "java.util.Set"));
    REG_METHOD("java.util.Set", "copyOf", ani_type_named(arena, "java.util.Set"));
    REG_METHOD("java.util.Set", "stream",
               ani_type_named(arena, "java.util.stream.Stream"));
    REG_METHOD("java.util.Set", "forEach", ani_type_builtin(arena, "void"));

    REG_METHOD("java.util.HashSet", "add", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.HashSet", "remove", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.HashSet", "contains", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.HashSet", "size", ani_type_builtin(arena, "int"));
    REG_METHOD("java.util.HashSet", "isEmpty", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.HashSet", "iterator", ani_type_named(arena, "java.util.Iterator"));
    REG_METHOD("java.util.HashSet", "clear", ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.HashSet", "stream",
               ani_type_named(arena, "java.util.stream.Stream"));
    REG_CTOR("java.util.HashSet", "HashSet");

    REG_METHOD("java.util.TreeSet", "first", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.TreeSet", "last", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.TreeSet", "headSet", ani_type_named(arena, "java.util.SortedSet"));
    REG_METHOD("java.util.TreeSet", "tailSet", ani_type_named(arena, "java.util.SortedSet"));
    REG_CTOR("java.util.TreeSet", "TreeSet");

    REG_CTOR("java.util.LinkedHashSet", "LinkedHashSet");

    /* ── Map methods ──────────────────────────────────────────── */
    REG_METHOD("java.util.Map", "get", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.Map", "put", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.Map", "remove", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.Map", "containsKey", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Map", "containsValue", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Map", "keySet", ani_type_named(arena, "java.util.Set"));
    REG_METHOD("java.util.Map", "values", ani_type_named(arena, "java.util.Collection"));
    REG_METHOD("java.util.Map", "entrySet", ani_type_named(arena, "java.util.Set"));
    REG_METHOD("java.util.Map", "size", ani_type_builtin(arena, "int"));
    REG_METHOD("java.util.Map", "isEmpty", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Map", "putAll", ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.Map", "clear", ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.Map", "getOrDefault", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.Map", "putIfAbsent", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.Map", "computeIfAbsent", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.Map", "compute", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.Map", "merge", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.Map", "of", ani_type_named(arena, "java.util.Map"));
    REG_METHOD("java.util.Map", "copyOf", ani_type_named(arena, "java.util.Map"));
    REG_METHOD("java.util.Map", "ofEntries", ani_type_named(arena, "java.util.Map"));
    REG_METHOD("java.util.Map", "entry", ani_type_named(arena, "java.util.Map.Entry"));
    REG_METHOD("java.util.Map", "forEach", ani_type_builtin(arena, "void"));

    REG_METHOD("java.util.Map.Entry", "getKey", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.Map.Entry", "getValue", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.Map.Entry", "setValue", ani_type_named(arena, "java.lang.Object"));

    REG_METHOD("java.util.HashMap", "get", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.HashMap", "put", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.HashMap", "remove", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.HashMap", "containsKey", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.HashMap", "containsValue", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.HashMap", "size", ani_type_builtin(arena, "int"));
    REG_METHOD("java.util.HashMap", "isEmpty", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.HashMap", "keySet", ani_type_named(arena, "java.util.Set"));
    REG_METHOD("java.util.HashMap", "values", ani_type_named(arena, "java.util.Collection"));
    REG_METHOD("java.util.HashMap", "entrySet", ani_type_named(arena, "java.util.Set"));
    REG_METHOD("java.util.HashMap", "clear", ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.HashMap", "getOrDefault", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.HashMap", "putIfAbsent", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.HashMap", "forEach", ani_type_builtin(arena, "void"));
    REG_CTOR("java.util.HashMap", "HashMap");

    REG_METHOD("java.util.TreeMap", "firstKey", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.TreeMap", "lastKey", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.TreeMap", "headMap", ani_type_named(arena, "java.util.SortedMap"));
    REG_METHOD("java.util.TreeMap", "tailMap", ani_type_named(arena, "java.util.SortedMap"));
    REG_CTOR("java.util.TreeMap", "TreeMap");

    REG_CTOR("java.util.LinkedHashMap", "LinkedHashMap");

    /* ── Iterator methods ─────────────────────────────────────── */
    REG_METHOD("java.util.Iterator", "hasNext", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Iterator", "next", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.Iterator", "remove", ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.Iterator", "forEachRemaining", ani_type_builtin(arena, "void"));

    /* ── Optional ─────────────────────────────────────────────── */
    REG_METHOD("java.util.Optional", "get", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.Optional", "isPresent", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Optional", "isEmpty", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Optional", "orElse", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.Optional", "orElseGet", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.Optional", "orElseThrow", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.Optional", "ifPresent", ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.Optional", "ifPresentOrElse", ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.Optional", "map", ani_type_named(arena, "java.util.Optional"));
    REG_METHOD("java.util.Optional", "flatMap", ani_type_named(arena, "java.util.Optional"));
    REG_METHOD("java.util.Optional", "filter", ani_type_named(arena, "java.util.Optional"));
    REG_METHOD("java.util.Optional", "of", ani_type_named(arena, "java.util.Optional"));
    REG_METHOD("java.util.Optional", "ofNullable", ani_type_named(arena, "java.util.Optional"));
    REG_METHOD("java.util.Optional", "empty", ani_type_named(arena, "java.util.Optional"));
    REG_METHOD("java.util.Optional", "stream",
               ani_type_named(arena, "java.util.stream.Stream"));

    /* ── Arrays / Collections / Objects helpers ───────────────── */
    REG_METHOD("java.util.Arrays", "asList", ani_type_named(arena, "java.util.List"));
    REG_METHOD("java.util.Arrays", "stream",
               ani_type_named(arena, "java.util.stream.Stream"));
    REG_METHOD("java.util.Arrays", "sort", ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.Arrays", "binarySearch", ani_type_builtin(arena, "int"));
    REG_METHOD("java.util.Arrays", "fill", ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.Arrays", "copyOf",
               ani_type_slice(arena, ani_type_named(arena, "java.lang.Object")));
    REG_METHOD("java.util.Arrays", "copyOfRange",
               ani_type_slice(arena, ani_type_named(arena, "java.lang.Object")));
    REG_METHOD("java.util.Arrays", "equals", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Arrays", "hashCode", ani_type_builtin(arena, "int"));
    REG_METHOD("java.util.Arrays", "toString", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.util.Arrays", "deepEquals", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Arrays", "deepToString", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.util.Arrays", "deepHashCode", ani_type_builtin(arena, "int"));

    REG_METHOD("java.util.Collections", "sort", ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.Collections", "reverse", ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.Collections", "shuffle", ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.Collections", "min", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.Collections", "max", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.Collections", "emptyList", ani_type_named(arena, "java.util.List"));
    REG_METHOD("java.util.Collections", "emptySet", ani_type_named(arena, "java.util.Set"));
    REG_METHOD("java.util.Collections", "emptyMap", ani_type_named(arena, "java.util.Map"));
    REG_METHOD("java.util.Collections", "singletonList",
               ani_type_named(arena, "java.util.List"));
    REG_METHOD("java.util.Collections", "singleton",
               ani_type_named(arena, "java.util.Set"));
    REG_METHOD("java.util.Collections", "unmodifiableList",
               ani_type_named(arena, "java.util.List"));
    REG_METHOD("java.util.Collections", "unmodifiableSet",
               ani_type_named(arena, "java.util.Set"));
    REG_METHOD("java.util.Collections", "unmodifiableMap",
               ani_type_named(arena, "java.util.Map"));
    REG_METHOD("java.util.Collections", "frequency", ani_type_builtin(arena, "int"));
    REG_METHOD("java.util.Collections", "binarySearch", ani_type_builtin(arena, "int"));

    REG_METHOD("java.util.Objects", "equals", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Objects", "hashCode", ani_type_builtin(arena, "int"));
    REG_METHOD("java.util.Objects", "hash", ani_type_builtin(arena, "int"));
    REG_METHOD("java.util.Objects", "toString", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.util.Objects", "isNull", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Objects", "nonNull", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Objects", "requireNonNull", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.Objects", "requireNonNullElse",
               ani_type_named(arena, "java.lang.Object"));

    /* ── UUID, Random, Scanner ────────────────────────────────── */
    REG_METHOD("java.util.UUID", "randomUUID", ani_type_named(arena, "java.util.UUID"));
    REG_METHOD("java.util.UUID", "fromString", ani_type_named(arena, "java.util.UUID"));
    REG_METHOD("java.util.UUID", "toString", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.util.UUID", "getMostSignificantBits", ani_type_builtin(arena, "long"));
    REG_METHOD("java.util.UUID", "getLeastSignificantBits", ani_type_builtin(arena, "long"));
    REG_CTOR("java.util.UUID", "UUID");

    REG_METHOD("java.util.Random", "nextInt", ani_type_builtin(arena, "int"));
    REG_METHOD("java.util.Random", "nextLong", ani_type_builtin(arena, "long"));
    REG_METHOD("java.util.Random", "nextDouble", ani_type_builtin(arena, "double"));
    REG_METHOD("java.util.Random", "nextFloat", ani_type_builtin(arena, "float"));
    REG_METHOD("java.util.Random", "nextBoolean", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Random", "nextGaussian", ani_type_builtin(arena, "double"));
    REG_METHOD("java.util.Random", "setSeed", ani_type_builtin(arena, "void"));
    REG_CTOR("java.util.Random", "Random");

    REG_METHOD("java.util.Scanner", "next", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.util.Scanner", "nextLine", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.util.Scanner", "nextInt", ani_type_builtin(arena, "int"));
    REG_METHOD("java.util.Scanner", "nextLong", ani_type_builtin(arena, "long"));
    REG_METHOD("java.util.Scanner", "nextDouble", ani_type_builtin(arena, "double"));
    REG_METHOD("java.util.Scanner", "hasNext", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Scanner", "hasNextLine", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Scanner", "hasNextInt", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Scanner", "close", ani_type_builtin(arena, "void"));
    REG_CTOR("java.util.Scanner", "Scanner");

    /* ── Locale / Date / Calendar / TimeZone ──────────────────── */
    REG_METHOD("java.util.Locale", "getLanguage", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.util.Locale", "getCountry", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.util.Locale", "toString", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.util.Locale", "getDefault", ani_type_named(arena, "java.util.Locale"));
    REG_CTOR("java.util.Locale", "Locale");

    REG_METHOD("java.util.Date", "getTime", ani_type_builtin(arena, "long"));
    REG_METHOD("java.util.Date", "setTime", ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.Date", "before", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Date", "after", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.Date", "compareTo", ani_type_builtin(arena, "int"));
    REG_METHOD("java.util.Date", "toString", ani_type_named(arena, "java.lang.String"));
    REG_CTOR("java.util.Date", "Date");

    REG_METHOD("java.util.Calendar", "getInstance", ani_type_named(arena, "java.util.Calendar"));
    REG_METHOD("java.util.Calendar", "getTime", ani_type_named(arena, "java.util.Date"));
    REG_METHOD("java.util.Calendar", "set", ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.Calendar", "get", ani_type_builtin(arena, "int"));
    REG_METHOD("java.util.Calendar", "add", ani_type_builtin(arena, "void"));

    REG_METHOD("java.util.TimeZone", "getDefault", ani_type_named(arena, "java.util.TimeZone"));
    REG_METHOD("java.util.TimeZone", "getID", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.util.TimeZone", "getTimeZone", ani_type_named(arena, "java.util.TimeZone"));

    /* ── regex ────────────────────────────────────────────────── */
    REG_METHOD("java.util.regex.Pattern", "compile",
               ani_type_named(arena, "java.util.regex.Pattern"));
    REG_METHOD("java.util.regex.Pattern", "matcher",
               ani_type_named(arena, "java.util.regex.Matcher"));
    REG_METHOD("java.util.regex.Pattern", "matches", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.regex.Pattern", "split",
               ani_type_slice(arena, ani_type_named(arena, "java.lang.String")));
    REG_METHOD("java.util.regex.Pattern", "pattern", ani_type_named(arena, "java.lang.String"));

    REG_METHOD("java.util.regex.Matcher", "matches", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.regex.Matcher", "find", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.regex.Matcher", "group", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.util.regex.Matcher", "groupCount", ani_type_builtin(arena, "int"));
    REG_METHOD("java.util.regex.Matcher", "start", ani_type_builtin(arena, "int"));
    REG_METHOD("java.util.regex.Matcher", "end", ani_type_builtin(arena, "int"));
    REG_METHOD("java.util.regex.Matcher", "replaceAll", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.util.regex.Matcher", "replaceFirst",
               ani_type_named(arena, "java.lang.String"));

    /* ── java.io ──────────────────────────────────────────────── */
    REG_TYPE("java.io.InputStream", "InputStream", false, parents_inputstream);
    REG_TYPE("java.io.OutputStream", "OutputStream", false, parents_outputstream);
    REG_TYPE("java.io.Reader", "Reader", false, parents_reader);
    REG_TYPE("java.io.Writer", "Writer", false, parents_writer);
    REG_TYPE("java.io.BufferedReader", "BufferedReader", false, parents_buffered_reader);
    REG_TYPE("java.io.BufferedWriter", "BufferedWriter", false, parents_buffered_writer);
    REG_TYPE("java.io.PrintStream", "PrintStream", false, parents_print_stream);
    REG_TYPE("java.io.PrintWriter", "PrintWriter", false, parents_print_writer);
    REG_TYPE("java.io.FileInputStream", "FileInputStream", false, parents_file_input_stream);
    REG_TYPE("java.io.FileOutputStream", "FileOutputStream", false, parents_file_output_stream);
    REG_TYPE("java.io.FileReader", "FileReader", false, parents_file_reader);
    REG_TYPE("java.io.FileWriter", "FileWriter", false, parents_file_writer);
    REG_TYPE("java.io.File", "File", false, parents_object);
    REG_TYPE("java.io.IOException", "IOException", false, parents_io_exception);
    REG_TYPE("java.io.FileNotFoundException", "FileNotFoundException", false,
             parents_file_not_found_exc);
    REG_TYPE("java.io.UncheckedIOException", "UncheckedIOException", false,
             parents_runtime_exc_chain);
    REG_TYPE("java.io.Serializable", "Serializable", true, no_parents);
    REG_TYPE("java.io.Closeable", "Closeable", true,
             parents_closeable);
    REG_TYPE("java.io.Flushable", "Flushable", true, no_parents);

    REG_METHOD("java.io.PrintStream", "println", ani_type_builtin(arena, "void"));
    REG_METHOD("java.io.PrintStream", "print", ani_type_builtin(arena, "void"));
    REG_METHOD("java.io.PrintStream", "printf", ani_type_named(arena, "java.io.PrintStream"));
    REG_METHOD("java.io.PrintStream", "format", ani_type_named(arena, "java.io.PrintStream"));
    REG_METHOD("java.io.PrintStream", "write", ani_type_builtin(arena, "void"));
    REG_METHOD("java.io.PrintStream", "flush", ani_type_builtin(arena, "void"));
    REG_METHOD("java.io.PrintStream", "close", ani_type_builtin(arena, "void"));

    REG_METHOD("java.io.PrintWriter", "println", ani_type_builtin(arena, "void"));
    REG_METHOD("java.io.PrintWriter", "print", ani_type_builtin(arena, "void"));
    REG_METHOD("java.io.PrintWriter", "printf", ani_type_named(arena, "java.io.PrintWriter"));
    REG_METHOD("java.io.PrintWriter", "flush", ani_type_builtin(arena, "void"));
    REG_METHOD("java.io.PrintWriter", "close", ani_type_builtin(arena, "void"));
    REG_CTOR("java.io.PrintWriter", "PrintWriter");

    REG_METHOD("java.io.InputStream", "read", ani_type_builtin(arena, "int"));
    REG_METHOD("java.io.InputStream", "close", ani_type_builtin(arena, "void"));
    REG_METHOD("java.io.InputStream", "available", ani_type_builtin(arena, "int"));
    REG_METHOD("java.io.InputStream", "skip", ani_type_builtin(arena, "long"));
    REG_METHOD("java.io.InputStream", "readAllBytes",
               ani_type_slice(arena, ani_type_builtin(arena, "byte")));

    REG_METHOD("java.io.OutputStream", "write", ani_type_builtin(arena, "void"));
    REG_METHOD("java.io.OutputStream", "flush", ani_type_builtin(arena, "void"));
    REG_METHOD("java.io.OutputStream", "close", ani_type_builtin(arena, "void"));

    REG_METHOD("java.io.Reader", "read", ani_type_builtin(arena, "int"));
    REG_METHOD("java.io.Reader", "close", ani_type_builtin(arena, "void"));
    REG_METHOD("java.io.Reader", "ready", ani_type_builtin(arena, "boolean"));

    REG_METHOD("java.io.Writer", "write", ani_type_builtin(arena, "void"));
    REG_METHOD("java.io.Writer", "flush", ani_type_builtin(arena, "void"));
    REG_METHOD("java.io.Writer", "close", ani_type_builtin(arena, "void"));

    REG_METHOD("java.io.BufferedReader", "readLine", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.io.BufferedReader", "lines",
               ani_type_named(arena, "java.util.stream.Stream"));
    REG_METHOD("java.io.BufferedReader", "close", ani_type_builtin(arena, "void"));
    REG_CTOR("java.io.BufferedReader", "BufferedReader");

    REG_METHOD("java.io.BufferedWriter", "write", ani_type_builtin(arena, "void"));
    REG_METHOD("java.io.BufferedWriter", "newLine", ani_type_builtin(arena, "void"));
    REG_METHOD("java.io.BufferedWriter", "flush", ani_type_builtin(arena, "void"));
    REG_METHOD("java.io.BufferedWriter", "close", ani_type_builtin(arena, "void"));
    REG_CTOR("java.io.BufferedWriter", "BufferedWriter");

    REG_METHOD("java.io.File", "exists", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.io.File", "isFile", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.io.File", "isDirectory", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.io.File", "canRead", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.io.File", "canWrite", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.io.File", "getName", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.io.File", "getPath", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.io.File", "getAbsolutePath", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.io.File", "getCanonicalPath", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.io.File", "getParent", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.io.File", "getParentFile", ani_type_named(arena, "java.io.File"));
    REG_METHOD("java.io.File", "length", ani_type_builtin(arena, "long"));
    REG_METHOD("java.io.File", "lastModified", ani_type_builtin(arena, "long"));
    REG_METHOD("java.io.File", "mkdir", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.io.File", "mkdirs", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.io.File", "delete", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.io.File", "renameTo", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.io.File", "list",
               ani_type_slice(arena, ani_type_named(arena, "java.lang.String")));
    REG_METHOD("java.io.File", "listFiles",
               ani_type_slice(arena, ani_type_named(arena, "java.io.File")));
    REG_METHOD("java.io.File", "toPath", ani_type_named(arena, "java.nio.file.Path"));
    REG_METHOD("java.io.File", "toURI", ani_type_named(arena, "java.net.URI"));
    REG_CTOR("java.io.File", "File");

    /* ── java.nio.file ───────────────────────────────────────── */
    REG_TYPE("java.nio.file.Path", "Path", true, no_parents);
    REG_TYPE("java.nio.file.Paths", "Paths", false, parents_object);
    REG_TYPE("java.nio.file.Files", "Files", false, parents_object);

    REG_METHOD("java.nio.file.Path", "getFileName", ani_type_named(arena, "java.nio.file.Path"));
    REG_METHOD("java.nio.file.Path", "getParent", ani_type_named(arena, "java.nio.file.Path"));
    REG_METHOD("java.nio.file.Path", "getRoot", ani_type_named(arena, "java.nio.file.Path"));
    REG_METHOD("java.nio.file.Path", "resolve", ani_type_named(arena, "java.nio.file.Path"));
    REG_METHOD("java.nio.file.Path", "resolveSibling",
               ani_type_named(arena, "java.nio.file.Path"));
    REG_METHOD("java.nio.file.Path", "relativize", ani_type_named(arena, "java.nio.file.Path"));
    REG_METHOD("java.nio.file.Path", "normalize", ani_type_named(arena, "java.nio.file.Path"));
    REG_METHOD("java.nio.file.Path", "toAbsolutePath",
               ani_type_named(arena, "java.nio.file.Path"));
    REG_METHOD("java.nio.file.Path", "toString", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.nio.file.Path", "toFile", ani_type_named(arena, "java.io.File"));
    REG_METHOD("java.nio.file.Path", "of", ani_type_named(arena, "java.nio.file.Path"));

    REG_METHOD("java.nio.file.Paths", "get", ani_type_named(arena, "java.nio.file.Path"));

    REG_METHOD("java.nio.file.Files", "exists", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.nio.file.Files", "isDirectory", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.nio.file.Files", "isRegularFile", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.nio.file.Files", "readString", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.nio.file.Files", "writeString", ani_type_named(arena, "java.nio.file.Path"));
    REG_METHOD("java.nio.file.Files", "readAllLines", ani_type_named(arena, "java.util.List"));
    REG_METHOD("java.nio.file.Files", "readAllBytes",
               ani_type_slice(arena, ani_type_builtin(arena, "byte")));
    REG_METHOD("java.nio.file.Files", "lines",
               ani_type_named(arena, "java.util.stream.Stream"));
    REG_METHOD("java.nio.file.Files", "list",
               ani_type_named(arena, "java.util.stream.Stream"));
    REG_METHOD("java.nio.file.Files", "walk",
               ani_type_named(arena, "java.util.stream.Stream"));
    REG_METHOD("java.nio.file.Files", "createDirectory",
               ani_type_named(arena, "java.nio.file.Path"));
    REG_METHOD("java.nio.file.Files", "createDirectories",
               ani_type_named(arena, "java.nio.file.Path"));
    REG_METHOD("java.nio.file.Files", "createFile",
               ani_type_named(arena, "java.nio.file.Path"));
    REG_METHOD("java.nio.file.Files", "delete", ani_type_builtin(arena, "void"));
    REG_METHOD("java.nio.file.Files", "deleteIfExists", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.nio.file.Files", "copy", ani_type_named(arena, "java.nio.file.Path"));
    REG_METHOD("java.nio.file.Files", "move", ani_type_named(arena, "java.nio.file.Path"));
    REG_METHOD("java.nio.file.Files", "size", ani_type_builtin(arena, "long"));

    /* ── java.util.function (the 21 functional interfaces) ──── */
    REG_TYPE("java.util.function.Function", "Function", true, no_parents);
    REG_TYPE("java.util.function.BiFunction", "BiFunction", true, no_parents);
    REG_TYPE("java.util.function.Predicate", "Predicate", true, no_parents);
    REG_TYPE("java.util.function.BiPredicate", "BiPredicate", true, no_parents);
    REG_TYPE("java.util.function.Consumer", "Consumer", true, no_parents);
    REG_TYPE("java.util.function.BiConsumer", "BiConsumer", true, no_parents);
    REG_TYPE("java.util.function.Supplier", "Supplier", true, no_parents);
    REG_TYPE("java.util.function.UnaryOperator", "UnaryOperator", true,
             parents_unary_operator);
    REG_TYPE("java.util.function.BinaryOperator", "BinaryOperator", true,
             parents_binary_operator);
    REG_TYPE("java.util.function.IntFunction", "IntFunction", true, no_parents);
    REG_TYPE("java.util.function.LongFunction", "LongFunction", true, no_parents);
    REG_TYPE("java.util.function.DoubleFunction", "DoubleFunction", true, no_parents);
    REG_TYPE("java.util.function.IntPredicate", "IntPredicate", true, no_parents);
    REG_TYPE("java.util.function.LongPredicate", "LongPredicate", true, no_parents);
    REG_TYPE("java.util.function.DoublePredicate", "DoublePredicate", true, no_parents);
    REG_TYPE("java.util.function.IntConsumer", "IntConsumer", true, no_parents);
    REG_TYPE("java.util.function.LongConsumer", "LongConsumer", true, no_parents);
    REG_TYPE("java.util.function.DoubleConsumer", "DoubleConsumer", true, no_parents);
    REG_TYPE("java.util.function.IntSupplier", "IntSupplier", true, no_parents);
    REG_TYPE("java.util.function.LongSupplier", "LongSupplier", true, no_parents);
    REG_TYPE("java.util.function.DoubleSupplier", "DoubleSupplier", true, no_parents);
    REG_TYPE("java.util.function.BooleanSupplier", "BooleanSupplier", true, no_parents);
    REG_TYPE("java.util.function.ToIntFunction", "ToIntFunction", true, no_parents);
    REG_TYPE("java.util.function.ToLongFunction", "ToLongFunction", true, no_parents);
    REG_TYPE("java.util.function.ToDoubleFunction", "ToDoubleFunction", true, no_parents);

    REG_METHOD("java.util.function.Function", "apply", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.function.Function", "compose",
               ani_type_named(arena, "java.util.function.Function"));
    REG_METHOD("java.util.function.Function", "andThen",
               ani_type_named(arena, "java.util.function.Function"));
    REG_METHOD("java.util.function.Function", "identity",
               ani_type_named(arena, "java.util.function.Function"));

    REG_METHOD("java.util.function.BiFunction", "apply",
               ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.function.BiFunction", "andThen",
               ani_type_named(arena, "java.util.function.BiFunction"));

    REG_METHOD("java.util.function.Predicate", "test", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.function.Predicate", "and",
               ani_type_named(arena, "java.util.function.Predicate"));
    REG_METHOD("java.util.function.Predicate", "or",
               ani_type_named(arena, "java.util.function.Predicate"));
    REG_METHOD("java.util.function.Predicate", "negate",
               ani_type_named(arena, "java.util.function.Predicate"));
    REG_METHOD("java.util.function.Predicate", "isEqual",
               ani_type_named(arena, "java.util.function.Predicate"));

    REG_METHOD("java.util.function.Consumer", "accept", ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.function.Consumer", "andThen",
               ani_type_named(arena, "java.util.function.Consumer"));

    REG_METHOD("java.util.function.Supplier", "get", ani_type_named(arena, "java.lang.Object"));

    REG_METHOD("java.util.function.UnaryOperator", "identity",
               ani_type_named(arena, "java.util.function.UnaryOperator"));
    REG_METHOD("java.util.function.UnaryOperator", "apply",
               ani_type_named(arena, "java.lang.Object"));

    /* ── java.util.stream ────────────────────────────────────── */
    REG_TYPE("java.util.stream.Stream", "Stream", true, no_parents);
    REG_TYPE("java.util.stream.IntStream", "IntStream", true, no_parents);
    REG_TYPE("java.util.stream.LongStream", "LongStream", true, no_parents);
    REG_TYPE("java.util.stream.DoubleStream", "DoubleStream", true, no_parents);
    REG_TYPE("java.util.stream.Collectors", "Collectors", false, parents_object);
    REG_TYPE("java.util.stream.Collector", "Collector", true, no_parents);

    REG_METHOD("java.util.stream.Stream", "filter",
               ani_type_named(arena, "java.util.stream.Stream"));
    REG_METHOD("java.util.stream.Stream", "map",
               ani_type_named(arena, "java.util.stream.Stream"));
    REG_METHOD("java.util.stream.Stream", "flatMap",
               ani_type_named(arena, "java.util.stream.Stream"));
    REG_METHOD("java.util.stream.Stream", "mapToInt",
               ani_type_named(arena, "java.util.stream.IntStream"));
    REG_METHOD("java.util.stream.Stream", "mapToLong",
               ani_type_named(arena, "java.util.stream.LongStream"));
    REG_METHOD("java.util.stream.Stream", "mapToDouble",
               ani_type_named(arena, "java.util.stream.DoubleStream"));
    REG_METHOD("java.util.stream.Stream", "sorted",
               ani_type_named(arena, "java.util.stream.Stream"));
    REG_METHOD("java.util.stream.Stream", "distinct",
               ani_type_named(arena, "java.util.stream.Stream"));
    REG_METHOD("java.util.stream.Stream", "limit",
               ani_type_named(arena, "java.util.stream.Stream"));
    REG_METHOD("java.util.stream.Stream", "skip",
               ani_type_named(arena, "java.util.stream.Stream"));
    REG_METHOD("java.util.stream.Stream", "peek",
               ani_type_named(arena, "java.util.stream.Stream"));
    REG_METHOD("java.util.stream.Stream", "forEach", ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.stream.Stream", "forEachOrdered", ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.stream.Stream", "toArray",
               ani_type_slice(arena, ani_type_named(arena, "java.lang.Object")));
    REG_METHOD("java.util.stream.Stream", "toList", ani_type_named(arena, "java.util.List"));
    REG_METHOD("java.util.stream.Stream", "reduce", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.stream.Stream", "collect", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.stream.Stream", "count", ani_type_builtin(arena, "long"));
    REG_METHOD("java.util.stream.Stream", "anyMatch", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.stream.Stream", "allMatch", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.stream.Stream", "noneMatch", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.stream.Stream", "findFirst",
               ani_type_named(arena, "java.util.Optional"));
    REG_METHOD("java.util.stream.Stream", "findAny",
               ani_type_named(arena, "java.util.Optional"));
    REG_METHOD("java.util.stream.Stream", "min", ani_type_named(arena, "java.util.Optional"));
    REG_METHOD("java.util.stream.Stream", "max", ani_type_named(arena, "java.util.Optional"));
    REG_METHOD("java.util.stream.Stream", "of",
               ani_type_named(arena, "java.util.stream.Stream"));
    REG_METHOD("java.util.stream.Stream", "empty",
               ani_type_named(arena, "java.util.stream.Stream"));
    REG_METHOD("java.util.stream.Stream", "concat",
               ani_type_named(arena, "java.util.stream.Stream"));
    REG_METHOD("java.util.stream.Stream", "iterate",
               ani_type_named(arena, "java.util.stream.Stream"));
    REG_METHOD("java.util.stream.Stream", "generate",
               ani_type_named(arena, "java.util.stream.Stream"));

    REG_METHOD("java.util.stream.IntStream", "sum", ani_type_builtin(arena, "int"));
    REG_METHOD("java.util.stream.IntStream", "average",
               ani_type_named(arena, "java.util.OptionalDouble"));
    REG_METHOD("java.util.stream.IntStream", "max",
               ani_type_named(arena, "java.util.OptionalInt"));
    REG_METHOD("java.util.stream.IntStream", "min",
               ani_type_named(arena, "java.util.OptionalInt"));
    REG_METHOD("java.util.stream.IntStream", "count", ani_type_builtin(arena, "long"));
    REG_METHOD("java.util.stream.IntStream", "boxed",
               ani_type_named(arena, "java.util.stream.Stream"));
    REG_METHOD("java.util.stream.IntStream", "filter",
               ani_type_named(arena, "java.util.stream.IntStream"));
    REG_METHOD("java.util.stream.IntStream", "map",
               ani_type_named(arena, "java.util.stream.IntStream"));
    REG_METHOD("java.util.stream.IntStream", "range",
               ani_type_named(arena, "java.util.stream.IntStream"));
    REG_METHOD("java.util.stream.IntStream", "rangeClosed",
               ani_type_named(arena, "java.util.stream.IntStream"));
    REG_METHOD("java.util.stream.IntStream", "of",
               ani_type_named(arena, "java.util.stream.IntStream"));

    REG_METHOD("java.util.stream.Collectors", "toList",
               ani_type_named(arena, "java.util.stream.Collector"));
    REG_METHOD("java.util.stream.Collectors", "toSet",
               ani_type_named(arena, "java.util.stream.Collector"));
    REG_METHOD("java.util.stream.Collectors", "toMap",
               ani_type_named(arena, "java.util.stream.Collector"));
    REG_METHOD("java.util.stream.Collectors", "joining",
               ani_type_named(arena, "java.util.stream.Collector"));
    REG_METHOD("java.util.stream.Collectors", "groupingBy",
               ani_type_named(arena, "java.util.stream.Collector"));
    REG_METHOD("java.util.stream.Collectors", "partitioningBy",
               ani_type_named(arena, "java.util.stream.Collector"));
    REG_METHOD("java.util.stream.Collectors", "counting",
               ani_type_named(arena, "java.util.stream.Collector"));
    REG_METHOD("java.util.stream.Collectors", "summingInt",
               ani_type_named(arena, "java.util.stream.Collector"));
    REG_METHOD("java.util.stream.Collectors", "averagingDouble",
               ani_type_named(arena, "java.util.stream.Collector"));
    REG_METHOD("java.util.stream.Collectors", "mapping",
               ani_type_named(arena, "java.util.stream.Collector"));
    REG_METHOD("java.util.stream.Collectors", "reducing",
               ani_type_named(arena, "java.util.stream.Collector"));

    /* ── java.util.concurrent ────────────────────────────────── */
    REG_TYPE("java.util.concurrent.ExecutorService", "ExecutorService", true, no_parents);
    REG_TYPE("java.util.concurrent.Executors", "Executors", false, parents_object);
    REG_TYPE("java.util.concurrent.Future", "Future", true, no_parents);
    REG_TYPE("java.util.concurrent.CompletableFuture", "CompletableFuture", false,
             parents_completable_future);
    REG_TYPE("java.util.concurrent.ConcurrentHashMap", "ConcurrentHashMap", false,
             parents_concurrent_hashmap);
    REG_TYPE("java.util.concurrent.ConcurrentMap", "ConcurrentMap", true, parents_map);
    REG_TYPE("java.util.concurrent.TimeUnit", "TimeUnit", false, parents_object);
    REG_TYPE("java.util.concurrent.atomic.AtomicInteger", "AtomicInteger", false, parents_object);
    REG_TYPE("java.util.concurrent.atomic.AtomicLong", "AtomicLong", false, parents_object);
    REG_TYPE("java.util.concurrent.atomic.AtomicBoolean", "AtomicBoolean", false, parents_object);
    REG_TYPE("java.util.concurrent.atomic.AtomicReference", "AtomicReference", false,
             parents_object);
    REG_TYPE("java.util.concurrent.locks.Lock", "Lock", true, no_parents);
    REG_TYPE("java.util.concurrent.locks.ReentrantLock", "ReentrantLock", false,
             parents_reentrant_lock);
    REG_TYPE("java.util.concurrent.locks.ReadWriteLock", "ReadWriteLock", true, no_parents);

    REG_METHOD("java.util.concurrent.ExecutorService", "submit",
               ani_type_named(arena, "java.util.concurrent.Future"));
    REG_METHOD("java.util.concurrent.ExecutorService", "execute", ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.concurrent.ExecutorService", "shutdown", ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.concurrent.ExecutorService", "shutdownNow",
               ani_type_named(arena, "java.util.List"));
    REG_METHOD("java.util.concurrent.ExecutorService", "awaitTermination",
               ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.concurrent.ExecutorService", "isShutdown",
               ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.concurrent.ExecutorService", "isTerminated",
               ani_type_builtin(arena, "boolean"));

    REG_METHOD("java.util.concurrent.Executors", "newFixedThreadPool",
               ani_type_named(arena, "java.util.concurrent.ExecutorService"));
    REG_METHOD("java.util.concurrent.Executors", "newSingleThreadExecutor",
               ani_type_named(arena, "java.util.concurrent.ExecutorService"));
    REG_METHOD("java.util.concurrent.Executors", "newCachedThreadPool",
               ani_type_named(arena, "java.util.concurrent.ExecutorService"));
    REG_METHOD("java.util.concurrent.Executors", "newScheduledThreadPool",
               ani_type_named(arena, "java.util.concurrent.ExecutorService"));

    REG_METHOD("java.util.concurrent.Future", "get", ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.concurrent.Future", "isDone", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.concurrent.Future", "cancel", ani_type_builtin(arena, "boolean"));

    REG_METHOD("java.util.concurrent.CompletableFuture", "thenApply",
               ani_type_named(arena, "java.util.concurrent.CompletableFuture"));
    REG_METHOD("java.util.concurrent.CompletableFuture", "thenAccept",
               ani_type_named(arena, "java.util.concurrent.CompletableFuture"));
    REG_METHOD("java.util.concurrent.CompletableFuture", "thenCompose",
               ani_type_named(arena, "java.util.concurrent.CompletableFuture"));
    REG_METHOD("java.util.concurrent.CompletableFuture", "thenCombine",
               ani_type_named(arena, "java.util.concurrent.CompletableFuture"));
    REG_METHOD("java.util.concurrent.CompletableFuture", "exceptionally",
               ani_type_named(arena, "java.util.concurrent.CompletableFuture"));
    REG_METHOD("java.util.concurrent.CompletableFuture", "join",
               ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.concurrent.CompletableFuture", "supplyAsync",
               ani_type_named(arena, "java.util.concurrent.CompletableFuture"));
    REG_METHOD("java.util.concurrent.CompletableFuture", "runAsync",
               ani_type_named(arena, "java.util.concurrent.CompletableFuture"));
    REG_METHOD("java.util.concurrent.CompletableFuture", "completedFuture",
               ani_type_named(arena, "java.util.concurrent.CompletableFuture"));
    REG_METHOD("java.util.concurrent.CompletableFuture", "allOf",
               ani_type_named(arena, "java.util.concurrent.CompletableFuture"));
    REG_METHOD("java.util.concurrent.CompletableFuture", "anyOf",
               ani_type_named(arena, "java.util.concurrent.CompletableFuture"));

    REG_METHOD("java.util.concurrent.atomic.AtomicInteger", "get", ani_type_builtin(arena, "int"));
    REG_METHOD("java.util.concurrent.atomic.AtomicInteger", "set", ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.concurrent.atomic.AtomicInteger", "incrementAndGet",
               ani_type_builtin(arena, "int"));
    REG_METHOD("java.util.concurrent.atomic.AtomicInteger", "decrementAndGet",
               ani_type_builtin(arena, "int"));
    REG_METHOD("java.util.concurrent.atomic.AtomicInteger", "getAndIncrement",
               ani_type_builtin(arena, "int"));
    REG_METHOD("java.util.concurrent.atomic.AtomicInteger", "compareAndSet",
               ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.util.concurrent.atomic.AtomicInteger", "addAndGet",
               ani_type_builtin(arena, "int"));
    REG_CTOR("java.util.concurrent.atomic.AtomicInteger", "AtomicInteger");

    REG_METHOD("java.util.concurrent.atomic.AtomicLong", "get",
               ani_type_builtin(arena, "long"));
    REG_METHOD("java.util.concurrent.atomic.AtomicLong", "incrementAndGet",
               ani_type_builtin(arena, "long"));
    REG_CTOR("java.util.concurrent.atomic.AtomicLong", "AtomicLong");

    REG_METHOD("java.util.concurrent.atomic.AtomicReference", "get",
               ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.concurrent.atomic.AtomicReference", "set",
               ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.concurrent.atomic.AtomicReference", "compareAndSet",
               ani_type_builtin(arena, "boolean"));
    REG_CTOR("java.util.concurrent.atomic.AtomicReference", "AtomicReference");

    REG_METHOD("java.util.concurrent.locks.ReentrantLock", "lock",
               ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.concurrent.locks.ReentrantLock", "unlock",
               ani_type_builtin(arena, "void"));
    REG_METHOD("java.util.concurrent.locks.ReentrantLock", "tryLock",
               ani_type_builtin(arena, "boolean"));
    REG_CTOR("java.util.concurrent.locks.ReentrantLock", "ReentrantLock");

    REG_METHOD("java.util.concurrent.ConcurrentHashMap", "put",
               ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.concurrent.ConcurrentHashMap", "get",
               ani_type_named(arena, "java.lang.Object"));
    REG_METHOD("java.util.concurrent.ConcurrentHashMap", "putIfAbsent",
               ani_type_named(arena, "java.lang.Object"));
    REG_CTOR("java.util.concurrent.ConcurrentHashMap", "ConcurrentHashMap");

    /* ── java.time ───────────────────────────────────────────── */
    REG_TYPE("java.time.LocalDate", "LocalDate", false, parents_object);
    REG_TYPE("java.time.LocalTime", "LocalTime", false, parents_object);
    REG_TYPE("java.time.LocalDateTime", "LocalDateTime", false, parents_object);
    REG_TYPE("java.time.ZonedDateTime", "ZonedDateTime", false, parents_object);
    REG_TYPE("java.time.OffsetDateTime", "OffsetDateTime", false, parents_object);
    REG_TYPE("java.time.Instant", "Instant", false, parents_object);
    REG_TYPE("java.time.Duration", "Duration", false, parents_object);
    REG_TYPE("java.time.Period", "Period", false, parents_object);
    REG_TYPE("java.time.ZoneId", "ZoneId", false, parents_object);
    REG_TYPE("java.time.format.DateTimeFormatter", "DateTimeFormatter", false, parents_object);

    REG_METHOD("java.time.LocalDate", "now", ani_type_named(arena, "java.time.LocalDate"));
    REG_METHOD("java.time.LocalDate", "of", ani_type_named(arena, "java.time.LocalDate"));
    REG_METHOD("java.time.LocalDate", "parse", ani_type_named(arena, "java.time.LocalDate"));
    REG_METHOD("java.time.LocalDate", "plusDays", ani_type_named(arena, "java.time.LocalDate"));
    REG_METHOD("java.time.LocalDate", "minusDays", ani_type_named(arena, "java.time.LocalDate"));
    REG_METHOD("java.time.LocalDate", "getYear", ani_type_builtin(arena, "int"));
    REG_METHOD("java.time.LocalDate", "getMonth", ani_type_named(arena, "java.time.Month"));
    REG_METHOD("java.time.LocalDate", "getDayOfMonth", ani_type_builtin(arena, "int"));
    REG_METHOD("java.time.LocalDate", "getDayOfWeek",
               ani_type_named(arena, "java.time.DayOfWeek"));
    REG_METHOD("java.time.LocalDate", "isAfter", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.time.LocalDate", "isBefore", ani_type_builtin(arena, "boolean"));
    REG_METHOD("java.time.LocalDate", "format", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.time.LocalDate", "toString", ani_type_named(arena, "java.lang.String"));

    REG_METHOD("java.time.LocalDateTime", "now",
               ani_type_named(arena, "java.time.LocalDateTime"));
    REG_METHOD("java.time.LocalDateTime", "of",
               ani_type_named(arena, "java.time.LocalDateTime"));
    REG_METHOD("java.time.LocalDateTime", "parse",
               ani_type_named(arena, "java.time.LocalDateTime"));
    REG_METHOD("java.time.LocalDateTime", "plusHours",
               ani_type_named(arena, "java.time.LocalDateTime"));
    REG_METHOD("java.time.LocalDateTime", "minusHours",
               ani_type_named(arena, "java.time.LocalDateTime"));
    REG_METHOD("java.time.LocalDateTime", "format", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.time.LocalDateTime", "toString", ani_type_named(arena, "java.lang.String"));

    REG_METHOD("java.time.Instant", "now", ani_type_named(arena, "java.time.Instant"));
    REG_METHOD("java.time.Instant", "ofEpochMilli", ani_type_named(arena, "java.time.Instant"));
    REG_METHOD("java.time.Instant", "ofEpochSecond", ani_type_named(arena, "java.time.Instant"));
    REG_METHOD("java.time.Instant", "toEpochMilli", ani_type_builtin(arena, "long"));
    REG_METHOD("java.time.Instant", "getEpochSecond", ani_type_builtin(arena, "long"));
    REG_METHOD("java.time.Instant", "plus", ani_type_named(arena, "java.time.Instant"));
    REG_METHOD("java.time.Instant", "minus", ani_type_named(arena, "java.time.Instant"));

    REG_METHOD("java.time.Duration", "ofSeconds", ani_type_named(arena, "java.time.Duration"));
    REG_METHOD("java.time.Duration", "ofMillis", ani_type_named(arena, "java.time.Duration"));
    REG_METHOD("java.time.Duration", "ofMinutes", ani_type_named(arena, "java.time.Duration"));
    REG_METHOD("java.time.Duration", "ofHours", ani_type_named(arena, "java.time.Duration"));
    REG_METHOD("java.time.Duration", "ofDays", ani_type_named(arena, "java.time.Duration"));
    REG_METHOD("java.time.Duration", "between", ani_type_named(arena, "java.time.Duration"));
    REG_METHOD("java.time.Duration", "toMillis", ani_type_builtin(arena, "long"));
    REG_METHOD("java.time.Duration", "toSeconds", ani_type_builtin(arena, "long"));
    REG_METHOD("java.time.Duration", "toMinutes", ani_type_builtin(arena, "long"));

    REG_METHOD("java.time.ZoneId", "of", ani_type_named(arena, "java.time.ZoneId"));
    REG_METHOD("java.time.ZoneId", "systemDefault", ani_type_named(arena, "java.time.ZoneId"));
    REG_METHOD("java.time.ZoneId", "getId", ani_type_named(arena, "java.lang.String"));

    REG_METHOD("java.time.format.DateTimeFormatter", "ofPattern",
               ani_type_named(arena, "java.time.format.DateTimeFormatter"));
    REG_METHOD("java.time.format.DateTimeFormatter", "format",
               ani_type_named(arena, "java.lang.String"));

    /* ── java.net (minimal) ──────────────────────────────────── */
    REG_TYPE("java.net.URI", "URI", false, parents_object);
    REG_TYPE("java.net.URL", "URL", false, parents_object);
    REG_METHOD("java.net.URI", "create", ani_type_named(arena, "java.net.URI"));
    REG_METHOD("java.net.URI", "toString", ani_type_named(arena, "java.lang.String"));
    REG_METHOD("java.net.URI", "toURL", ani_type_named(arena, "java.net.URL"));
    REG_METHOD("java.net.URL", "openStream", ani_type_named(arena, "java.io.InputStream"));
    REG_METHOD("java.net.URL", "toString", ani_type_named(arena, "java.lang.String"));
    REG_CTOR("java.net.URL", "URL");
    REG_CTOR("java.net.URI", "URI");
}
