/*
 * test_language.c — Tests for language detection (filename + extension).
 *
 * RED phase: These tests define the expected behavior for registered languages.
 */
#include "../src/foundation/compat.h"
#include "test_framework.h"
#include "discover/discover.h"

/* ── Extension-based detection ─────────────────────────────────── */

TEST(lang_ext_go) {
    ASSERT_EQ(ani_language_for_extension(".go"), ANI_LANG_GO);
    PASS();
}
TEST(lang_ext_python) {
    ASSERT_EQ(ani_language_for_extension(".py"), ANI_LANG_PYTHON);
    PASS();
}
TEST(lang_ext_javascript) {
    ASSERT_EQ(ani_language_for_extension(".js"), ANI_LANG_JAVASCRIPT);
    PASS();
}
TEST(lang_ext_jsx) {
    ASSERT_EQ(ani_language_for_extension(".jsx"), ANI_LANG_JAVASCRIPT);
    PASS();
}
/* Issue #197: .mjs (ES modules) / .cjs (CommonJS) were unmapped, so those
 * files were never indexed or searchable. */
TEST(lang_ext_mjs_cjs) {
    ASSERT_EQ(ani_language_for_extension(".mjs"), ANI_LANG_JAVASCRIPT);
    ASSERT_EQ(ani_language_for_extension(".cjs"), ANI_LANG_JAVASCRIPT);
    PASS();
}
TEST(lang_ext_mts_cts) {
    ASSERT_EQ(ani_language_for_extension(".mts"), ANI_LANG_TYPESCRIPT);
    ASSERT_EQ(ani_language_for_extension(".cts"), ANI_LANG_TYPESCRIPT);
    PASS();
}
TEST(lang_ext_typescript) {
    ASSERT_EQ(ani_language_for_extension(".ts"), ANI_LANG_TYPESCRIPT);
    PASS();
}
TEST(lang_ext_tsx) {
    ASSERT_EQ(ani_language_for_extension(".tsx"), ANI_LANG_TSX);
    PASS();
}
TEST(lang_ext_rust) {
    ASSERT_EQ(ani_language_for_extension(".rs"), ANI_LANG_RUST);
    PASS();
}
TEST(lang_ext_java) {
    ASSERT_EQ(ani_language_for_extension(".java"), ANI_LANG_JAVA);
    PASS();
}
TEST(lang_ext_cpp) {
    ASSERT_EQ(ani_language_for_extension(".cpp"), ANI_LANG_CPP);
    PASS();
}
TEST(lang_ext_hpp) {
    ASSERT_EQ(ani_language_for_extension(".hpp"), ANI_LANG_CPP);
    PASS();
}
TEST(lang_ext_cc) {
    ASSERT_EQ(ani_language_for_extension(".cc"), ANI_LANG_CPP);
    PASS();
}
TEST(lang_ext_cxx) {
    ASSERT_EQ(ani_language_for_extension(".cxx"), ANI_LANG_CPP);
    PASS();
}
TEST(lang_ext_hxx) {
    ASSERT_EQ(ani_language_for_extension(".hxx"), ANI_LANG_CPP);
    PASS();
}
TEST(lang_ext_hh) {
    ASSERT_EQ(ani_language_for_extension(".hh"), ANI_LANG_CPP);
    PASS();
}
TEST(lang_ext_h) {
    ASSERT_EQ(ani_language_for_extension(".h"), ANI_LANG_CPP);
    PASS();
}
TEST(lang_ext_ixx) {
    ASSERT_EQ(ani_language_for_extension(".ixx"), ANI_LANG_CPP);
    PASS();
}
TEST(lang_ext_csharp) {
    ASSERT_EQ(ani_language_for_extension(".cs"), ANI_LANG_CSHARP);
    PASS();
}
/* Blazor components were unmapped, so a .razor file was never discovered at
 * all: indexing a Blazor app produced no nodes for any component, and reaching
 * them required an undocumented extra_extensions entry in a per-project
 * .ani.json. */
TEST(lang_ext_razor) {
    ASSERT_EQ(ani_language_for_extension(".razor"), ANI_LANG_CSHARP);
    PASS();
}
TEST(lang_ext_php) {
    ASSERT_EQ(ani_language_for_extension(".php"), ANI_LANG_PHP);
    PASS();
}
TEST(lang_ext_lua) {
    ASSERT_EQ(ani_language_for_extension(".lua"), ANI_LANG_LUA);
    PASS();
}
TEST(lang_ext_scala) {
    ASSERT_EQ(ani_language_for_extension(".scala"), ANI_LANG_SCALA);
    PASS();
}
TEST(lang_ext_sc) {
    ASSERT_EQ(ani_language_for_extension(".sc"), ANI_LANG_SCALA);
    PASS();
}
TEST(lang_ext_kotlin) {
    ASSERT_EQ(ani_language_for_extension(".kt"), ANI_LANG_KOTLIN);
    PASS();
}
TEST(lang_ext_kts) {
    ASSERT_EQ(ani_language_for_extension(".kts"), ANI_LANG_KOTLIN);
    PASS();
}
TEST(lang_ext_ruby) {
    ASSERT_EQ(ani_language_for_extension(".rb"), ANI_LANG_RUBY);
    PASS();
}
TEST(lang_ext_rake) {
    ASSERT_EQ(ani_language_for_extension(".rake"), ANI_LANG_RUBY);
    PASS();
}
TEST(lang_ext_gemspec) {
    ASSERT_EQ(ani_language_for_extension(".gemspec"), ANI_LANG_RUBY);
    PASS();
}
TEST(lang_ext_c) {
    ASSERT_EQ(ani_language_for_extension(".c"), ANI_LANG_C);
    PASS();
}
TEST(lang_ext_bash) {
    ASSERT_EQ(ani_language_for_extension(".sh"), ANI_LANG_BASH);
    PASS();
}
TEST(lang_ext_bash2) {
    ASSERT_EQ(ani_language_for_extension(".bash"), ANI_LANG_BASH);
    PASS();
}
TEST(lang_ext_zig) {
    ASSERT_EQ(ani_language_for_extension(".zig"), ANI_LANG_ZIG);
    PASS();
}
TEST(lang_ext_elixir) {
    ASSERT_EQ(ani_language_for_extension(".ex"), ANI_LANG_ELIXIR);
    PASS();
}
TEST(lang_ext_exs) {
    ASSERT_EQ(ani_language_for_extension(".exs"), ANI_LANG_ELIXIR);
    PASS();
}
TEST(lang_ext_haskell) {
    ASSERT_EQ(ani_language_for_extension(".hs"), ANI_LANG_HASKELL);
    PASS();
}
TEST(lang_ext_ocaml) {
    ASSERT_EQ(ani_language_for_extension(".ml"), ANI_LANG_OCAML);
    PASS();
}
TEST(lang_ext_mli) {
    ASSERT_EQ(ani_language_for_extension(".mli"), ANI_LANG_OCAML);
    PASS();
}
TEST(lang_ext_swift) {
    ASSERT_EQ(ani_language_for_extension(".swift"), ANI_LANG_SWIFT);
    PASS();
}
TEST(lang_ext_dart) {
    ASSERT_EQ(ani_language_for_extension(".dart"), ANI_LANG_DART);
    PASS();
}
TEST(lang_ext_perl) {
    ASSERT_EQ(ani_language_for_extension(".pl"), ANI_LANG_PERL);
    PASS();
}
TEST(lang_ext_pm) {
    ASSERT_EQ(ani_language_for_extension(".pm"), ANI_LANG_PERL);
    PASS();
}
TEST(lang_ext_groovy) {
    ASSERT_EQ(ani_language_for_extension(".groovy"), ANI_LANG_GROOVY);
    PASS();
}
TEST(lang_ext_gradle) {
    ASSERT_EQ(ani_language_for_extension(".gradle"), ANI_LANG_GROOVY);
    PASS();
}
TEST(lang_ext_erlang) {
    ASSERT_EQ(ani_language_for_extension(".erl"), ANI_LANG_ERLANG);
    PASS();
}
TEST(lang_ext_r) {
    ASSERT_EQ(ani_language_for_extension(".r"), ANI_LANG_R);
    PASS();
}
TEST(lang_ext_R) {
    ASSERT_EQ(ani_language_for_extension(".R"), ANI_LANG_R);
    PASS();
}

/* Tier 2 programming */
TEST(lang_ext_clojure) {
    ASSERT_EQ(ani_language_for_extension(".clj"), ANI_LANG_CLOJURE);
    PASS();
}
TEST(lang_ext_cljs) {
    ASSERT_EQ(ani_language_for_extension(".cljs"), ANI_LANG_CLOJURE);
    PASS();
}
TEST(lang_ext_cljc) {
    ASSERT_EQ(ani_language_for_extension(".cljc"), ANI_LANG_CLOJURE);
    PASS();
}
TEST(lang_ext_fsharp) {
    ASSERT_EQ(ani_language_for_extension(".fs"), ANI_LANG_FSHARP);
    PASS();
}
TEST(lang_ext_fsi) {
    ASSERT_EQ(ani_language_for_extension(".fsi"), ANI_LANG_FSHARP);
    PASS();
}
TEST(lang_ext_fsx) {
    ASSERT_EQ(ani_language_for_extension(".fsx"), ANI_LANG_FSHARP);
    PASS();
}
TEST(lang_ext_julia) {
    ASSERT_EQ(ani_language_for_extension(".jl"), ANI_LANG_JULIA);
    PASS();
}
TEST(lang_ext_vim) {
    ASSERT_EQ(ani_language_for_extension(".vim"), ANI_LANG_VIMSCRIPT);
    PASS();
}
TEST(lang_ext_nix) {
    ASSERT_EQ(ani_language_for_extension(".nix"), ANI_LANG_NIX);
    PASS();
}
TEST(lang_ext_commonlisp) {
    ASSERT_EQ(ani_language_for_extension(".lisp"), ANI_LANG_COMMONLISP);
    PASS();
}
TEST(lang_ext_lsp) {
    ASSERT_EQ(ani_language_for_extension(".lsp"), ANI_LANG_COMMONLISP);
    PASS();
}
TEST(lang_ext_cl) {
    ASSERT_EQ(ani_language_for_extension(".cl"), ANI_LANG_COMMONLISP);
    PASS();
}
TEST(lang_ext_elm) {
    ASSERT_EQ(ani_language_for_extension(".elm"), ANI_LANG_ELM);
    PASS();
}
TEST(lang_ext_fortran) {
    ASSERT_EQ(ani_language_for_extension(".f90"), ANI_LANG_FORTRAN);
    PASS();
}
TEST(lang_ext_f95) {
    ASSERT_EQ(ani_language_for_extension(".f95"), ANI_LANG_FORTRAN);
    PASS();
}
TEST(lang_ext_f03) {
    ASSERT_EQ(ani_language_for_extension(".f03"), ANI_LANG_FORTRAN);
    PASS();
}
TEST(lang_ext_f08) {
    ASSERT_EQ(ani_language_for_extension(".f08"), ANI_LANG_FORTRAN);
    PASS();
}
TEST(lang_ext_cuda) {
    ASSERT_EQ(ani_language_for_extension(".cu"), ANI_LANG_CUDA);
    PASS();
}
TEST(lang_ext_cuh) {
    ASSERT_EQ(ani_language_for_extension(".cuh"), ANI_LANG_CUDA);
    PASS();
}
TEST(lang_ext_cobol) {
    ASSERT_EQ(ani_language_for_extension(".cob"), ANI_LANG_COBOL);
    PASS();
}
TEST(lang_ext_cbl) {
    ASSERT_EQ(ani_language_for_extension(".cbl"), ANI_LANG_COBOL);
    PASS();
}
TEST(lang_ext_verilog) {
    ASSERT_EQ(ani_language_for_extension(".v"), ANI_LANG_VERILOG);
    PASS();
}
TEST(lang_ext_sv) {
    ASSERT_EQ(ani_language_for_extension(".sv"), ANI_LANG_VERILOG);
    PASS();
}
TEST(lang_ext_emacslisp) {
    ASSERT_EQ(ani_language_for_extension(".el"), ANI_LANG_EMACSLISP);
    PASS();
}

/* Scientific/math */
TEST(lang_ext_matlab) {
    ASSERT_EQ(ani_language_for_extension(".matlab"), ANI_LANG_MATLAB);
    PASS();
}
TEST(lang_ext_mlx) {
    ASSERT_EQ(ani_language_for_extension(".mlx"), ANI_LANG_MATLAB);
    PASS();
}
TEST(lang_ext_lean) {
    ASSERT_EQ(ani_language_for_extension(".lean"), ANI_LANG_LEAN);
    PASS();
}
TEST(lang_ext_form) {
    ASSERT_EQ(ani_language_for_extension(".frm"), ANI_LANG_FORM);
    PASS();
}
TEST(lang_ext_prc) {
    ASSERT_EQ(ani_language_for_extension(".prc"), ANI_LANG_FORM);
    PASS();
}
TEST(lang_ext_magma) {
    ASSERT_EQ(ani_language_for_extension(".mag"), ANI_LANG_MAGMA);
    PASS();
}
TEST(lang_ext_magma2) {
    ASSERT_EQ(ani_language_for_extension(".magma"), ANI_LANG_MAGMA);
    PASS();
}
TEST(lang_ext_wolfram) {
    ASSERT_EQ(ani_language_for_extension(".wl"), ANI_LANG_WOLFRAM);
    PASS();
}
TEST(lang_ext_wls) {
    ASSERT_EQ(ani_language_for_extension(".wls"), ANI_LANG_WOLFRAM);
    PASS();
}

/* Helper languages */
TEST(lang_ext_html) {
    ASSERT_EQ(ani_language_for_extension(".html"), ANI_LANG_HTML);
    PASS();
}
TEST(lang_ext_htm) {
    ASSERT_EQ(ani_language_for_extension(".htm"), ANI_LANG_HTML);
    PASS();
}
TEST(lang_ext_css) {
    ASSERT_EQ(ani_language_for_extension(".css"), ANI_LANG_CSS);
    PASS();
}
TEST(lang_ext_scss) {
    ASSERT_EQ(ani_language_for_extension(".scss"), ANI_LANG_SCSS);
    PASS();
}
TEST(lang_ext_yaml) {
    ASSERT_EQ(ani_language_for_extension(".yml"), ANI_LANG_YAML);
    PASS();
}
TEST(lang_ext_yaml2) {
    ASSERT_EQ(ani_language_for_extension(".yaml"), ANI_LANG_YAML);
    PASS();
}
TEST(lang_ext_toml) {
    ASSERT_EQ(ani_language_for_extension(".toml"), ANI_LANG_TOML);
    PASS();
}
TEST(lang_ext_hcl) {
    ASSERT_EQ(ani_language_for_extension(".tf"), ANI_LANG_HCL);
    PASS();
}
TEST(lang_ext_hcl2) {
    ASSERT_EQ(ani_language_for_extension(".hcl"), ANI_LANG_HCL);
    PASS();
}
TEST(lang_ext_sql) {
    ASSERT_EQ(ani_language_for_extension(".sql"), ANI_LANG_SQL);
    PASS();
}
TEST(lang_ext_dockerfile) {
    ASSERT_EQ(ani_language_for_extension(".dockerfile"), ANI_LANG_DOCKERFILE);
    PASS();
}
TEST(lang_ext_json) {
    ASSERT_EQ(ani_language_for_extension(".json"), ANI_LANG_JSON);
    PASS();
}
TEST(lang_ext_xml) {
    ASSERT_EQ(ani_language_for_extension(".xml"), ANI_LANG_XML);
    PASS();
}
TEST(lang_ext_xsl) {
    ASSERT_EQ(ani_language_for_extension(".xsl"), ANI_LANG_XML);
    PASS();
}
TEST(lang_ext_xsd) {
    ASSERT_EQ(ani_language_for_extension(".xsd"), ANI_LANG_XML);
    PASS();
}
TEST(lang_ext_svg) {
    ASSERT_EQ(ani_language_for_extension(".svg"), ANI_LANG_XML);
    PASS();
}
TEST(lang_ext_markdown) {
    ASSERT_EQ(ani_language_for_extension(".md"), ANI_LANG_MARKDOWN);
    PASS();
}
TEST(lang_ext_mdx) {
    ASSERT_EQ(ani_language_for_extension(".mdx"), ANI_LANG_MARKDOWN);
    PASS();
}
TEST(lang_ext_makefile) {
    ASSERT_EQ(ani_language_for_extension(".mk"), ANI_LANG_MAKEFILE);
    PASS();
}
TEST(lang_ext_cmake) {
    ASSERT_EQ(ani_language_for_extension(".cmake"), ANI_LANG_CMAKE);
    PASS();
}
TEST(lang_ext_protobuf) {
    ASSERT_EQ(ani_language_for_extension(".proto"), ANI_LANG_PROTOBUF);
    PASS();
}
TEST(lang_ext_graphql) {
    ASSERT_EQ(ani_language_for_extension(".graphql"), ANI_LANG_GRAPHQL);
    PASS();
}
TEST(lang_ext_gql) {
    ASSERT_EQ(ani_language_for_extension(".gql"), ANI_LANG_GRAPHQL);
    PASS();
}
TEST(lang_ext_vue) {
    ASSERT_EQ(ani_language_for_extension(".vue"), ANI_LANG_VUE);
    PASS();
}
TEST(lang_ext_svelte) {
    ASSERT_EQ(ani_language_for_extension(".svelte"), ANI_LANG_SVELTE);
    PASS();
}
TEST(lang_ext_meson) {
    ASSERT_EQ(ani_language_for_extension(".meson"), ANI_LANG_MESON);
    PASS();
}
TEST(lang_ext_glsl) {
    ASSERT_EQ(ani_language_for_extension(".glsl"), ANI_LANG_GLSL);
    PASS();
}
TEST(lang_ext_vert) {
    ASSERT_EQ(ani_language_for_extension(".vert"), ANI_LANG_GLSL);
    PASS();
}
TEST(lang_ext_frag) {
    ASSERT_EQ(ani_language_for_extension(".frag"), ANI_LANG_GLSL);
    PASS();
}
TEST(lang_ext_ini) {
    ASSERT_EQ(ani_language_for_extension(".ini"), ANI_LANG_INI);
    PASS();
}
TEST(lang_ext_cfg) {
    ASSERT_EQ(ani_language_for_extension(".cfg"), ANI_LANG_INI);
    PASS();
}
TEST(lang_ext_conf) {
    ASSERT_EQ(ani_language_for_extension(".conf"), ANI_LANG_INI);
    PASS();
}

/* Unknown extension */
TEST(lang_ext_unknown) {
    ASSERT_EQ(ani_language_for_extension(".xyz"), ANI_LANG_COUNT);
    PASS();
}
TEST(lang_ext_null) {
    ASSERT_EQ(ani_language_for_extension(""), ANI_LANG_COUNT);
    PASS();
}

/* ── Filename-based detection ──────────────────────────────────── */

TEST(lang_fn_makefile) {
    ASSERT_EQ(ani_language_for_filename("Makefile"), ANI_LANG_MAKEFILE);
    PASS();
}
TEST(lang_fn_gnumakefile) {
    ASSERT_EQ(ani_language_for_filename("GNUmakefile"), ANI_LANG_MAKEFILE);
    PASS();
}
TEST(lang_fn_makefile_lower) {
    ASSERT_EQ(ani_language_for_filename("makefile"), ANI_LANG_MAKEFILE);
    PASS();
}
TEST(lang_fn_cmake) {
    ASSERT_EQ(ani_language_for_filename("CMakeLists.txt"), ANI_LANG_CMAKE);
    PASS();
}
TEST(lang_fn_dockerfile) {
    ASSERT_EQ(ani_language_for_filename("Dockerfile"), ANI_LANG_DOCKERFILE);
    PASS();
}
TEST(lang_fn_meson_build) {
    ASSERT_EQ(ani_language_for_filename("meson.build"), ANI_LANG_MESON);
    PASS();
}
TEST(lang_fn_meson_opts) {
    ASSERT_EQ(ani_language_for_filename("meson.options"), ANI_LANG_MESON);
    PASS();
}
TEST(lang_fn_meson_opts_txt) {
    ASSERT_EQ(ani_language_for_filename("meson_options.txt"), ANI_LANG_MESON);
    PASS();
}
TEST(lang_fn_vimrc) {
    ASSERT_EQ(ani_language_for_filename(".vimrc"), ANI_LANG_VIMSCRIPT);
    PASS();
}

/* issue #258: .blade.php is a built-in compound extension → Blade by default
 * (previously fell through to the single-extension lookup and was mis-typed as
 * PHP). Plain .php still maps to PHP. */
TEST(lang_fn_blade_php_compound_issue258) {
    ASSERT_EQ(ani_language_for_filename("login.blade.php"), ANI_LANG_BLADE);
    ASSERT_EQ(ani_language_for_filename("alert.blade.php"), ANI_LANG_BLADE);
    ASSERT_EQ(ani_language_for_filename("index.php"), ANI_LANG_PHP);
    PASS();
}

/* Filename with extension falls through to extension lookup */
TEST(lang_fn_main_go) {
    ASSERT_EQ(ani_language_for_filename("main.go"), ANI_LANG_GO);
    PASS();
}
TEST(lang_fn_test_py) {
    ASSERT_EQ(ani_language_for_filename("test.py"), ANI_LANG_PYTHON);
    PASS();
}
TEST(lang_fn_unknown) {
    ASSERT_EQ(ani_language_for_filename("README"), ANI_LANG_COUNT);
    PASS();
}

/* ── Language name ─────────────────────────────────────────────── */

TEST(lang_name_go) {
    ASSERT_STR_EQ(ani_language_name(ANI_LANG_GO), "Go");
    PASS();
}
TEST(lang_name_python) {
    ASSERT_STR_EQ(ani_language_name(ANI_LANG_PYTHON), "Python");
    PASS();
}
TEST(lang_name_cpp) {
    ASSERT_STR_EQ(ani_language_name(ANI_LANG_CPP), "C++");
    PASS();
}
TEST(lang_name_csharp) {
    ASSERT_STR_EQ(ani_language_name(ANI_LANG_CSHARP), "C#");
    PASS();
}
TEST(lang_name_unknown) {
    ASSERT_STR_EQ(ani_language_name(ANI_LANG_COUNT), "Unknown");
    PASS();
}

/* ── .m disambiguation ─────────────────────────────────────────── */

/* These tests need temp files with content markers */
TEST(lang_m_objc) {
    /* Write a temp file with Objective-C markers */
    char path[256];
    snprintf(path, sizeof(path), "%s/test_lang_objc.m", ani_tmpdir());
    FILE *f = fopen(path, "w");
    ASSERT_NOT_NULL(f);
    fprintf(f, "#import <Foundation/Foundation.h>\n@interface Foo : NSObject\n@end\n");
    fclose(f);

    ASSERT_EQ(ani_disambiguate_m(path), ANI_LANG_OBJC);
    remove(path);
    PASS();
}

TEST(lang_m_magma) {
    char path[256];
    snprintf(path, sizeof(path), "%s/test_lang_magma.m", ani_tmpdir());
    FILE *f = fopen(path, "w");
    ASSERT_NOT_NULL(f);
    fprintf(f, "function MyFunc(x)\n  return x^2;\nend function;\n");
    fclose(f);

    ASSERT_EQ(ani_disambiguate_m(path), ANI_LANG_MAGMA);
    remove(path);
    PASS();
}

TEST(lang_m_matlab) {
    char path[256];
    snprintf(path, sizeof(path), "%s/test_lang_matlab.m", ani_tmpdir());
    FILE *f = fopen(path, "w");
    ASSERT_NOT_NULL(f);
    fprintf(f, "function y = square(x)\n  y = x.^2;\nend\n");
    fclose(f);

    ASSERT_EQ(ani_disambiguate_m(path), ANI_LANG_MATLAB);
    remove(path);
    PASS();
}

TEST(lang_m_default_on_read_fail) {
    /* Non-existent file defaults to MATLAB */
    ASSERT_EQ(ani_disambiguate_m("/tmp/nonexistent_file_12345.m"), ANI_LANG_MATLAB);
    PASS();
}

/* ── .cfc disambiguation (tag vs script dialect) ───────────────── */

/* Helper: write content to a temp .cfc and return its disambiguated language. */
static ANILanguage disambiguate_cfc_content(const char *name, const char *content) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", ani_tmpdir(), name);
    FILE *f = fopen(path, "w");
    if (!f) {
        return ANI_LANG_COUNT;
    }
    fputs(content, f);
    fclose(f);
    ANILanguage lang = ani_disambiguate_cfc(path);
    remove(path);
    return lang;
}

TEST(lang_cfc_tag_component) {
    /* <cfcomponent> wrapper ⇒ tag dialect. */
    ASSERT_EQ(disambiguate_cfc_content("test_cfc_tag.cfc",
                                       "<cfcomponent>\n<cffunction name=\"f\"></cffunction>\n"
                                       "</cfcomponent>\n"),
              ANI_LANG_CFML);
    PASS();
}

TEST(lang_cfc_bare_cffunction) {
    /* A component that omits <cfcomponent> but uses <cffunction> is still tag. */
    ASSERT_EQ(disambiguate_cfc_content("test_cfc_bare.cfc",
                                       "<cffunction name=\"f\" output=\"false\">\n"
                                       "<cfreturn 1>\n</cffunction>\n"),
              ANI_LANG_CFML);
    PASS();
}

TEST(lang_cfc_script_component) {
    /* Plain "component { ... }" ⇒ script dialect. */
    ASSERT_EQ(disambiguate_cfc_content("test_cfc_script.cfc",
                                       "component {\n    function f() { return 1; }\n}\n"),
              ANI_LANG_CFSCRIPT);
    PASS();
}

TEST(lang_cfc_script_bare_keyword) {
    /* "component" on its own line (brace on the next) ⇒ script dialect. */
    ASSERT_EQ(disambiguate_cfc_content("test_cfc_kw.cfc",
                                       "component\n{\n    function f() { return 1; }\n}\n"),
              ANI_LANG_CFSCRIPT);
    PASS();
}

TEST(lang_cfc_cfscript_wrapped_script) {
    /* A script component wrapped in a leading <cfscript> is still script — the
     * leading '<' must NOT route it to the tag grammar. */
    ASSERT_EQ(
        disambiguate_cfc_content("test_cfc_wrapped.cfc",
                                 "<cfscript>\ncomponent {\n    function f() { return 1; }\n}\n"
                                 "</cfscript>\n"),
        ANI_LANG_CFSCRIPT);
    PASS();
}

TEST(lang_cfc_tag_after_license_comment) {
    /* A leading <!--- ---> license comment before <cfcomponent> ⇒ tag. */
    ASSERT_EQ(disambiguate_cfc_content("test_cfc_licensed.cfc",
                                       "<!---\n  Copyright\n--->\n<cfcomponent>\n</cfcomponent>\n"),
              ANI_LANG_CFML);
    PASS();
}

TEST(lang_cfc_script_after_license_comment) {
    /* A leading <!--- ---> comment before a script component ⇒ script (the
     * comment's '<' must be skipped, not treated as a tag opener). */
    ASSERT_EQ(disambiguate_cfc_content("test_cfc_lic_script.cfc",
                                       "<!--- header ---> \ncomponent {\n"
                                       "    function f() { return 1; }\n}\n"),
              ANI_LANG_CFSCRIPT);
    PASS();
}

TEST(lang_cfc_default_on_read_fail) {
    /* Non-existent file defaults to script dialect. */
    ASSERT_EQ(ani_disambiguate_cfc("/tmp/nonexistent_file_98765.cfc"), ANI_LANG_CFSCRIPT);
    PASS();
}

/* --- New languages (auto-generated) --- */
TEST(lang_ext_solidity) {
    ASSERT_EQ(ani_language_for_extension(".sol"), ANI_LANG_SOLIDITY);
    PASS();
}

TEST(lang_ext_typst) {
    ASSERT_EQ(ani_language_for_extension(".typ"), ANI_LANG_TYPST);
    PASS();
}

TEST(lang_ext_gdscript) {
    ASSERT_EQ(ani_language_for_extension(".gd"), ANI_LANG_GDSCRIPT);
    PASS();
}

TEST(lang_ext_gleam) {
    ASSERT_EQ(ani_language_for_extension(".gleam"), ANI_LANG_GLEAM);
    PASS();
}

TEST(lang_ext_powershell) {
    ASSERT_EQ(ani_language_for_extension(".ps1"), ANI_LANG_POWERSHELL);
    ASSERT_EQ(ani_language_for_extension(".psm1"), ANI_LANG_POWERSHELL);
    ASSERT_EQ(ani_language_for_extension(".psd1"), ANI_LANG_POWERSHELL);
    PASS();
}

TEST(lang_ext_pascal) {
    ASSERT_EQ(ani_language_for_extension(".pas"), ANI_LANG_PASCAL);
    ASSERT_EQ(ani_language_for_extension(".lpr"), ANI_LANG_PASCAL);
    ASSERT_EQ(ani_language_for_extension(".dpr"), ANI_LANG_PASCAL);
    PASS();
}

TEST(lang_ext_d) {
    ASSERT_EQ(ani_language_for_extension(".d"), ANI_LANG_DLANG);
    PASS();
}

TEST(lang_ext_nim) {
    /* nim grammar removed — .nim/.nims no longer map to a language */
    ASSERT_EQ(ani_language_for_extension(".nim"), ANI_LANG_COUNT);
    ASSERT_EQ(ani_language_for_extension(".nims"), ANI_LANG_COUNT);
    PASS();
}

TEST(lang_ext_scheme) {
    ASSERT_EQ(ani_language_for_extension(".scm"), ANI_LANG_SCHEME);
    ASSERT_EQ(ani_language_for_extension(".ss"), ANI_LANG_SCHEME);
    PASS();
}

TEST(lang_ext_chialisp) {
    ASSERT_EQ(ani_language_for_extension(".clsp"), ANI_LANG_CHIALISP);
    ASSERT_EQ(ani_language_for_extension(".clib"), ANI_LANG_CHIALISP);
    ASSERT_EQ(ani_language_for_extension(".clinc"), ANI_LANG_CHIALISP);
    /* .clj stays Clojure — the Chialisp extensions must not widen it. */
    ASSERT_EQ(ani_language_for_extension(".clj"), ANI_LANG_CLOJURE);
    PASS();
}

TEST(lang_ext_fennel) {
    ASSERT_EQ(ani_language_for_extension(".fnl"), ANI_LANG_FENNEL);
    PASS();
}

TEST(lang_ext_fish) {
    ASSERT_EQ(ani_language_for_extension(".fish"), ANI_LANG_FISH);
    PASS();
}

TEST(lang_ext_awk) {
    ASSERT_EQ(ani_language_for_extension(".awk"), ANI_LANG_AWK);
    PASS();
}

TEST(lang_ext_zsh) {
    ASSERT_EQ(ani_language_for_extension(".zsh"), ANI_LANG_ZSH);
    PASS();
}

TEST(lang_ext_tcl) {
    ASSERT_EQ(ani_language_for_extension(".tcl"), ANI_LANG_TCL);
    PASS();
}

TEST(lang_ext_ada) {
    ASSERT_EQ(ani_language_for_extension(".adb"), ANI_LANG_ADA);
    ASSERT_EQ(ani_language_for_extension(".ads"), ANI_LANG_ADA);
    PASS();
}

TEST(lang_ext_agda) {
    ASSERT_EQ(ani_language_for_extension(".agda"), ANI_LANG_AGDA);
    PASS();
}

TEST(lang_ext_racket) {
    ASSERT_EQ(ani_language_for_extension(".rkt"), ANI_LANG_RACKET);
    PASS();
}

TEST(lang_ext_odin) {
    ASSERT_EQ(ani_language_for_extension(".odin"), ANI_LANG_ODIN);
    PASS();
}

TEST(lang_ext_rescript) {
    ASSERT_EQ(ani_language_for_extension(".res"), ANI_LANG_RESCRIPT);
    ASSERT_EQ(ani_language_for_extension(".resi"), ANI_LANG_RESCRIPT);
    PASS();
}

TEST(lang_ext_purescript) {
    ASSERT_EQ(ani_language_for_extension(".purs"), ANI_LANG_PURESCRIPT);
    PASS();
}

TEST(lang_ext_nickel) {
    ASSERT_EQ(ani_language_for_extension(".ncl"), ANI_LANG_NICKEL);
    PASS();
}

TEST(lang_ext_crystal) {
    ASSERT_EQ(ani_language_for_extension(".cr"), ANI_LANG_CRYSTAL);
    PASS();
}

TEST(lang_ext_teal) {
    ASSERT_EQ(ani_language_for_extension(".tl"), ANI_LANG_TEAL);
    PASS();
}

TEST(lang_ext_hare) {
    ASSERT_EQ(ani_language_for_extension(".ha"), ANI_LANG_HARE);
    PASS();
}

TEST(lang_ext_pony) {
    ASSERT_EQ(ani_language_for_extension(".pony"), ANI_LANG_PONY);
    PASS();
}

TEST(lang_ext_luau) {
    ASSERT_EQ(ani_language_for_extension(".luau"), ANI_LANG_LUAU);
    PASS();
}

TEST(lang_ext_qml) {
    ASSERT_EQ(ani_language_for_extension(".qml"), ANI_LANG_QML);
    PASS();
}

TEST(lang_ext_cfml) {
    ASSERT_EQ(ani_language_for_extension(".cfc"), ANI_LANG_CFSCRIPT);
    ASSERT_EQ(ani_language_for_extension(".cfm"), ANI_LANG_CFML);
    PASS();
}

TEST(lang_ext_helm_tpl) {
    ASSERT_EQ(ani_language_for_extension(".tpl"), ANI_LANG_GOTEMPLATE);
    PASS();
}

TEST(lang_ext_janet) {
    ASSERT_EQ(ani_language_for_extension(".janet"), ANI_LANG_JANET);
    PASS();
}

TEST(lang_ext_sway) {
    ASSERT_EQ(ani_language_for_extension(".sw"), ANI_LANG_SWAY);
    PASS();
}

TEST(lang_ext_nasm) {
    ASSERT_EQ(ani_language_for_extension(".nasm"), ANI_LANG_NASM);
    PASS();
}

TEST(lang_ext_assembly) {
    ASSERT_EQ(ani_language_for_extension(".s"), ANI_LANG_ASSEMBLY);
    ASSERT_EQ(ani_language_for_extension(".S"), ANI_LANG_ASSEMBLY);
    PASS();
}

TEST(lang_ext_astro) {
    ASSERT_EQ(ani_language_for_extension(".astro"), ANI_LANG_ASTRO);
    PASS();
}

TEST(lang_ext_gotemplate) {
    ASSERT_EQ(ani_language_for_extension(".tmpl"), ANI_LANG_GOTEMPLATE);
    ASSERT_EQ(ani_language_for_extension(".gotmpl"), ANI_LANG_GOTEMPLATE);
    PASS();
}

TEST(lang_ext_templ) {
    ASSERT_EQ(ani_language_for_extension(".templ"), ANI_LANG_TEMPL);
    PASS();
}

TEST(lang_ext_liquid) {
    ASSERT_EQ(ani_language_for_extension(".liquid"), ANI_LANG_LIQUID);
    PASS();
}

TEST(lang_ext_jinja2) {
    ASSERT_EQ(ani_language_for_extension(".j2"), ANI_LANG_JINJA2);
    ASSERT_EQ(ani_language_for_extension(".jinja2"), ANI_LANG_JINJA2);
    ASSERT_EQ(ani_language_for_extension(".jinja"), ANI_LANG_JINJA2);
    PASS();
}

TEST(lang_ext_prisma) {
    ASSERT_EQ(ani_language_for_extension(".prisma"), ANI_LANG_PRISMA);
    PASS();
}

TEST(lang_ext_hyprlang) {
    ASSERT_EQ(ani_language_for_extension(".hl"), ANI_LANG_HYPRLANG);
    PASS();
}

TEST(lang_ext_diff) {
    ASSERT_EQ(ani_language_for_extension(".diff"), ANI_LANG_DIFF);
    ASSERT_EQ(ani_language_for_extension(".patch"), ANI_LANG_DIFF);
    PASS();
}

TEST(lang_ext_wgsl) {
    ASSERT_EQ(ani_language_for_extension(".wgsl"), ANI_LANG_WGSL);
    PASS();
}

TEST(lang_ext_kdl) {
    ASSERT_EQ(ani_language_for_extension(".kdl"), ANI_LANG_KDL);
    PASS();
}

TEST(lang_ext_json5) {
    ASSERT_EQ(ani_language_for_extension(".json5"), ANI_LANG_JSON5);
    PASS();
}

TEST(lang_ext_jsonnet) {
    ASSERT_EQ(ani_language_for_extension(".jsonnet"), ANI_LANG_JSONNET);
    ASSERT_EQ(ani_language_for_extension(".libsonnet"), ANI_LANG_JSONNET);
    PASS();
}

TEST(lang_ext_ron) {
    ASSERT_EQ(ani_language_for_extension(".ron"), ANI_LANG_RON);
    PASS();
}

TEST(lang_ext_thrift) {
    ASSERT_EQ(ani_language_for_extension(".thrift"), ANI_LANG_THRIFT);
    PASS();
}

TEST(lang_ext_capnp) {
    ASSERT_EQ(ani_language_for_extension(".capnp"), ANI_LANG_CAPNP);
    PASS();
}

TEST(lang_ext_properties) {
    ASSERT_EQ(ani_language_for_extension(".properties"), ANI_LANG_PROPERTIES);
    PASS();
}

TEST(lang_ext_bibtex) {
    ASSERT_EQ(ani_language_for_extension(".bib"), ANI_LANG_BIBTEX);
    PASS();
}

TEST(lang_ext_starlark) {
    ASSERT_EQ(ani_language_for_extension(".star"), ANI_LANG_STARLARK);
    ASSERT_EQ(ani_language_for_extension(".bzl"), ANI_LANG_STARLARK);
    PASS();
}

TEST(lang_ext_bicep) {
    ASSERT_EQ(ani_language_for_extension(".bicep"), ANI_LANG_BICEP);
    PASS();
}

TEST(lang_ext_csv) {
    ASSERT_EQ(ani_language_for_extension(".csv"), ANI_LANG_CSV);
    PASS();
}

TEST(lang_ext_hlsl) {
    ASSERT_EQ(ani_language_for_extension(".hlsl"), ANI_LANG_HLSL);
    ASSERT_EQ(ani_language_for_extension(".hlsli"), ANI_LANG_HLSL);
    ASSERT_EQ(ani_language_for_extension(".fx"), ANI_LANG_HLSL);
    PASS();
}

TEST(lang_ext_vhdl) {
    ASSERT_EQ(ani_language_for_extension(".vhd"), ANI_LANG_VHDL);
    ASSERT_EQ(ani_language_for_extension(".vhdl"), ANI_LANG_VHDL);
    PASS();
}

TEST(lang_ext_devicetree) {
    ASSERT_EQ(ani_language_for_extension(".dts"), ANI_LANG_DEVICETREE);
    ASSERT_EQ(ani_language_for_extension(".dtsi"), ANI_LANG_DEVICETREE);
    ASSERT_EQ(ani_language_for_extension(".overlay"), ANI_LANG_DEVICETREE);
    PASS();
}

TEST(lang_ext_linkerscript) {
    ASSERT_EQ(ani_language_for_extension(".ld"), ANI_LANG_LINKERSCRIPT);
    ASSERT_EQ(ani_language_for_extension(".lds"), ANI_LANG_LINKERSCRIPT);
    PASS();
}

TEST(lang_ext_gn) {
    ASSERT_EQ(ani_language_for_extension(".gn"), ANI_LANG_GN);
    ASSERT_EQ(ani_language_for_extension(".gni"), ANI_LANG_GN);
    PASS();
}

TEST(lang_ext_bitbake) {
    ASSERT_EQ(ani_language_for_extension(".bb"), ANI_LANG_BITBAKE);
    ASSERT_EQ(ani_language_for_extension(".bbclass"), ANI_LANG_BITBAKE);
    ASSERT_EQ(ani_language_for_extension(".bbappend"), ANI_LANG_BITBAKE);
    PASS();
}

TEST(lang_ext_smali) {
    ASSERT_EQ(ani_language_for_extension(".smali"), ANI_LANG_SMALI);
    PASS();
}

TEST(lang_ext_tablegen) {
    ASSERT_EQ(ani_language_for_extension(".td"), ANI_LANG_TABLEGEN);
    PASS();
}

TEST(lang_ext_ispc) {
    ASSERT_EQ(ani_language_for_extension(".ispc"), ANI_LANG_ISPC);
    PASS();
}

TEST(lang_ext_cairo) {
    ASSERT_EQ(ani_language_for_extension(".cairo"), ANI_LANG_CAIRO);
    PASS();
}

TEST(lang_ext_move) {
    ASSERT_EQ(ani_language_for_extension(".move"), ANI_LANG_MOVE);
    PASS();
}

TEST(lang_ext_mojo) {
    ASSERT_EQ(ani_language_for_extension(".mojo"), ANI_LANG_MOJO);
    PASS();
}

TEST(lang_ext_arkts) {
    ASSERT_EQ(ani_language_for_extension(".ets"), ANI_LANG_ARKTS);
    PASS();
}

TEST(lang_ext_plsql) {
    ASSERT_EQ(ani_language_for_extension(".pks"), ANI_LANG_PLSQL);
    ASSERT_EQ(ani_language_for_extension(".pkb"), ANI_LANG_PLSQL);
    ASSERT_EQ(ani_language_for_extension(".pck"), ANI_LANG_PLSQL);
    ASSERT_EQ(ani_language_for_extension(".pls"), ANI_LANG_PLSQL);
    ASSERT_EQ(ani_language_for_extension(".plb"), ANI_LANG_PLSQL);
    ASSERT_EQ(ani_language_for_extension(".plsql"), ANI_LANG_PLSQL);
    ASSERT_EQ(ani_language_for_extension(".fnc"), ANI_LANG_PLSQL);
    ASSERT_EQ(ani_language_for_extension(".trg"), ANI_LANG_PLSQL);
    ASSERT_EQ(ani_language_for_extension(".bdy"), ANI_LANG_PLSQL);
    ASSERT_EQ(ani_language_for_extension(".tps"), ANI_LANG_PLSQL);
    ASSERT_EQ(ani_language_for_extension(".tpb"), ANI_LANG_PLSQL);
    /* .sql stays generic SQL; .prc stays FORM */
    ASSERT_EQ(ani_language_for_extension(".sql"), ANI_LANG_SQL);
    ASSERT_EQ(ani_language_for_extension(".prc"), ANI_LANG_FORM);
    PASS();
}

TEST(lang_ext_squirrel) {
    ASSERT_EQ(ani_language_for_extension(".nut"), ANI_LANG_SQUIRREL);
    PASS();
}

TEST(lang_ext_func) {
    ASSERT_EQ(ani_language_for_extension(".fc"), ANI_LANG_FUNC);
    PASS();
}

TEST(lang_ext_rst) {
    ASSERT_EQ(ani_language_for_extension(".rst"), ANI_LANG_RST);
    PASS();
}

TEST(lang_ext_beancount) {
    ASSERT_EQ(ani_language_for_extension(".beancount"), ANI_LANG_BEANCOUNT);
    PASS();
}

TEST(lang_ext_mermaid) {
    ASSERT_EQ(ani_language_for_extension(".mmd"), ANI_LANG_MERMAID);
    ASSERT_EQ(ani_language_for_extension(".mermaid"), ANI_LANG_MERMAID);
    PASS();
}

TEST(lang_ext_puppet) {
    ASSERT_EQ(ani_language_for_extension(".pp"), ANI_LANG_PUPPET);
    PASS();
}

TEST(lang_ext_po) {
    ASSERT_EQ(ani_language_for_extension(".po"), ANI_LANG_PO);
    ASSERT_EQ(ani_language_for_extension(".pot"), ANI_LANG_PO);
    PASS();
}

TEST(lang_ext_slang) {
    ASSERT_EQ(ani_language_for_extension(".slang"), ANI_LANG_SLANG);
    PASS();
}

TEST(lang_ext_llvm) {
    ASSERT_EQ(ani_language_for_extension(".ll"), ANI_LANG_LLVM_IR);
    PASS();
}

TEST(lang_ext_smithy) {
    ASSERT_EQ(ani_language_for_extension(".smithy"), ANI_LANG_SMITHY);
    PASS();
}

TEST(lang_ext_wit) {
    ASSERT_EQ(ani_language_for_extension(".wit"), ANI_LANG_WIT);
    PASS();
}

TEST(lang_ext_tlaplus) {
    ASSERT_EQ(ani_language_for_extension(".tla"), ANI_LANG_TLAPLUS);
    PASS();
}

TEST(lang_ext_pkl) {
    ASSERT_EQ(ani_language_for_extension(".pkl"), ANI_LANG_PKL);
    PASS();
}

TEST(lang_ext_apex) {
    ASSERT_EQ(ani_language_for_extension(".cls"), ANI_LANG_APEX);
    ASSERT_EQ(ani_language_for_extension(".trigger"), ANI_LANG_APEX);
    PASS();
}

TEST(lang_ext_soql) {
    ASSERT_EQ(ani_language_for_extension(".soql"), ANI_LANG_SOQL);
    PASS();
}

TEST(lang_ext_sosl) {
    ASSERT_EQ(ani_language_for_extension(".sosl"), ANI_LANG_SOSL);
    PASS();
}

/* --- Ported from lang_test.go: TestForLanguage --- */
TEST(lang_all_have_names) {
    /* Every language enum value from 0 to ANI_LANG_COUNT-1
     * should have a non-"Unknown" name. */
    for (int i = 0; i < ANI_LANG_COUNT; i++) {
        const char *name = ani_language_name((ANILanguage)i);
        ASSERT_NOT_NULL(name);
        ASSERT_TRUE(strcmp(name, "Unknown") != 0);
    }
    PASS();
}

/* ── Suite ─────────────────────────────────────────────────────── */

SUITE(language) {
    /* Extension: Tier 1 programming */
    RUN_TEST(lang_ext_go);
    RUN_TEST(lang_ext_python);
    RUN_TEST(lang_ext_javascript);
    RUN_TEST(lang_ext_jsx);
    RUN_TEST(lang_ext_mjs_cjs);
    RUN_TEST(lang_ext_mts_cts);
    RUN_TEST(lang_ext_typescript);
    RUN_TEST(lang_ext_tsx);
    RUN_TEST(lang_ext_rust);
    RUN_TEST(lang_ext_java);
    RUN_TEST(lang_ext_cpp);
    RUN_TEST(lang_ext_hpp);
    RUN_TEST(lang_ext_cc);
    RUN_TEST(lang_ext_cxx);
    RUN_TEST(lang_ext_hxx);
    RUN_TEST(lang_ext_hh);
    RUN_TEST(lang_ext_h);
    RUN_TEST(lang_ext_ixx);
    RUN_TEST(lang_ext_csharp);
    RUN_TEST(lang_ext_razor);
    RUN_TEST(lang_ext_php);
    RUN_TEST(lang_ext_lua);
    RUN_TEST(lang_ext_scala);
    RUN_TEST(lang_ext_sc);
    RUN_TEST(lang_ext_kotlin);
    RUN_TEST(lang_ext_kts);
    RUN_TEST(lang_ext_ruby);
    RUN_TEST(lang_ext_rake);
    RUN_TEST(lang_ext_gemspec);
    RUN_TEST(lang_ext_c);
    RUN_TEST(lang_ext_bash);
    RUN_TEST(lang_ext_bash2);
    RUN_TEST(lang_ext_zig);
    RUN_TEST(lang_ext_elixir);
    RUN_TEST(lang_ext_exs);
    RUN_TEST(lang_ext_haskell);
    RUN_TEST(lang_ext_ocaml);
    RUN_TEST(lang_ext_mli);
    RUN_TEST(lang_ext_swift);
    RUN_TEST(lang_ext_dart);
    RUN_TEST(lang_ext_perl);
    RUN_TEST(lang_ext_pm);
    RUN_TEST(lang_ext_groovy);
    RUN_TEST(lang_ext_gradle);
    RUN_TEST(lang_ext_erlang);
    RUN_TEST(lang_ext_r);
    RUN_TEST(lang_ext_R);

    /* Extension: Tier 2 programming */
    RUN_TEST(lang_ext_clojure);
    RUN_TEST(lang_ext_cljs);
    RUN_TEST(lang_ext_cljc);
    RUN_TEST(lang_ext_fsharp);
    RUN_TEST(lang_ext_fsi);
    RUN_TEST(lang_ext_fsx);
    RUN_TEST(lang_ext_julia);
    RUN_TEST(lang_ext_vim);
    RUN_TEST(lang_ext_nix);
    RUN_TEST(lang_ext_commonlisp);
    RUN_TEST(lang_ext_lsp);
    RUN_TEST(lang_ext_cl);
    RUN_TEST(lang_ext_elm);
    RUN_TEST(lang_ext_fortran);
    RUN_TEST(lang_ext_f95);
    RUN_TEST(lang_ext_f03);
    RUN_TEST(lang_ext_f08);
    RUN_TEST(lang_ext_cuda);
    RUN_TEST(lang_ext_cuh);
    RUN_TEST(lang_ext_cobol);
    RUN_TEST(lang_ext_cbl);
    RUN_TEST(lang_ext_verilog);
    RUN_TEST(lang_ext_sv);
    RUN_TEST(lang_ext_emacslisp);

    /* Extension: Scientific/math */
    RUN_TEST(lang_ext_matlab);
    RUN_TEST(lang_ext_mlx);
    RUN_TEST(lang_ext_lean);
    RUN_TEST(lang_ext_form);
    RUN_TEST(lang_ext_prc);
    RUN_TEST(lang_ext_magma);
    RUN_TEST(lang_ext_magma2);
    RUN_TEST(lang_ext_wolfram);
    RUN_TEST(lang_ext_wls);

    /* Extension: Helper languages */
    RUN_TEST(lang_ext_html);
    RUN_TEST(lang_ext_htm);
    RUN_TEST(lang_ext_css);
    RUN_TEST(lang_ext_scss);
    RUN_TEST(lang_ext_yaml);
    RUN_TEST(lang_ext_yaml2);
    RUN_TEST(lang_ext_toml);
    RUN_TEST(lang_ext_hcl);
    RUN_TEST(lang_ext_hcl2);
    RUN_TEST(lang_ext_sql);
    RUN_TEST(lang_ext_dockerfile);
    RUN_TEST(lang_ext_json);
    RUN_TEST(lang_ext_xml);
    RUN_TEST(lang_ext_xsl);
    RUN_TEST(lang_ext_xsd);
    RUN_TEST(lang_ext_svg);
    RUN_TEST(lang_ext_markdown);
    RUN_TEST(lang_ext_mdx);
    RUN_TEST(lang_ext_makefile);
    RUN_TEST(lang_ext_cmake);
    RUN_TEST(lang_ext_protobuf);
    RUN_TEST(lang_ext_graphql);
    RUN_TEST(lang_ext_gql);
    RUN_TEST(lang_ext_vue);
    RUN_TEST(lang_ext_svelte);
    RUN_TEST(lang_ext_meson);
    RUN_TEST(lang_ext_glsl);
    RUN_TEST(lang_ext_vert);
    RUN_TEST(lang_ext_frag);
    RUN_TEST(lang_ext_ini);
    RUN_TEST(lang_ext_cfg);
    RUN_TEST(lang_ext_conf);

    /* Unknown/edge cases */
    RUN_TEST(lang_ext_unknown);
    RUN_TEST(lang_ext_null);

    /* Filename-based */
    RUN_TEST(lang_fn_makefile);
    RUN_TEST(lang_fn_gnumakefile);
    RUN_TEST(lang_fn_makefile_lower);
    RUN_TEST(lang_fn_cmake);
    RUN_TEST(lang_fn_dockerfile);
    RUN_TEST(lang_fn_meson_build);
    RUN_TEST(lang_fn_meson_opts);
    RUN_TEST(lang_fn_meson_opts_txt);
    RUN_TEST(lang_fn_vimrc);
    RUN_TEST(lang_fn_blade_php_compound_issue258);
    RUN_TEST(lang_fn_main_go);
    RUN_TEST(lang_fn_test_py);
    RUN_TEST(lang_fn_unknown);

    /* Language names */
    RUN_TEST(lang_name_go);
    RUN_TEST(lang_name_python);
    RUN_TEST(lang_name_cpp);
    RUN_TEST(lang_name_csharp);
    RUN_TEST(lang_name_unknown);

    /* .m disambiguation */
    RUN_TEST(lang_m_objc);
    RUN_TEST(lang_m_magma);
    RUN_TEST(lang_m_matlab);
    RUN_TEST(lang_m_default_on_read_fail);
    RUN_TEST(lang_cfc_tag_component);
    RUN_TEST(lang_cfc_bare_cffunction);
    RUN_TEST(lang_cfc_script_component);
    RUN_TEST(lang_cfc_script_bare_keyword);
    RUN_TEST(lang_cfc_cfscript_wrapped_script);
    RUN_TEST(lang_cfc_tag_after_license_comment);
    RUN_TEST(lang_cfc_script_after_license_comment);
    RUN_TEST(lang_cfc_default_on_read_fail);

    /* Go test ports */
    /* New languages */
    RUN_TEST(lang_ext_solidity);
    RUN_TEST(lang_ext_typst);
    RUN_TEST(lang_ext_gdscript);
    RUN_TEST(lang_ext_gleam);
    RUN_TEST(lang_ext_powershell);
    RUN_TEST(lang_ext_pascal);
    RUN_TEST(lang_ext_d);
    RUN_TEST(lang_ext_nim);
    RUN_TEST(lang_ext_scheme);
    RUN_TEST(lang_ext_chialisp);
    RUN_TEST(lang_ext_fennel);
    RUN_TEST(lang_ext_fish);
    RUN_TEST(lang_ext_awk);
    RUN_TEST(lang_ext_zsh);
    RUN_TEST(lang_ext_tcl);
    RUN_TEST(lang_ext_ada);
    RUN_TEST(lang_ext_agda);
    RUN_TEST(lang_ext_racket);
    RUN_TEST(lang_ext_odin);
    RUN_TEST(lang_ext_rescript);
    RUN_TEST(lang_ext_purescript);
    RUN_TEST(lang_ext_nickel);
    RUN_TEST(lang_ext_crystal);
    RUN_TEST(lang_ext_teal);
    RUN_TEST(lang_ext_hare);
    RUN_TEST(lang_ext_pony);
    RUN_TEST(lang_ext_luau);
    RUN_TEST(lang_ext_qml);
    RUN_TEST(lang_ext_cfml);
    RUN_TEST(lang_ext_helm_tpl);
    RUN_TEST(lang_ext_janet);
    RUN_TEST(lang_ext_sway);
    RUN_TEST(lang_ext_nasm);
    RUN_TEST(lang_ext_assembly);
    RUN_TEST(lang_ext_astro);
    RUN_TEST(lang_ext_gotemplate);
    RUN_TEST(lang_ext_templ);
    RUN_TEST(lang_ext_liquid);
    RUN_TEST(lang_ext_jinja2);
    RUN_TEST(lang_ext_prisma);
    RUN_TEST(lang_ext_hyprlang);
    RUN_TEST(lang_ext_diff);
    RUN_TEST(lang_ext_wgsl);
    RUN_TEST(lang_ext_kdl);
    RUN_TEST(lang_ext_json5);
    RUN_TEST(lang_ext_jsonnet);
    RUN_TEST(lang_ext_ron);
    RUN_TEST(lang_ext_thrift);
    RUN_TEST(lang_ext_capnp);
    RUN_TEST(lang_ext_properties);
    RUN_TEST(lang_ext_bibtex);
    RUN_TEST(lang_ext_starlark);
    RUN_TEST(lang_ext_bicep);
    RUN_TEST(lang_ext_csv);
    RUN_TEST(lang_ext_hlsl);
    RUN_TEST(lang_ext_vhdl);
    RUN_TEST(lang_ext_devicetree);
    RUN_TEST(lang_ext_linkerscript);
    RUN_TEST(lang_ext_gn);
    RUN_TEST(lang_ext_bitbake);
    RUN_TEST(lang_ext_smali);
    RUN_TEST(lang_ext_tablegen);
    RUN_TEST(lang_ext_ispc);
    RUN_TEST(lang_ext_cairo);
    RUN_TEST(lang_ext_move);
    RUN_TEST(lang_ext_mojo);
    RUN_TEST(lang_ext_arkts);
    RUN_TEST(lang_ext_plsql);
    RUN_TEST(lang_ext_squirrel);
    RUN_TEST(lang_ext_func);
    RUN_TEST(lang_ext_rst);
    RUN_TEST(lang_ext_beancount);
    RUN_TEST(lang_ext_mermaid);
    RUN_TEST(lang_ext_puppet);
    RUN_TEST(lang_ext_po);
    RUN_TEST(lang_ext_slang);
    RUN_TEST(lang_ext_llvm);
    RUN_TEST(lang_ext_smithy);
    RUN_TEST(lang_ext_wit);
    RUN_TEST(lang_ext_tlaplus);
    RUN_TEST(lang_ext_pkl);
    RUN_TEST(lang_ext_apex);
    RUN_TEST(lang_ext_soql);
    RUN_TEST(lang_ext_sosl);

    RUN_TEST(lang_all_have_names);
}
