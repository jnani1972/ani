/*
 * language.c — Language detection from filename and extension.
 *
 * Maps file extensions and special filenames to ANILanguage enum values.
 * Handles .m disambiguation (Objective-C vs Magma vs MATLAB).
 * Consults the process-global user config (set via ani_set_user_lang_config)
 * before the built-in lookup table.
 */
#include "discover/discover.h"
#include "discover/userconfig.h"
#include "ani.h" // ANILanguage, ANI_LANG_*

#include "foundation/constants.h"
#include "foundation/compat.h" // ani_strcasestr
#include "foundation/compat_fs.h"

enum { LANG_SCAN_PASSES = 2 };
#define SLEN(s) (sizeof(s) - 1)
#include <ctype.h>
#include <stdio.h>
#include <string.h>

/* ── Extension → Language lookup table ───────────────────────────── */

typedef struct {
    const char *ext; /* including dot, e.g. ".go" */
    ANILanguage language;
} ext_entry_t;

/* Sorted by extension for binary search (but linear scan is fine for ~120 entries) */
static const ext_entry_t EXT_TABLE[] = {
    /* Bash */
    {".bash", ANI_LANG_BASH},
    {".sh", ANI_LANG_BASH},

    /* C */
    {".c", ANI_LANG_C},

    /* C++ */
    {".cc", ANI_LANG_CPP},
    {".ccm", ANI_LANG_CPP},
    {".cpp", ANI_LANG_CPP},
    {".cppm", ANI_LANG_CPP},
    {".cxx", ANI_LANG_CPP},
    {".h", ANI_LANG_CPP},
    {".hh", ANI_LANG_CPP},
    {".hpp", ANI_LANG_CPP},
    {".hxx", ANI_LANG_CPP},
    {".ixx", ANI_LANG_CPP},

    /* C# */
    {".cs", ANI_LANG_CSHARP},
    /* Blazor components. The C# grammar recovers the @code block; the
     * surrounding markup parses as ERROR regions and is reported via
     * parse_partial, which is why this is a best-effort mapping rather
     * than a dedicated grammar. */
    {".razor", ANI_LANG_CSHARP},

    /* Clojure */
    {".clj", ANI_LANG_CLOJURE},
    {".cljc", ANI_LANG_CLOJURE},
    {".cljs", ANI_LANG_CLOJURE},

    /* CMake */
    {".cmake", ANI_LANG_CMAKE},

    /* COBOL */
    {".cbl", ANI_LANG_COBOL},
    {".cob", ANI_LANG_COBOL},

    /* Common Lisp */
    {".cl", ANI_LANG_COMMONLISP},
    {".lisp", ANI_LANG_COMMONLISP},
    {".lsp", ANI_LANG_COMMONLISP},

    /* CSS */
    {".css", ANI_LANG_CSS},

    /* CUDA */
    {".cu", ANI_LANG_CUDA},
    {".cuh", ANI_LANG_CUDA},

    /* Dart */
    {".dart", ANI_LANG_DART},

    /* Dockerfile */
    {".dockerfile", ANI_LANG_DOCKERFILE},

    /* Elixir */
    {".ex", ANI_LANG_ELIXIR},
    {".exs", ANI_LANG_ELIXIR},

    /* DotEnv */
    {".env", ANI_LANG_DOTENV},

    /* Elm */
    {".elm", ANI_LANG_ELM},

    /* ArkTS (HarmonyOS/OpenHarmony) */
    {".ets", ANI_LANG_ARKTS},

    /* Emacs Lisp */
    {".el", ANI_LANG_EMACSLISP},

    /* Erlang */
    {".erl", ANI_LANG_ERLANG},

    /* F# */
    {".fs", ANI_LANG_FSHARP},
    {".fsi", ANI_LANG_FSHARP},
    {".fsx", ANI_LANG_FSHARP},

    /* FORM */
    {".frm", ANI_LANG_FORM},
    {".prc", ANI_LANG_FORM},

    /* Fortran */
    {".f03", ANI_LANG_FORTRAN},
    {".f08", ANI_LANG_FORTRAN},
    {".f90", ANI_LANG_FORTRAN},
    {".f95", ANI_LANG_FORTRAN},

    /* GLSL */
    {".frag", ANI_LANG_GLSL},
    {".glsl", ANI_LANG_GLSL},
    {".vert", ANI_LANG_GLSL},

    /* Go */
    {".go", ANI_LANG_GO},

    /* GraphQL */
    {".gql", ANI_LANG_GRAPHQL},
    {".graphql", ANI_LANG_GRAPHQL},

    /* Groovy */
    {".gradle", ANI_LANG_GROOVY},
    {".groovy", ANI_LANG_GROOVY},

    /* Haskell */
    {".hs", ANI_LANG_HASKELL},

    /* HCL / Terraform */
    {".hcl", ANI_LANG_HCL},
    {".tf", ANI_LANG_HCL},

    /* HTML */
    {".htm", ANI_LANG_HTML},
    {".html", ANI_LANG_HTML},

    /* INI */
    {".cfg", ANI_LANG_INI},
    {".conf", ANI_LANG_INI},
    {".ini", ANI_LANG_INI},

    /* Java */
    {".java", ANI_LANG_JAVA},

    /* JavaScript */
    {".js", ANI_LANG_JAVASCRIPT},
    {".jsx", ANI_LANG_JAVASCRIPT},
    {".mjs", ANI_LANG_JAVASCRIPT}, /* ES modules (#197) */
    {".cjs", ANI_LANG_JAVASCRIPT}, /* CommonJS modules */

    /* JSON */
    {".json", ANI_LANG_JSON},

    /* Julia */
    {".jl", ANI_LANG_JULIA},

    /* Kotlin */
    {".kt", ANI_LANG_KOTLIN},
    {".kts", ANI_LANG_KOTLIN},

    /* Lean */
    {".lean", ANI_LANG_LEAN},

    /* Lua */
    {".lua", ANI_LANG_LUA},

    /* Magma */
    {".mag", ANI_LANG_MAGMA},
    {".magma", ANI_LANG_MAGMA},

    /* Makefile */
    {".mk", ANI_LANG_MAKEFILE},

    /* Markdown */
    {".md", ANI_LANG_MARKDOWN},
    {".mdx", ANI_LANG_MARKDOWN},

    /* MATLAB */
    {".m", ANI_LANG_MATLAB},
    {".matlab", ANI_LANG_MATLAB},
    {".mlx", ANI_LANG_MATLAB},

    /* Meson */
    {".meson", ANI_LANG_MESON},

    /* Mojo */
    {".mojo", ANI_LANG_MOJO},

    /* Nix */
    {".nix", ANI_LANG_NIX},

    /* OCaml */
    {".ml", ANI_LANG_OCAML},
    {".mli", ANI_LANG_OCAML},

    /* Perl */
    {".pl", ANI_LANG_PERL},
    {".pm", ANI_LANG_PERL},

    /* PHP */
    {".php", ANI_LANG_PHP},

    /* Oracle PL/SQL (do not map .sql — stays generic SQL; .prc stays FORM) */
    {".pks", ANI_LANG_PLSQL},
    {".pkb", ANI_LANG_PLSQL},
    {".pck", ANI_LANG_PLSQL},
    {".pls", ANI_LANG_PLSQL},
    {".plb", ANI_LANG_PLSQL},
    {".plsql", ANI_LANG_PLSQL},
    {".fnc", ANI_LANG_PLSQL},
    {".trg", ANI_LANG_PLSQL},
    {".bdy", ANI_LANG_PLSQL},
    {".tps", ANI_LANG_PLSQL},
    {".tpb", ANI_LANG_PLSQL},

    /* Protobuf */
    {".proto", ANI_LANG_PROTOBUF},

    /* Python */
    {".py", ANI_LANG_PYTHON},

    /* R — case insensitive handled separately */
    {".R", ANI_LANG_R},
    {".r", ANI_LANG_R},

    /* Ruby */
    {".gemspec", ANI_LANG_RUBY},
    {".rake", ANI_LANG_RUBY},
    {".rb", ANI_LANG_RUBY},

    /* Rust */
    {".rs", ANI_LANG_RUST},

    /* Scala */
    {".sc", ANI_LANG_SCALA},
    {".scala", ANI_LANG_SCALA},

    /* SCSS */
    {".scss", ANI_LANG_SCSS},

    /* SQL */
    {".sql", ANI_LANG_SQL},

    /* Svelte */
    {".svelte", ANI_LANG_SVELTE},

    /* Swift */
    {".swift", ANI_LANG_SWIFT},

    /* SystemVerilog + Verilog */
    {".sv", ANI_LANG_VERILOG},
    {".v", ANI_LANG_VERILOG},

    /* TOML */
    {".toml", ANI_LANG_TOML},

    /* TSX */
    {".tsx", ANI_LANG_TSX},

    /* TypeScript */
    {".ts", ANI_LANG_TYPESCRIPT},
    {".mts", ANI_LANG_TYPESCRIPT}, /* TS ES modules */
    {".cts", ANI_LANG_TYPESCRIPT}, /* TS CommonJS modules */

    /* VimScript */
    {".vim", ANI_LANG_VIMSCRIPT},
    {".vimrc", ANI_LANG_VIMSCRIPT},
    {"justfile", ANI_LANG_JUST},
    {"Justfile", ANI_LANG_JUST},
    {".justfile", ANI_LANG_JUST},
    {".just", ANI_LANG_JUST}, /* `import 'common.just'` target files */
    {"hyprland.conf", ANI_LANG_HYPRLANG},
    {"ssh_config", ANI_LANG_SSHCONFIG},
    {"sshd_config", ANI_LANG_SSHCONFIG},
    {"BUILD", ANI_LANG_STARLARK},
    {"BUILD.bazel", ANI_LANG_STARLARK},
    {"WORKSPACE", ANI_LANG_STARLARK},
    {"WORKSPACE.bazel", ANI_LANG_STARLARK},

    /* BitBake include fragments — `require/include foo.inc` target files.
     * NOTE: .inc is also used by ObjectScript include (macro) files; the
     * ambiguity is resolved by content in ani_disambiguate_inc(). */
    {".inc", ANI_LANG_BITBAKE},

    /* InterSystems ObjectScript routines (.mac/.int/.rtn unambiguous; .cls is
     * shared with Apex and resolved by content in ani_disambiguate_cls()). */
    {".mac", ANI_LANG_OBJECTSCRIPT_ROUTINE},
    {".int", ANI_LANG_OBJECTSCRIPT_ROUTINE},
    {".rtn", ANI_LANG_OBJECTSCRIPT_ROUTINE},

    /* Vue */
    {".vue", ANI_LANG_VUE},

    /* Wolfram */
    {".wl", ANI_LANG_WOLFRAM},
    {".wls", ANI_LANG_WOLFRAM},

    /* XML */
    {".xml", ANI_LANG_XML},
    {".xsd", ANI_LANG_XML},
    {".xsl", ANI_LANG_XML},
    {".svg", ANI_LANG_XML},

    /* YAML */
    {".yaml", ANI_LANG_YAML},
    {".yml", ANI_LANG_YAML},

    /* Ada */
    {".adb", ANI_LANG_ADA},

    /* Ada */
    {".ads", ANI_LANG_ADA},

    /* Agda */
    {".agda", ANI_LANG_AGDA},

    /* Astro */
    {".astro", ANI_LANG_ASTRO},

    /* AWK */
    {".awk", ANI_LANG_AWK},

    /* BitBake */
    {".bb", ANI_LANG_BITBAKE},

    /* BitBake */
    {".bbappend", ANI_LANG_BITBAKE},

    /* BitBake */
    {".bbclass", ANI_LANG_BITBAKE},

    /* Beancount */
    {".beancount", ANI_LANG_BEANCOUNT},

    /* BibTeX */
    {".bib", ANI_LANG_BIBTEX},

    /* Bicep */
    {".bicep", ANI_LANG_BICEP},

    /* Blade */
    /* .blade.php handled by userconfig compound extensions, not EXT_TABLE */

    /* Starlark */
    {".bzl", ANI_LANG_STARLARK},

    /* Cairo */
    {".cairo", ANI_LANG_CAIRO},

    /* Cap'n Proto */
    {".capnp", ANI_LANG_CAPNP},

    /* Apex */
    {".cls", ANI_LANG_APEX},

    /* Crystal */
    {".cr", ANI_LANG_CRYSTAL},

    /* CSV */
    {".csv", ANI_LANG_CSV},

    /* D */
    {".d", ANI_LANG_DLANG},

    /* Diff */
    {".diff", ANI_LANG_DIFF},

    /* Pascal */
    {".dpr", ANI_LANG_PASCAL},

    /* DeviceTree */
    {".dts", ANI_LANG_DEVICETREE},

    /* DeviceTree */
    {".dtsi", ANI_LANG_DEVICETREE},

    /* FunC */
    {".fc", ANI_LANG_FUNC},

    /* Fish */
    {".fish", ANI_LANG_FISH},

    /* Fennel */
    {".fnl", ANI_LANG_FENNEL},

    /* HLSL */
    {".fx", ANI_LANG_HLSL},

    /* GDScript */
    {".gd", ANI_LANG_GDSCRIPT},

    /* Gleam */
    {".gleam", ANI_LANG_GLEAM},

    /* GN */
    {".gn", ANI_LANG_GN},

    /* GN */
    {".gni", ANI_LANG_GN},

    /* Go Template */
    {".gotmpl", ANI_LANG_GOTEMPLATE},
    {".tpl", ANI_LANG_GOTEMPLATE}, /* Helm _helpers.tpl named-template definitions */

    /* Hare */
    {".ha", ANI_LANG_HARE},

    /* Hyprlang */
    {".hl", ANI_LANG_HYPRLANG},

    /* HLSL */
    {".hlsl", ANI_LANG_HLSL},

    /* HLSL */
    {".hlsli", ANI_LANG_HLSL},

    /* ISPC */
    {".ispc", ANI_LANG_ISPC},

    /* Jinja2 */
    {".j2", ANI_LANG_JINJA2},

    /* Janet */
    {".janet", ANI_LANG_JANET},

    /* Jinja2 */
    {".jinja", ANI_LANG_JINJA2},

    /* Jinja2 */
    {".jinja2", ANI_LANG_JINJA2},

    /* JSON5 */
    {".json5", ANI_LANG_JSON5},

    /* Jsonnet */
    {".jsonnet", ANI_LANG_JSONNET},

    /* KDL */
    {".kdl", ANI_LANG_KDL},

    /* Linker Script */
    {".ld", ANI_LANG_LINKERSCRIPT},

    /* Linker Script */
    {".lds", ANI_LANG_LINKERSCRIPT},

    /* Jsonnet */
    {".libsonnet", ANI_LANG_JSONNET},

    /* Liquid */
    {".liquid", ANI_LANG_LIQUID},

    /* LLVM IR */
    {".ll", ANI_LANG_LLVM_IR},

    /* Pascal */
    {".lpr", ANI_LANG_PASCAL},

    /* Luau */
    {".luau", ANI_LANG_LUAU},

    /* Qt QML */
    {".qml", ANI_LANG_QML},

    /* CFML / ColdFusion — .cfm are tag templates; .cfc components may be EITHER
     * script-dialect (component { ... }) or tag-dialect (<cfcomponent> ...). The
     * table default is script; tag-based .cfc are resolved by content in
     * ani_disambiguate_cfc(). */
    {".cfc", ANI_LANG_CFSCRIPT},
    {".cfm", ANI_LANG_CFML},

    /* Mermaid */
    {".mermaid", ANI_LANG_MERMAID},

    /* Mermaid */
    {".mmd", ANI_LANG_MERMAID},

    /* Move */
    {".move", ANI_LANG_MOVE},

    /* NASM */
    {".nasm", ANI_LANG_NASM},

    /* Nickel */
    {".ncl", ANI_LANG_NICKEL},

    /* Nim */

    /* Nim */

    /* Squirrel */
    {".nut", ANI_LANG_SQUIRREL},

    /* Odin */
    {".odin", ANI_LANG_ODIN},

    /* DeviceTree */
    {".overlay", ANI_LANG_DEVICETREE},

    /* Pascal */
    {".pas", ANI_LANG_PASCAL},

    /* Diff */
    {".patch", ANI_LANG_DIFF},

    /* Pine Script */
    {".pine", ANI_LANG_PINE},

    /* Pkl */
    {".pkl", ANI_LANG_PKL},

    /* PO */
    {".po", ANI_LANG_PO},

    /* Pony */
    {".pony", ANI_LANG_PONY},

    /* PO */
    {".pot", ANI_LANG_PO},

    /* Puppet */
    {".pp", ANI_LANG_PUPPET},

    /* Prisma */
    {".prisma", ANI_LANG_PRISMA},

    /* Properties */
    {".properties", ANI_LANG_PROPERTIES},

    /* PowerShell */
    {".ps1", ANI_LANG_POWERSHELL},

    /* PowerShell */
    {".psd1", ANI_LANG_POWERSHELL},

    /* PowerShell */
    {".psm1", ANI_LANG_POWERSHELL},

    /* PureScript */
    {".purs", ANI_LANG_PURESCRIPT},

    /* ReScript */
    {".res", ANI_LANG_RESCRIPT},

    /* ReScript */
    {".resi", ANI_LANG_RESCRIPT},

    /* Regex */
    {".re", ANI_LANG_REGEX},

    /* Racket */
    {".rkt", ANI_LANG_RACKET},

    /* RON */
    {".ron", ANI_LANG_RON},

    /* reStructuredText */
    {".rst", ANI_LANG_RST},

    /* Assembly */
    {".s", ANI_LANG_ASSEMBLY},

    /* Assembly */
    {".S", ANI_LANG_ASSEMBLY},

    /* Scheme */
    {".scm", ANI_LANG_SCHEME},

    /* Chialisp — .clsp puzzles, .clib/.clinc includable libraries */
    {".clsp", ANI_LANG_CHIALISP},
    {".clib", ANI_LANG_CHIALISP},
    {".clinc", ANI_LANG_CHIALISP},

    /* Slang */
    {".slang", ANI_LANG_SLANG},

    /* Smali */
    {".smali", ANI_LANG_SMALI},

    /* Smithy */
    {".smithy", ANI_LANG_SMITHY},

    /* Solidity */
    {".sol", ANI_LANG_SOLIDITY},

    /* SOQL */
    {".soql", ANI_LANG_SOQL},

    /* SOSL */
    {".sosl", ANI_LANG_SOSL},

    /* Scheme */
    {".ss", ANI_LANG_SCHEME},

    /* Starlark */
    {".star", ANI_LANG_STARLARK},

    /* SystemVerilog */

    /* SystemVerilog */

    /* Sway */
    {".sw", ANI_LANG_SWAY},

    /* Tcl */
    {".tcl", ANI_LANG_TCL},

    /* TableGen */
    {".td", ANI_LANG_TABLEGEN},

    /* Templ */
    {".templ", ANI_LANG_TEMPL},

    /* Thrift */
    {".thrift", ANI_LANG_THRIFT},

    /* Teal */
    {".tl", ANI_LANG_TEAL},

    /* TLA+ */
    {".tla", ANI_LANG_TLAPLUS},

    /* Go Template */
    {".tmpl", ANI_LANG_GOTEMPLATE},

    /* Apex */
    {".trigger", ANI_LANG_APEX},

    /* Typst */
    {".typ", ANI_LANG_TYPST},

    /* VHDL */
    {".vhd", ANI_LANG_VHDL},

    /* VHDL */
    {".vhdl", ANI_LANG_VHDL},

    /* WGSL */
    {".wgsl", ANI_LANG_WGSL},

    /* WIT */
    {".wit", ANI_LANG_WIT},

    /* Zsh */
    {".zsh", ANI_LANG_ZSH},

    /* Zig */
    {".zig", ANI_LANG_ZIG},
};

#define EXT_TABLE_SIZE (sizeof(EXT_TABLE) / sizeof(EXT_TABLE[0]))

/* ── Special filename → Language lookup ──────────────────────────── */

typedef struct {
    const char *filename;
    ANILanguage language;
} filename_entry_t;

static const filename_entry_t FILENAME_TABLE[] = {
    {"CMakeLists.txt", ANI_LANG_CMAKE},
    {"Dockerfile", ANI_LANG_DOCKERFILE},
    {"GNUmakefile", ANI_LANG_MAKEFILE},
    {"Makefile", ANI_LANG_MAKEFILE},
    {"makefile", ANI_LANG_MAKEFILE},
    {"meson.build", ANI_LANG_MESON},
    {"meson.options", ANI_LANG_MESON},
    {"meson_options.txt", ANI_LANG_MESON},
    {"kustomization.yaml", ANI_LANG_KUSTOMIZE},
    {"kustomization.yml", ANI_LANG_KUSTOMIZE},
    /* Note: FILENAME_TABLE uses case-sensitive strcmp, so mixed-case variants
     * (e.g. "Kustomization.yaml") are not matched here.  They fall through to
     * ANI_LANG_YAML and are re-classified by ani_is_kustomize_file() in
     * pass_k8s.c, which performs a case-insensitive comparison.  This is the
     * intended behaviour — no additional entries are needed. */
    {".vimrc", ANI_LANG_VIMSCRIPT},
    {".zshrc", ANI_LANG_ZSH},
    {".zshenv", ANI_LANG_ZSH},
    {".zprofile", ANI_LANG_ZSH},
    {"justfile", ANI_LANG_JUST},
    {"Justfile", ANI_LANG_JUST},
    {".justfile", ANI_LANG_JUST},
    {"hyprland.conf", ANI_LANG_HYPRLANG},
    {"ssh_config", ANI_LANG_SSHCONFIG},
    {"sshd_config", ANI_LANG_SSHCONFIG},
    {".ssh/config", ANI_LANG_SSHCONFIG},
    {"BUILD", ANI_LANG_STARLARK},
    {"BUILD.bazel", ANI_LANG_STARLARK},
    {"WORKSPACE", ANI_LANG_STARLARK},
    {"WORKSPACE.bazel", ANI_LANG_STARLARK},
    {"requirements.txt", ANI_LANG_REQUIREMENTS},
    {"requirements-dev.txt", ANI_LANG_REQUIREMENTS},
    {"requirements-test.txt", ANI_LANG_REQUIREMENTS},
    {"Kconfig", ANI_LANG_KCONFIG},
    {"go.mod", ANI_LANG_GOMOD},
    {".env", ANI_LANG_DOTENV},
    {".env.local", ANI_LANG_DOTENV},
    {".gitattributes", ANI_LANG_GITATTRIBUTES},

};

#define FILENAME_TABLE_SIZE (sizeof(FILENAME_TABLE) / sizeof(FILENAME_TABLE[0]))

/* ── Language names ──────────────────────────────────────────────── */

static const char *LANG_NAMES[ANI_LANG_COUNT] = {
    [ANI_LANG_GO] = "Go",
    [ANI_LANG_PYTHON] = "Python",
    [ANI_LANG_JAVASCRIPT] = "JavaScript",
    [ANI_LANG_TYPESCRIPT] = "TypeScript",
    [ANI_LANG_TSX] = "TSX",
    [ANI_LANG_RUST] = "Rust",
    [ANI_LANG_JAVA] = "Java",
    [ANI_LANG_CPP] = "C++",
    [ANI_LANG_CSHARP] = "C#",
    [ANI_LANG_PHP] = "PHP",
    [ANI_LANG_LUA] = "Lua",
    [ANI_LANG_SCALA] = "Scala",
    [ANI_LANG_KOTLIN] = "Kotlin",
    [ANI_LANG_RUBY] = "Ruby",
    [ANI_LANG_C] = "C",
    [ANI_LANG_BASH] = "Bash",
    [ANI_LANG_ZIG] = "Zig",
    [ANI_LANG_ELIXIR] = "Elixir",
    [ANI_LANG_HASKELL] = "Haskell",
    [ANI_LANG_OCAML] = "OCaml",
    [ANI_LANG_OBJC] = "Objective-C",
    [ANI_LANG_SWIFT] = "Swift",
    [ANI_LANG_DART] = "Dart",
    [ANI_LANG_PERL] = "Perl",
    [ANI_LANG_GROOVY] = "Groovy",
    [ANI_LANG_ERLANG] = "Erlang",
    [ANI_LANG_R] = "R",
    [ANI_LANG_HTML] = "HTML",
    [ANI_LANG_CSS] = "CSS",
    [ANI_LANG_SCSS] = "SCSS",
    [ANI_LANG_YAML] = "YAML",
    [ANI_LANG_TOML] = "TOML",
    [ANI_LANG_HCL] = "HCL",
    [ANI_LANG_SQL] = "SQL",
    [ANI_LANG_DOCKERFILE] = "Dockerfile",
    [ANI_LANG_CLOJURE] = "Clojure",
    [ANI_LANG_FSHARP] = "F#",
    [ANI_LANG_JULIA] = "Julia",
    [ANI_LANG_VIMSCRIPT] = "VimScript",
    [ANI_LANG_NIX] = "Nix",
    [ANI_LANG_COMMONLISP] = "Common Lisp",
    [ANI_LANG_ELM] = "Elm",
    [ANI_LANG_FORTRAN] = "Fortran",
    [ANI_LANG_CUDA] = "CUDA",
    [ANI_LANG_COBOL] = "COBOL",
    [ANI_LANG_VERILOG] = "Verilog",
    [ANI_LANG_EMACSLISP] = "Emacs Lisp",
    [ANI_LANG_JSON] = "JSON",
    [ANI_LANG_XML] = "XML",
    [ANI_LANG_MARKDOWN] = "Markdown",
    [ANI_LANG_MAKEFILE] = "Makefile",
    [ANI_LANG_CMAKE] = "CMake",
    [ANI_LANG_PROTOBUF] = "Protobuf",
    [ANI_LANG_GRAPHQL] = "GraphQL",
    [ANI_LANG_VUE] = "Vue",
    [ANI_LANG_SVELTE] = "Svelte",
    [ANI_LANG_MESON] = "Meson",
    [ANI_LANG_GLSL] = "GLSL",
    [ANI_LANG_INI] = "INI",
    [ANI_LANG_MATLAB] = "MATLAB",
    [ANI_LANG_LEAN] = "Lean",
    [ANI_LANG_FORM] = "FORM",
    [ANI_LANG_MAGMA] = "Magma",
    [ANI_LANG_WOLFRAM] = "Wolfram",
    [ANI_LANG_KUSTOMIZE] = "Kustomize",
    [ANI_LANG_K8S] = "Kubernetes",
    [ANI_LANG_PINE] = "PineScript",
    [ANI_LANG_SOLIDITY] = "Solidity",
    [ANI_LANG_TYPST] = "Typst",
    [ANI_LANG_GDSCRIPT] = "GDScript",
    [ANI_LANG_GLEAM] = "Gleam",
    [ANI_LANG_POWERSHELL] = "PowerShell",
    [ANI_LANG_PASCAL] = "Pascal",
    [ANI_LANG_DLANG] = "D",
    [ANI_LANG_NIM] = "Nim",
    [ANI_LANG_SCHEME] = "Scheme",
    [ANI_LANG_CHIALISP] = "Chialisp",
    [ANI_LANG_FENNEL] = "Fennel",
    [ANI_LANG_FISH] = "Fish",
    [ANI_LANG_AWK] = "AWK",
    [ANI_LANG_ZSH] = "Zsh",
    [ANI_LANG_TCL] = "Tcl",
    [ANI_LANG_ADA] = "Ada",
    [ANI_LANG_AGDA] = "Agda",
    [ANI_LANG_RACKET] = "Racket",
    [ANI_LANG_ODIN] = "Odin",
    [ANI_LANG_RESCRIPT] = "ReScript",
    [ANI_LANG_PURESCRIPT] = "PureScript",
    [ANI_LANG_NICKEL] = "Nickel",
    [ANI_LANG_CRYSTAL] = "Crystal",
    [ANI_LANG_TEAL] = "Teal",
    [ANI_LANG_HARE] = "Hare",
    [ANI_LANG_PONY] = "Pony",
    [ANI_LANG_LUAU] = "Luau",
    [ANI_LANG_QML] = "QML",
    [ANI_LANG_CFSCRIPT] = "CFML",
    [ANI_LANG_CFML] = "CFML",
    [ANI_LANG_JANET] = "Janet",
    [ANI_LANG_SWAY] = "Sway",
    [ANI_LANG_NASM] = "NASM",
    [ANI_LANG_ASSEMBLY] = "Assembly",
    [ANI_LANG_ASTRO] = "Astro",
    [ANI_LANG_BLADE] = "Blade",
    [ANI_LANG_JUST] = "Just",
    [ANI_LANG_GOTEMPLATE] = "Go Template",
    [ANI_LANG_TEMPL] = "Templ",
    [ANI_LANG_LIQUID] = "Liquid",
    [ANI_LANG_JINJA2] = "Jinja2",
    [ANI_LANG_PRISMA] = "Prisma",
    [ANI_LANG_HYPRLANG] = "Hyprlang",
    [ANI_LANG_DOTENV] = "DotEnv",
    [ANI_LANG_SYSTEMVERILOG] = "SystemVerilog",
    [ANI_LANG_DIFF] = "Diff",
    [ANI_LANG_WGSL] = "WGSL",
    [ANI_LANG_KDL] = "KDL",
    [ANI_LANG_JSON5] = "JSON5",
    [ANI_LANG_JSONNET] = "Jsonnet",
    [ANI_LANG_RON] = "RON",
    [ANI_LANG_THRIFT] = "Thrift",
    [ANI_LANG_CAPNP] = "Cap'n Proto",
    [ANI_LANG_PROPERTIES] = "Properties",
    [ANI_LANG_SSHCONFIG] = "SSH Config",
    [ANI_LANG_BIBTEX] = "BibTeX",
    [ANI_LANG_STARLARK] = "Starlark",
    [ANI_LANG_BICEP] = "Bicep",
    [ANI_LANG_CSV] = "CSV",
    [ANI_LANG_REQUIREMENTS] = "Requirements",
    [ANI_LANG_HLSL] = "HLSL",
    [ANI_LANG_VHDL] = "VHDL",
    [ANI_LANG_DEVICETREE] = "DeviceTree",
    [ANI_LANG_LINKERSCRIPT] = "Linker Script",
    [ANI_LANG_GN] = "GN",
    [ANI_LANG_KCONFIG] = "Kconfig",
    [ANI_LANG_BITBAKE] = "BitBake",
    [ANI_LANG_SMALI] = "Smali",
    [ANI_LANG_TABLEGEN] = "TableGen",
    [ANI_LANG_ISPC] = "ISPC",
    [ANI_LANG_CAIRO] = "Cairo",
    [ANI_LANG_MOVE] = "Move",
    [ANI_LANG_SQUIRREL] = "Squirrel",
    [ANI_LANG_FUNC] = "FunC",
    [ANI_LANG_REGEX] = "Regex",
    [ANI_LANG_JSDOC] = "JSDoc",
    [ANI_LANG_RST] = "reStructuredText",
    [ANI_LANG_BEANCOUNT] = "Beancount",
    [ANI_LANG_MERMAID] = "Mermaid",
    [ANI_LANG_PUPPET] = "Puppet",
    [ANI_LANG_PO] = "PO",
    [ANI_LANG_GITATTRIBUTES] = "gitattributes",
    [ANI_LANG_GITIGNORE] = "gitignore",
    [ANI_LANG_SLANG] = "Slang",
    [ANI_LANG_LLVM_IR] = "LLVM IR",
    [ANI_LANG_SMITHY] = "Smithy",
    [ANI_LANG_WIT] = "WIT",
    [ANI_LANG_TLAPLUS] = "TLA+",
    [ANI_LANG_PKL] = "Pkl",
    [ANI_LANG_GOMOD] = "Go Mod",
    [ANI_LANG_APEX] = "Apex",
    [ANI_LANG_SOQL] = "SOQL",
    [ANI_LANG_SOSL] = "SOSL",
    [ANI_LANG_MOJO] = "Mojo",
    [ANI_LANG_OBJECTSCRIPT_UDL] = "ObjectScript UDL",
    [ANI_LANG_OBJECTSCRIPT_ROUTINE] = "ObjectScript Routine",
    [ANI_LANG_OBJECTSCRIPT_EXPORT] = "ObjectScript Export XML",
    [ANI_LANG_ARKTS] = "ArkTS",
    [ANI_LANG_PLSQL] = "PL/SQL",

};

/* ── Public API ──────────────────────────────────────────────────── */

ANILanguage ani_language_for_extension(const char *ext) {
    if (!ext || !ext[0]) {
        return ANI_LANG_COUNT;
    }

    /* Check user-defined overrides first */
    const ani_userconfig_t *ucfg = ani_get_user_lang_config();
    if (ucfg) {
        ANILanguage ulang = ani_userconfig_lookup(ucfg, ext);
        if (ulang != ANI_LANG_COUNT) {
            return ulang;
        }
    }

    for (size_t i = 0; i < EXT_TABLE_SIZE; i++) {
        if (strcmp(EXT_TABLE[i].ext, ext) == 0) {
            return EXT_TABLE[i].language;
        }
    }
    return ANI_LANG_COUNT;
}

ANILanguage ani_language_for_filename(const char *filename) {
    if (!filename || !filename[0]) {
        return ANI_LANG_COUNT;
    }

    /* Check special filenames first */
    for (size_t i = 0; i < FILENAME_TABLE_SIZE; i++) {
        if (strcmp(FILENAME_TABLE[i].filename, filename) == 0) {
            return FILENAME_TABLE[i].language;
        }
    }

    /* DotEnv variant filenames (".env.local", ".env.production", …): the
     * filename starts with ".env." but its last "extension" (e.g. ".local")
     * is not a real language extension.  Match the dotenv convention used by
     * pass_envscan/pass_infrascan (".env" exact, ".env." prefix, "*.env"
     * suffix) so file-index routing agrees with direct extraction. */
    if (strncmp(filename, ".env.", SLEN(".env.")) == 0) {
        return ANI_LANG_DOTENV;
    }

    /* Fall back to extension-based lookup.
     * For compound extensions (e.g. ".blade.php") defined in the user config,
     * scan from the first dot in the basename toward the last, checking user
     * config at each position.  Built-in extensions use the last dot only. */
    const char *last_dot = strrchr(filename, '.');
    if (!last_dot) {
        return ANI_LANG_COUNT;
    }

    /* Probe compound extensions (e.g. ".blade.php") from the first dot toward
     * the last. Built-in compounds are checked first so e.g. Laravel Blade
     * templates map to Blade rather than the single-extension fallback (PHP);
     * user config can still add more (#258). */
    static const struct {
        const char *ext;
        ANILanguage lang;
    } COMPOUND_EXT_TABLE[] = {
        {".blade.php", ANI_LANG_BLADE},
    };
    const ani_userconfig_t *ucfg = ani_get_user_lang_config();
    const char *p = strchr(filename, '.');
    while (p && p < last_dot) {
        for (size_t i = 0; i < sizeof(COMPOUND_EXT_TABLE) / sizeof(COMPOUND_EXT_TABLE[0]); i++) {
            if (strcmp(p, COMPOUND_EXT_TABLE[i].ext) == 0) {
                return COMPOUND_EXT_TABLE[i].lang;
            }
        }
        if (ucfg) {
            ANILanguage lang = ani_userconfig_lookup(ucfg, p);
            if (lang != ANI_LANG_COUNT) {
                return lang;
            }
        }
        p = strchr(p + SKIP_ONE, '.');
    }

    /* Standard single-extension lookup (built-ins + user overrides). */
    return ani_language_for_extension(last_dot);
}

const char *ani_language_name(ANILanguage lang) {
    if (lang < 0 || lang >= ANI_LANG_COUNT) {
        return "Unknown";
    }
    return LANG_NAMES[lang] ? LANG_NAMES[lang] : "Unknown";
}

/* ── Shebang interpreter detection (extensionless scripts) ────────── */

/* Basename of an interpreter path: the segment after the last '/'.  Shebangs
 * are a POSIX convention, so only '/' is treated as a separator. */
static const char *interp_basename(const char *path) {
    const char *slash = strrchr(path, '/');
    return slash ? slash + SKIP_ONE : path;
}

/* "python" optionally followed by an explicit numeric version (digits and dots
 * only, e.g. "python3", "python3.12").  Bounded and explicit so arbitrary
 * suffixes like "python-wrapper" are rejected. */
static bool is_python_interp(const char *base) {
    if (strncmp(base, "python", SLEN("python")) != 0) {
        return false;
    }
    const char *version = base + SLEN("python");
    if (*version == '\0') {
        return true;
    }

    /* Each numeric component must contain at least one digit. */
    bool need_digit = true;
    for (const char *v = version; *v; v++) {
        if (isdigit((unsigned char)*v)) {
            need_digit = false;
        } else if (*v == '.' && !need_digit) {
            need_digit = true;
        } else {
            return false;
        }
    }
    return !need_digit;
}

/* Map an interpreter basename to a language, or ANI_LANG_COUNT if unrecognized.
 * Non-python interpreters are matched exactly (no prefix/suffix logic). */
static ANILanguage lang_for_interpreter(const char *base) {
    if (is_python_interp(base)) {
        return ANI_LANG_PYTHON;
    }
    static const struct {
        const char *name;
        ANILanguage lang;
    } INTERP_TABLE[] = {
        {"sh", ANI_LANG_BASH},           {"bash", ANI_LANG_BASH}, {"dash", ANI_LANG_BASH},
        {"ksh", ANI_LANG_BASH},          {"zsh", ANI_LANG_BASH},  {"node", ANI_LANG_JAVASCRIPT},
        {"nodejs", ANI_LANG_JAVASCRIPT}, {"ruby", ANI_LANG_RUBY}, {"perl", ANI_LANG_PERL},
        {"php", ANI_LANG_PHP},           {"lua", ANI_LANG_LUA},
    };
    for (size_t i = 0; i < sizeof(INTERP_TABLE) / sizeof(INTERP_TABLE[0]); i++) {
        if (strcmp(base, INTERP_TABLE[i].name) == 0) {
            return INTERP_TABLE[i].lang;
        }
    }
    return ANI_LANG_COUNT;
}

/* Advance *cursor past leading blanks and return the next whitespace-delimited
 * token (NUL-terminated in place), or NULL when the line is exhausted. */
static char *shebang_next_token(char **cursor) {
    char *p = *cursor;
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    if (*p == '\0') {
        *cursor = p;
        return NULL;
    }
    char *start = p;
    while (*p && *p != ' ' && *p != '\t') {
        p++;
    }
    if (*p) {
        *p = '\0';
        p++;
    }
    *cursor = p;
    return start;
}

ANILanguage ani_language_from_shebang(const char *path) {
    if (!path) {
        return ANI_LANG_COUNT;
    }

    FILE *f = ani_fopen(path, "rb");
    if (!f) {
        return ANI_LANG_COUNT; /* fail closed on read error */
    }

    /* Read only a bounded first line. */
    char buf[ANI_SZ_256];
    size_t n = fread(buf, SKIP_ONE, sizeof(buf) - SKIP_ONE, f);

    /* Fail closed on any read error rather than parsing a partial buffer. */
    if (ferror(f)) {
        (void)fclose(f);
        return ANI_LANG_COUNT;
    }

    /* If the bounded buffer filled without containing a newline, the first
     * line may extend past our bound. Probe a single extra byte to tell an
     * exact EOF (the whole file is <= 255 bytes) from a truncated longer
     * line: any surviving byte -- including a newline just beyond the bound --
     * means the first line was cut off, so fail closed. A probe read error
     * fails closed too. This keeps the read bounded (no unbounded line read
     * or allocation). */
    bool have_newline = (memchr(buf, '\n', n) != NULL);
    if (!have_newline && n == sizeof(buf) - SKIP_ONE) {
        int probe = fgetc(f);
        if (probe != EOF || ferror(f)) {
            (void)fclose(f);
            return ANI_LANG_COUNT;
        }
    }
    (void)fclose(f);

    /* Must begin with "#!". */
    if (n < PAIR_LEN || buf[0] != '#' || buf[1] != '!') {
        return ANI_LANG_COUNT;
    }

    /* Isolate the first line; reject an embedded NUL before the newline. */
    size_t line_len = 0;
    while (line_len < n && buf[line_len] != '\n') {
        if (buf[line_len] == '\0') {
            return ANI_LANG_COUNT; /* embedded NUL — treat as binary */
        }
        line_len++;
    }
    /* Trim a trailing CR so CRLF first lines parse. */
    if (line_len > 0 && buf[line_len - SKIP_ONE] == '\r') {
        line_len--;
    }
    buf[line_len] = '\0';

    /* First token after "#!" is the interpreter (or env). */
    char *cursor = buf + PAIR_LEN;
    char *interp = shebang_next_token(&cursor);
    if (!interp) {
        return ANI_LANG_COUNT;
    }
    const char *base = interp_basename(interp);

    /* "env [-S] <interp> [args...]": the real interpreter is the next token.
     * Only the plain "env <interp>" and "env -S/--split-string <interp> [args]"
     * shapes are supported. After the optional -S, the interpreter token must
     * be a real command, so reject option tokens (leading '-') and NAME=value
     * assignments (containing '=') -- e.g. "env PYTHON=/usr/bin/python
     * python-wrapper", where env would treat the first token as an env-var
     * setting rather than the program to run. */
    if (strcmp(base, "env") == 0) {
        char *tok = shebang_next_token(&cursor);
        if (tok && (strcmp(tok, "-S") == 0 || strcmp(tok, "--split-string") == 0)) {
            tok = shebang_next_token(&cursor);
        }
        if (!tok || tok[0] == '-' || strchr(tok, '=') != NULL) {
            return ANI_LANG_COUNT;
        }
        base = interp_basename(tok);
    }

    return lang_for_interpreter(base);
}

/* ── .m file disambiguation ──────────────────────────────────────── */

/* Simple substring search helper */
static bool str_contains(const char *haystack, const char *needle) {
    return strstr(haystack, needle) != NULL;
}

static bool has_objc_markers(const char *buf) {
    return str_contains(buf, "@interface") || str_contains(buf, "@implementation") ||
           str_contains(buf, "@protocol") || str_contains(buf, "@property") ||
           str_contains(buf, "#import") || str_contains(buf, "@selector") ||
           str_contains(buf, "@encode") || str_contains(buf, "@synthesize") ||
           str_contains(buf, "@dynamic");
}

static bool has_magma_end_markers(const char *buf) {
    return str_contains(buf, "end function;") || str_contains(buf, "end procedure;") ||
           str_contains(buf, "end intrinsic;") || str_contains(buf, "end if;") ||
           str_contains(buf, "end for;") || str_contains(buf, "end while;");
}

/* Check for "intrinsic Name(" or "procedure Name(" patterns. */
static bool has_magma_callable_pattern(const char *buf) {
    const char *markers[] = {"intrinsic ", "procedure "};
    for (int i = 0; i < LANG_SCAN_PASSES; i++) {
        const char *p = strstr(buf, markers[i]);
        if (!p) {
            continue;
        }
        p += strlen(markers[i]);
        while (*p && isalpha((unsigned char)*p)) {
            p++;
        }
        if (*p == '(') {
            return true;
        }
    }
    return false;
}

/* Scan lines for MATLAB-specific markers (function/classdef/%%). */
static bool has_matlab_line_markers(const char *buf) {
    const char *line = buf;
    while (*line) {
        const char *p = line;
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (strncmp(p, "function ", SLEN("function ")) == 0 ||
            strncmp(p, "function\t", SLEN("function\t")) == 0 ||
            strncmp(p, "classdef ", SLEN("classdef ")) == 0 ||
            strncmp(p, "classdef\t", SLEN("classdef\t")) == 0 || strncmp(p, "%%", PAIR_LEN) == 0 ||
            (*p == '%' && *(p + SKIP_ONE) != '{')) {
            return true;
        }
        const char *nl = strchr(line, '\n');
        if (!nl) {
            break;
        }
        line = nl + SKIP_ONE;
    }
    return false;
}

ANILanguage ani_disambiguate_m(const char *path) {
    if (!path) {
        return ANI_LANG_MATLAB;
    }

    FILE *f = ani_fopen(path, "r");
    if (!f) {
        return ANI_LANG_MATLAB;
    }

    /* Read first 4KB */
    char buf[ANI_SZ_4K + SKIP_ONE];
    size_t n = fread(buf, SKIP_ONE, ANI_SZ_4K, f);
    buf[n] = '\0';
    (void)fclose(f);

    if (has_objc_markers(buf)) {
        return ANI_LANG_OBJC;
    }
    if (has_magma_end_markers(buf)) {
        return ANI_LANG_MAGMA;
    }
    if ((str_contains(buf, "intrinsic ") || str_contains(buf, "procedure ")) &&
        has_magma_callable_pattern(buf)) {
        return ANI_LANG_MAGMA;
    }
    if (has_matlab_line_markers(buf)) {
        return ANI_LANG_MATLAB;
    }

    return ANI_LANG_MATLAB;
}

/* Disambiguate .cls files: shared by InterSystems ObjectScript UDL and
 * Salesforce Apex. ObjectScript class files begin with a line of the form
 * "Class <UppercasePackage>...". Defaults to Apex on any doubt. */
ANILanguage ani_disambiguate_cls(const char *path) {
    if (!path) {
        return ANI_LANG_APEX;
    }

    FILE *f = ani_fopen(path, "r");
    if (!f) {
        return ANI_LANG_APEX;
    }

    char buf[ANI_SZ_4K + SKIP_ONE];
    size_t n = fread(buf, SKIP_ONE, ANI_SZ_4K, f);
    buf[n] = '\0';
    (void)fclose(f);

    const char *line = buf;
    while (*line) {
        if (strncmp(line, "Class ", SLEN("Class ")) == 0 &&
            isupper((unsigned char)line[SLEN("Class ")])) {
            return ANI_LANG_OBJECTSCRIPT_UDL;
        }
        const char *nl = strchr(line, '\n');
        if (!nl) {
            break;
        }
        line = nl + SKIP_ONE;
    }
    return ANI_LANG_APEX;
}

/* Disambiguate .inc files: shared by BitBake include fragments and
 * InterSystems ObjectScript include (macro) files. ObjectScript .inc files are
 * predominantly macro definitions ("#define NAME ..." / "#def1arg NAME ...");
 * some also carry a "ROUTINE <Name>" header. The macro-preprocessor directives
 * are the strongest signal because that is the primary content of an .inc file,
 * whereas BitBake uses '#' only for "# comment" lines (always '#' + space).
 * We therefore match ObjectScript preprocessor directives ('#' immediately
 * followed by 'def'/';'), which BitBake never produces. Defaults to BitBake on
 * any doubt (preserves existing behaviour). */
ANILanguage ani_disambiguate_inc(const char *path) {
    if (!path) {
        return ANI_LANG_BITBAKE;
    }

    FILE *f = ani_fopen(path, "r");
    if (!f) {
        return ANI_LANG_BITBAKE;
    }

    char buf[ANI_SZ_4K + SKIP_ONE];
    size_t n = fread(buf, SKIP_ONE, ANI_SZ_4K, f);
    buf[n] = '\0';
    (void)fclose(f);

    const char *line = buf;
    while (*line) {
        /* ObjectScript include header: a line beginning "ROUTINE <Uppercase>". */
        if (strncmp(line, "ROUTINE ", SLEN("ROUTINE ")) == 0 &&
            isupper((unsigned char)line[SLEN("ROUTINE ")])) {
            return ANI_LANG_OBJECTSCRIPT_ROUTINE;
        }
        /* ObjectScript macro directives — the primary content of .inc files.
         * "#define"/"#def1arg" (macro defs) and "#;" (line comment). BitBake's
         * only '#' use is "# comment" (hash + space), so these never collide. */
        if (strncmp(line, "#define", SLEN("#define")) == 0 ||
            strncmp(line, "#def1arg", SLEN("#def1arg")) == 0 ||
            strncmp(line, "#;", SLEN("#;")) == 0) {
            return ANI_LANG_OBJECTSCRIPT_ROUTINE;
        }
        const char *nl = strchr(line, '\n');
        if (!nl) {
            break;
        }
        line = nl + SKIP_ONE;
    }
    return ANI_LANG_BITBAKE;
}

/* Case-insensitive prefix match (portable — no strncasecmp dependency). */
static bool starts_with_ci(const char *s, const char *prefix) {
    for (; *prefix; s++, prefix++) {
        if (tolower((unsigned char)*s) != tolower((unsigned char)*prefix)) {
            return false;
        }
    }
    return true;
}

/* Disambiguate .cfc files: a ColdFusion component may be written in the script
 * dialect ("component { ... }", parsed by the JS-like cfscript grammar) or the
 * tag dialect ("<cfcomponent> ... <cffunction>", parsed by the HTML-derived cfml
 * grammar). The extension table defaults to cfscript because that is what modern
 * Lucee/ACF templates use, but large legacy codebases are predominantly tag-based
 * and feeding those to the wrong grammar fails wholesale. Routing rules:
 *   1. A "<cfcomponent" or top-level "<cffunction" tag ⇒ tag dialect. (The latter
 *      catches "bare" tag components that omit the <cfcomponent> wrapper.) This
 *      wins regardless of any leading <!---/<cfscript>, so it is checked first.
 *   2. Otherwise the file is script-dialect content. Find the first significant
 *      token, skipping whitespace and <!--- ---> comments:
 *        - a leading "<cfscript>" wrapper is still script content ⇒ cfscript;
 *        - a different leading tag (e.g. <cfquery> in a bare-tag file) ⇒ cfml;
 *        - anything else ("component { ... }") ⇒ cfscript.
 * Defaults to ANI_LANG_CFSCRIPT on any doubt (preserves table behaviour). */
ANILanguage ani_disambiguate_cfc(const char *path) {
    if (!path) {
        return ANI_LANG_CFSCRIPT;
    }

    FILE *f = ani_fopen(path, "r");
    if (!f) {
        return ANI_LANG_CFSCRIPT;
    }

    /* Read a generous head: tag components can carry a large license/revision
     * comment block before the <cfcomponent> opener. */
    char buf[ANI_SZ_16K + SKIP_ONE];
    size_t n = fread(buf, SKIP_ONE, ANI_SZ_16K, f);
    buf[n] = '\0';
    (void)fclose(f);

    /* Rule 1: explicit tag-component markers ⇒ tag dialect. */
    if (ani_strcasestr(buf, "<cfcomponent") != NULL || ani_strcasestr(buf, "<cffunction") != NULL) {
        return ANI_LANG_CFML;
    }

    /* Rule 2: locate the first significant token, past whitespace and comments. */
    const char *p = buf;
    for (;;) {
        while (*p && isspace((unsigned char)*p)) {
            p++;
        }
        if (starts_with_ci(p, "<!---")) {
            const char *end = strstr(p + SLEN("<!---"), "--->");
            if (!end) {
                break; /* comment runs past the buffer — treat as no token */
            }
            p = end + SLEN("--->");
            continue;
        }
        break;
    }
    if (*p == '<') {
        /* A leading <cfscript> wrapper is script content; any other leading tag
         * (bare-tag file) is tag content. */
        return starts_with_ci(p, "<cfscript") ? ANI_LANG_CFSCRIPT : ANI_LANG_CFML;
    }
    return ANI_LANG_CFSCRIPT;
}
