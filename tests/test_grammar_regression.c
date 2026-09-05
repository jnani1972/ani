/*
 * test_grammar_regression.c — Per-language extraction regression net.
 *
 * Guards against silent extraction breakage when vendored tree-sitter grammars
 * are refreshed. Every ANI_LANG_* enum is exercised: a minimal sample is
 * extracted and checked for (a) no crash (non-NULL result) and (b) a
 * catastrophic-break floor (defs >= min_defs) plus, for confident cases,
 * expected definition names. A future grammar upgrade that renames/removes the
 * node types extraction depends on (e.g. the fwcd kotlin `name`-field drop that
 * produced 0 defs) fails this suite loudly.
 *
 * min_defs convention: code languages use >=1 (a sample with a function/type
 * must yield at least one def); data/config/markup languages use 0 (the check
 * is "parses without crashing"). Tighten a row's min_defs to lock in a count.
 */
#include "test_framework.h"
#include "ani.h"
#include "grammar_cases.h"

static int reg_has_def_any(ANIFileResult *r, const char *name) {
    for (int i = 0; i < r->defs.count; i++) {
        if (strcmp(r->defs.items[i].name, name) == 0)
            return 1;
    }
    return 0;
}

static ANIFileResult *extract(const char *src, ANILanguage lang, const char *proj,
                              const char *path) {
    return ani_extract_file(src, (int)strlen(src), lang, proj, path, 0, NULL, NULL);
}

const GrammarCase ANI_GRAMMAR_CASES[] = {
    /* ── LSP-backed / mainstream code languages (expect named defs) ── */
    {"go",
     ANI_LANG_GO,
     "a.go",
     "package m\nfunc Foo() {}\nfunc Bar() int { return 0 }\n",
     2,
     {"Foo", "Bar", NULL}},
    {"c",
     ANI_LANG_C,
     "a.c",
     "int foo(void){return 0;}\nint bar(void){return 1;}\n",
     2,
     {"foo", "bar", NULL}},
    {"cpp", ANI_LANG_CPP, "a.cpp", "struct A {};\nint foo(){return 0;}\n", 2, {"A", "foo", NULL}},
    {"cuda",
     ANI_LANG_CUDA,
     "a.cu",
     "__global__ void foo(){}\nint bar(){return 0;}\n",
     1,
     {"bar", NULL}},
    {"python",
     ANI_LANG_PYTHON,
     "a.py",
     "def foo():\n    pass\nclass A:\n    pass\n",
     2,
     {"foo", "A", NULL}},
    {"javascript",
     ANI_LANG_JAVASCRIPT,
     "a.js",
     "function foo(){}\nclass A{}\n",
     2,
     {"foo", "A", NULL}},
    {"typescript",
     ANI_LANG_TYPESCRIPT,
     "a.ts",
     "function foo(): number { return 1; }\nclass A {}\n",
     2,
     {"foo", "A", NULL}},
    {"tsx", ANI_LANG_TSX, "a.tsx", "function foo(): number { return 1; }\n", 1, {"foo", NULL}},
    {"java", ANI_LANG_JAVA, "A.java", "class A {\n    void foo() {}\n}\n", 2, {"A", "foo", NULL}},
    {"kotlin", ANI_LANG_KOTLIN, "a.kt", "fun foo() {}\nclass A\n", 2, {"foo", "A", NULL}},
    {"rust", ANI_LANG_RUST, "a.rs", "fn foo() {}\nstruct A;\n", 2, {"foo", "A", NULL}},
    {"ruby", ANI_LANG_RUBY, "a.rb", "def foo\nend\nclass A\nend\n", 2, {"foo", "A", NULL}},
    {"php", ANI_LANG_PHP, "a.php", "<?php\nfunction foo() {}\nclass A {}\n", 2, {"foo", "A", NULL}},
    {"c_sharp",
     ANI_LANG_CSHARP,
     "A.cs",
     "class A {\n    void Foo() {}\n}\n",
     2,
     {"A", "Foo", NULL}},
    {"bash",
     ANI_LANG_BASH,
     "a.sh",
     "foo() {\n  echo hi\n}\nbar() {\n  foo\n}\n",
     2,
     {"foo", "bar", NULL}},
    {"zsh", ANI_LANG_ZSH, "a.zsh", "foo() {\n  echo hi\n}\nbar() {\n  foo\n}\n", 1, {"foo", NULL}},
    {"lua",
     ANI_LANG_LUA,
     "a.lua",
     "function foo() end\nfunction bar() end\n",
     2,
     {"foo", "bar", NULL}},
    {"luau", ANI_LANG_LUAU, "a.luau", "function foo() end\nfunction bar() end\n", 1, {"foo", NULL}},
    {"perl", ANI_LANG_PERL, "a.pl", "sub foo {}\nsub bar {}\n", 2, {"foo", "bar", NULL}},
    {"dart", ANI_LANG_DART, "a.dart", "class A {}\nvoid foo() {}\n", 2, {"A", "foo", NULL}},
    {"swift", ANI_LANG_SWIFT, "a.swift", "func foo() {}\nclass A {}\n", 2, {"foo", "A", NULL}},
    {"scala", ANI_LANG_SCALA, "a.scala", "object A {\n  def foo() = 1\n}\n", 1, {"A", NULL}},
    {"gdscript", ANI_LANG_GDSCRIPT, "a.gd", "func foo():\n    pass\n", 1, {"foo", NULL}},
    {"groovy", ANI_LANG_GROOVY, "a.groovy", "class A {\n  def foo() {}\n}\n", 1, {"A", NULL}},
    {"zig", ANI_LANG_ZIG, "a.zig", "fn foo() void {}\nfn bar() void {}\n", 1, {"foo", NULL}},
    {"solidity",
     ANI_LANG_SOLIDITY,
     "a.sol",
     "contract A {\n  function foo() public {}\n}\n",
     1,
     {"A", NULL}},
    {"tcl", ANI_LANG_TCL, "a.tcl", "proc foo {} {}\nproc bar {} {}\n", 1, {"foo", NULL}},
    {"powershell",
     ANI_LANG_POWERSHELL,
     "a.ps1",
     "function Get-Foo {\n}\nfunction Get-Bar {\n}\n",
     1,
     {"Get-Foo", NULL}},
    {"r", ANI_LANG_R, "a.R", "foo <- function() {}\nbar <- function() {}\n", 1, {"foo", NULL}},
    {"julia", ANI_LANG_JULIA, "a.jl", "function foo() end\nstruct A end\n", 1, {"foo", NULL}},
    {"matlab",
     ANI_LANG_MATLAB,
     "a.m",
     "function foo()\nend\nfunction bar()\nend\n",
     1,
     {"foo", NULL}},

    /* ── code languages: catastrophic-break floor only (>=1), names vary by grammar ── */
    {"ada", ANI_LANG_ADA, "a.adb", "procedure Foo is\nbegin\n   null;\nend Foo;\n", 1, {NULL}},
    {"agda", ANI_LANG_AGDA, "a.agda", "module M where\nfoo : Set\nfoo = Set\n", 0, {NULL}},
    {"apex", ANI_LANG_APEX, "A.cls", "public class A {\n  void foo() {}\n}\n", 1, {NULL}},
    {"awk",
     ANI_LANG_AWK,
     "a.awk",
     "function foo() { print 1 }\nfunction bar() { print 2 }\n",
     1,
     {NULL}},
    {"cairo", ANI_LANG_CAIRO, "a.cairo", "fn foo() {}\nfn bar() {}\n", 1, {NULL}},
    {"clojure", ANI_LANG_CLOJURE, "a.clj", "(defn foo [] 1)\n(defn bar [] 2)\n", 1, {NULL}},
    {"commonlisp",
     ANI_LANG_COMMONLISP,
     "a.lisp",
     "(defun foo () 1)\n(defun bar () 2)\n",
     1,
     {NULL}},
    {"emacslisp", ANI_LANG_EMACSLISP, "a.el", "(defun foo () 1)\n(defun bar () 2)\n", 1, {NULL}},
    {"crystal", ANI_LANG_CRYSTAL, "a.cr", "def foo\nend\nclass A\nend\n", 1, {NULL}},
    {"d", ANI_LANG_DLANG, "a.d", "void foo() {}\nvoid bar() {}\n", 1, {NULL}},
    {"elixir", ANI_LANG_ELIXIR, "a.ex", "defmodule A do\n  def foo do\n  end\nend\n", 1, {NULL}},
    {"erlang", ANI_LANG_ERLANG, "a.erl", "-module(a).\nfoo() -> ok.\nbar() -> ok.\n", 1, {NULL}},
    {"fennel", ANI_LANG_FENNEL, "a.fnl", "(fn foo [] 1)\n(fn bar [] 2)\n", 1, {NULL}},
    {"fish", ANI_LANG_FISH, "a.fish", "function foo\n  echo hi\nend\n", 1, {NULL}},
    {"fortran", ANI_LANG_FORTRAN, "a.f90", "subroutine foo()\nend subroutine\n", 1, {NULL}},
    {"fsharp", ANI_LANG_FSHARP, "a.fs", "let foo () = 1\nlet bar () = 2\n", 1, {NULL}},
    {"gleam", ANI_LANG_GLEAM, "a.gleam", "pub fn foo() { 1 }\npub fn bar() { 2 }\n", 1, {NULL}},
    {"glsl", ANI_LANG_GLSL, "a.glsl", "void foo() {}\nvoid main() {}\n", 1, {NULL}},
    {"hare", ANI_LANG_HARE, "a.ha", "fn foo() void = void;\nfn bar() void = void;\n", 1, {NULL}},
    {"haskell", ANI_LANG_HASKELL, "a.hs", "foo :: Int\nfoo = 1\n", 1, {NULL}},
    {"hlsl", ANI_LANG_HLSL, "a.hlsl", "void foo() {}\nvoid bar() {}\n", 1, {NULL}},
    {"ispc", ANI_LANG_ISPC, "a.ispc", "void foo() {}\nvoid bar() {}\n", 1, {NULL}},
    {"objc", ANI_LANG_OBJC, "a.m", "@implementation A\n- (void)foo {}\n@end\n", 0, {NULL}},
    {"ocaml", ANI_LANG_OCAML, "a.ml", "let foo () = 1\nlet bar () = 2\n", 1, {NULL}},
    {"odin", ANI_LANG_ODIN, "a.odin", "foo :: proc() {}\nbar :: proc() {}\n", 1, {NULL}},
    {"pascal", ANI_LANG_PASCAL, "a.pas", "procedure Foo;\nbegin\nend;\n", 1, {NULL}},
    {"pony",
     ANI_LANG_PONY,
     "a.pony",
     "actor Main\n  fun foo(): U32 => 1\n",
     2,
     {"Main", "foo", NULL}},
    {"purescript", ANI_LANG_PURESCRIPT, "a.purs", "foo :: Int\nfoo = 1\n", 0, {NULL}},
    {"racket", ANI_LANG_RACKET, "a.rkt", "(define (foo) 1)\n(define (bar) 2)\n", 1, {NULL}},
    {"rescript", ANI_LANG_RESCRIPT, "a.res", "let foo = () => 1\nlet bar = () => 2\n", 1, {NULL}},
    {"scheme", ANI_LANG_SCHEME, "a.scm", "(define (foo) 1)\n(define (bar) 2)\n", 1, {NULL}},
    /* Chialisp: every def is a head symbol of a generic `list`, and they are
     * NESTED inside the top-level `(mod ...)` — the fixture must exercise that
     * nesting, or the walk could stop at `mod` and still look green. */
    {"chialisp",
     ANI_LANG_CHIALISP,
     "a.clsp",
     "(mod (ARG)\n"
     "  (include condition_codes.clib)\n"
     "  (defconstant TWO 2)\n"
     "  (defun-inline square (x) (* x x))\n"
     "  (defmacro twice (v) (qq (+ (unquote v) (unquote v))))\n"
     "  (square ARG)\n"
     ")\n",
     3,
     {"square", "TWO", "twice", NULL}},
    {"slang", ANI_LANG_SLANG, "a.slang", "void foo() {}\nvoid bar() {}\n", 1, {NULL}},
    {"squirrel", ANI_LANG_SQUIRREL, "a.nut", "function foo() {}\nfunction bar() {}\n", 1, {NULL}},
    {"starlark",
     ANI_LANG_STARLARK,
     "a.bzl",
     "def foo():\n    pass\ndef bar():\n    pass\n",
     1,
     {NULL}},
    {"sway", ANI_LANG_SWAY, "a.sw", "fn foo() {}\nfn bar() {}\n", 1, {NULL}},
    {"teal",
     ANI_LANG_TEAL,
     "a.tl",
     "local function foo() end\nlocal function bar() end\n",
     1,
     {NULL}},
    {"vimscript", ANI_LANG_VIMSCRIPT, "a.vim", "function! Foo()\nendfunction\n", 1, {NULL}},
    {"elm", ANI_LANG_ELM, "a.elm", "module M exposing (..)\nfoo = 1\n", 0, {NULL}},
    {"func", ANI_LANG_FUNC, "a.fc", "() foo() {\n}\n", 0, {NULL}},
    {"lean", ANI_LANG_LEAN, "a.lean", "def foo : Nat := 1\ndef bar : Nat := 2\n", 0, {NULL}},
    {"move",
     ANI_LANG_MOVE,
     "a.move",
     "module 0x1::m {\n  public fun foo() {}\n}\n",
     1,
     {"foo", NULL}},
    {"mojo",
     ANI_LANG_MOJO,
     "a.mojo",
     "fn foo() -> Int:\n    return 1\n\nstruct A:\n    fn bar(self) -> Int:\n        return "
     "foo()\n",
     2,
     {"foo", "A", NULL}},
    {"arkts",
     ANI_LANG_ARKTS,
     "a.ets",
     "@Component\nstruct Card {\n  @State title: string = ''\n\n  build() {\n    Column() {\n"
     "      Text(this.title).fontSize(20)\n    }\n  }\n}\n\n@Builder\nfunction rowCard() {\n"
     "  Row() {}\n}\n",
     2,
     {"Card", "build", "rowCard"}},
    {"plsql",
     ANI_LANG_PLSQL,
     "emp_pkg.pkb",
     "CREATE OR REPLACE PACKAGE BODY emp_pkg AS\n"
     "  FUNCTION hire(p_name VARCHAR2) RETURN NUMBER IS\n"
     "  BEGIN\n"
     "    RETURN util_pkg.calc_salary(p_name);\n"
     "  END;\n"
     "END emp_pkg;\n"
     "/\n",
     2,
     {"emp_pkg", "hire", NULL}},
    {"smali",
     ANI_LANG_SMALI,
     "A.smali",
     ".class public LA;\n.super Ljava/lang/Object;\n.method public foo()V\n.end method\n",
     2,
     {"foo", NULL}},
    {"systemverilog",
     ANI_LANG_SYSTEMVERILOG,
     "a.sv",
     "module m;\n  function int foo(); return 0; endfunction\nendmodule\n",
     0,
     {NULL}},
    {"verilog", ANI_LANG_VERILOG, "a.v", "module m;\nendmodule\n", 0, {NULL}},
    {"vhdl", ANI_LANG_VHDL, "a.vhd", "entity foo is\nend foo;\n", 0, {NULL}},
    {"wgsl", ANI_LANG_WGSL, "a.wgsl", "fn foo() {}\nfn bar() {}\n", 1, {NULL}},
    {"tlaplus", ANI_LANG_TLAPLUS, "a.tla", "---- MODULE M ----\nFoo == 1\n====\n", 0, {NULL}},
    {"llvm", ANI_LANG_LLVM_IR, "a.ll", "define void @foo() {\n  ret void\n}\n", 0, {NULL}},
    {"tablegen", ANI_LANG_TABLEGEN, "a.td", "def Foo {}\n", 0, {NULL}},
    {"puppet", ANI_LANG_PUPPET, "a.pp", "class foo {\n}\n", 0, {NULL}},

    /* ── first-party / self-maintained grammars ── */
    {"assembly", ANI_LANG_ASSEMBLY, "a.s", ".global foo\nfoo:\n  ret\n", 0, {NULL}},
    {"nasm", ANI_LANG_NASM, "a.asm", "global foo\nfoo:\n  ret\n", 0, {NULL}},
    {"cfml", ANI_LANG_CFML, "a.cfm", "<cffunction name=\"foo\"></cffunction>\n", 0, {NULL}},
    {"cfscript", ANI_LANG_CFSCRIPT, "a.cfc", "component {\n  function foo() {}\n}\n", 0, {NULL}},
    {"cobol",
     ANI_LANG_COBOL,
     "a.cob",
     "       IDENTIFICATION DIVISION.\n       PROGRAM-ID. FOO.\n       PROCEDURE DIVISION.\n       "
     "    MAIN-PARA.\n               STOP RUN.\n",
     1,
     {"FOO", NULL}},
    {"janet", ANI_LANG_JANET, "a.janet", "(defn foo [] 1)\n(defn bar [] 2)\n", 0, {NULL}},
    {"magma", ANI_LANG_MAGMA, "a.magma", "function foo()\n  return 1;\nend function;\n", 0, {NULL}},
    {"qml", ANI_LANG_QML, "a.qml", "import QtQuick\nItem {\n  function foo() {}\n}\n", 0, {NULL}},
    {"wolfram", ANI_LANG_WOLFRAM, "a.wl", "foo[x_] := x + 1\n", 0, {NULL}},
    {"pine", ANI_LANG_PINE, "a.pine", "foo() =>\n    1\n", 0, {NULL}},
    {"form", ANI_LANG_FORM, "a.frm", "Symbol x;\nLocal F = x;\n", 0, {NULL}},
    {"protobuf",
     ANI_LANG_PROTOBUF,
     "a.proto",
     "syntax = \"proto3\";\n\nmessage Foo {\n  int32 id = 1;\n}\n",
     1,
     {"Foo", NULL}},
    {"soql", ANI_LANG_SOQL, "a.soql", "SELECT Id FROM Account\n", 0, {NULL}},
    {"sosl", ANI_LANG_SOSL, "a.sosl", "FIND {test} IN ALL FIELDS\n", 0, {NULL}},
    {"dotenv", ANI_LANG_DOTENV, "a.env", "FOO=bar\nBAZ=qux\n", 0, {NULL}},

    /* ── data / config / markup / template languages (no-crash floor) ── */
    {"json", ANI_LANG_JSON, "a.json", "{\"a\": 1, \"b\": [2, 3]}\n", 0, {NULL}},
    {"json5", ANI_LANG_JSON5, "a.json5", "{a: 1, b: 2}\n", 0, {NULL}},
    {"jsonnet", ANI_LANG_JSONNET, "a.jsonnet", "{ a: 1, b: 2 }\n", 0, {NULL}},
    {"jsdoc", ANI_LANG_JSDOC, "a.jsdoc", "/** @param {number} x */\n", 0, {NULL}},
    {"yaml", ANI_LANG_YAML, "a.yaml", "a: 1\nb:\n  - 2\n  - 3\n", 0, {NULL}},
    {"k8s", ANI_LANG_K8S, "a.yaml", "apiVersion: v1\nkind: Pod\n", 0, {NULL}},
    {"kustomize", ANI_LANG_KUSTOMIZE, "kustomization.yaml", "resources:\n  - a.yaml\n", 0, {NULL}},
    {"toml", ANI_LANG_TOML, "a.toml", "[section]\nkey = 1\n", 0, {NULL}},
    {"ini", ANI_LANG_INI, "a.ini", "[section]\nkey=1\n", 0, {NULL}},
    {"csv", ANI_LANG_CSV, "a.csv", "a,b,c\n1,2,3\n", 0, {NULL}},
    {"sql", ANI_LANG_SQL, "a.sql", "CREATE TABLE t (id INT);\nSELECT * FROM t;\n", 0, {NULL}},
    {"xml", ANI_LANG_XML, "a.xml", "<root><child>x</child></root>\n", 0, {NULL}},
    {"html", ANI_LANG_HTML, "a.html", "<html><body><p>hi</p></body></html>\n", 0, {NULL}},
    {"css", ANI_LANG_CSS, "a.css", "a { color: red; }\n.x { width: 1px; }\n", 0, {NULL}},
    {"scss", ANI_LANG_SCSS, "a.scss", "$c: red;\na { color: $c; }\n", 0, {NULL}},
    {"markdown", ANI_LANG_MARKDOWN, "a.md", "# Title\n\nSome text.\n", 0, {NULL}},
    {"rst", ANI_LANG_RST, "a.rst", "Title\n=====\n\ntext\n", 0, {NULL}},
    {"dockerfile", ANI_LANG_DOCKERFILE, "Dockerfile", "FROM alpine\nRUN echo hi\n", 0, {NULL}},
    {"makefile", ANI_LANG_MAKEFILE, "Makefile", "all:\n\techo hi\n", 0, {NULL}},
    {"cmake", ANI_LANG_CMAKE, "CMakeLists.txt", "function(foo)\nendfunction()\n", 0, {NULL}},
    {"meson", ANI_LANG_MESON, "meson.build", "project('x', 'c')\n", 0, {NULL}},
    {"gn", ANI_LANG_GN, "a.gn", "executable(\"foo\") {\n}\n", 0, {NULL}},
    {"just", ANI_LANG_JUST, "justfile", "foo:\n\techo hi\n", 0, {NULL}},
    {"hcl", ANI_LANG_HCL, "a.hcl", "resource \"x\" \"y\" {\n  a = 1\n}\n", 0, {NULL}},
    {"nix", ANI_LANG_NIX, "a.nix", "{ foo = 1; bar = 2; }\n", 0, {NULL}},
    {"gomod", ANI_LANG_GOMOD, "go.mod", "module example.com/x\n\ngo 1.21\n", 0, {NULL}},
    {"gotemplate", ANI_LANG_GOTEMPLATE, "a.tmpl", "{{ if .X }}{{ .Y }}{{ end }}\n", 0, {NULL}},
    {"graphql", ANI_LANG_GRAPHQL, "a.graphql", "type Foo {\n  id: ID\n}\n", 0, {NULL}},
    {"prisma", ANI_LANG_PRISMA, "a.prisma", "model Foo {\n  id Int @id\n}\n", 0, {NULL}},
    {"thrift", ANI_LANG_THRIFT, "a.thrift", "service Foo {\n  void ping()\n}\n", 0, {NULL}},
    {"capnp", ANI_LANG_CAPNP, "a.capnp", "struct Foo {\n  id @0 :Int32;\n}\n", 0, {NULL}},
    {"smithy",
     ANI_LANG_SMITHY,
     "a.smithy",
     "namespace example\n\nstructure Foo {\n  id: Integer\n}\n",
     1,
     {"Foo", NULL}},
    {"wit",
     ANI_LANG_WIT,
     "a.wit",
     "interface i {\n  record r { x: u32 }\n  enum e { a, b }\n  f: func() -> u32;\n}\n",
     3,
     {"r", "e", "f", NULL}},
    {"kdl", ANI_LANG_KDL, "a.kdl", "node \"x\" {\n}\n", 0, {NULL}},
    {"ron", ANI_LANG_RON, "a.ron", "(a: 1, b: 2)\n", 0, {NULL}},
    {"nickel", ANI_LANG_NICKEL, "a.ncl", "{ foo = 1, bar = 2 }\n", 0, {NULL}},
    {"pkl", ANI_LANG_PKL, "a.pkl", "foo = 1\nbar = 2\n", 0, {NULL}},
    {"bicep", ANI_LANG_BICEP, "a.bicep", "param foo string\n", 0, {NULL}},
    {"bitbake", ANI_LANG_BITBAKE, "a.bb", "DESCRIPTION = \"x\"\n", 0, {NULL}},
    {"beancount", ANI_LANG_BEANCOUNT, "a.beancount", "2020-01-01 open Assets:Cash\n", 0, {NULL}},
    {"bibtex", ANI_LANG_BIBTEX, "a.bib", "@article{key,\n  title = {X}\n}\n", 0, {NULL}},
    {"po", ANI_LANG_PO, "a.po", "msgid \"x\"\nmsgstr \"y\"\n", 0, {NULL}},
    {"diff", ANI_LANG_DIFF, "a.diff", "--- a\n+++ b\n@@ -1 +1 @@\n-x\n+y\n", 0, {NULL}},
    {"regex", ANI_LANG_REGEX, "a.re", "(foo|bar)+\n", 0, {NULL}},
    {"requirements",
     ANI_LANG_REQUIREMENTS,
     "requirements.txt",
     "flask==1.0\nrequests>=2\n",
     0,
     {NULL}},
    {"properties", ANI_LANG_PROPERTIES, "a.properties", "foo=1\nbar=2\n", 0, {NULL}},
    {"gitignore", ANI_LANG_GITIGNORE, ".gitignore", "*.o\nbuild/\n", 0, {NULL}},
    {"gitattributes", ANI_LANG_GITATTRIBUTES, ".gitattributes", "*.c text\n", 0, {NULL}},
    {"sshconfig", ANI_LANG_SSHCONFIG, "config", "Host x\n  HostName y\n", 0, {NULL}},
    {"hyprlang", ANI_LANG_HYPRLANG, "a.conf", "general {\n  gaps_in = 5\n}\n", 0, {NULL}},
    {"kconfig", ANI_LANG_KCONFIG, "Kconfig", "config FOO\n  bool \"foo\"\n", 0, {NULL}},
    {"linkerscript", ANI_LANG_LINKERSCRIPT, "a.ld", "SECTIONS {\n  .text : {}\n}\n", 0, {NULL}},
    {"devicetree", ANI_LANG_DEVICETREE, "a.dts", "/dts-v1/;\n/ {\n};\n", 0, {NULL}},
    {"jinja2", ANI_LANG_JINJA2, "a.j2", "{% if x %}{{ y }}{% endif %}\n", 0, {NULL}},
    {"liquid", ANI_LANG_LIQUID, "a.liquid", "{% if x %}{{ y }}{% endif %}\n", 0, {NULL}},
    {"blade", ANI_LANG_BLADE, "a.blade.php", "@if($x)\n{{ $y }}\n@endif\n", 0, {NULL}},
    {"vue", ANI_LANG_VUE, "a.vue", "<template><div></div></template>\n", 0, {NULL}},
    {"svelte", ANI_LANG_SVELTE, "a.svelte", "<script>let x = 1;</script>\n<p>{x}</p>\n", 0, {NULL}},
    {"astro", ANI_LANG_ASTRO, "a.astro", "---\nconst x = 1;\n---\n<div>{x}</div>\n", 0, {NULL}},
    {"templ", ANI_LANG_TEMPL, "a.templ", "templ foo() {\n  <div></div>\n}\n", 0, {NULL}},
    {"typst", ANI_LANG_TYPST, "a.typ", "= Title\n\nsome text\n", 0, {NULL}},
    {"mermaid", ANI_LANG_MERMAID, "a.mmd", "graph TD\n  A --> B\n", 0, {NULL}},
};

const size_t ANI_GRAMMAR_CASES_COUNT = sizeof(ANI_GRAMMAR_CASES) / sizeof(ANI_GRAMMAR_CASES[0]);

TEST(grammar_regression_all) {
    int failures = 0;
    size_t n = ANI_GRAMMAR_CASES_COUNT;
    for (size_t i = 0; i < n; i++) {
        const GrammarCase *c = &ANI_GRAMMAR_CASES[i];
        ANIFileResult *r = extract(c->src, c->lang, "reg", c->path);
        if (!r) {
            fprintf(stderr, "  [REG] %-14s extract returned NULL\n", c->name);
            failures++;
            continue;
        }
        if (r->defs.count < c->min_defs) {
            fprintf(stderr, "  [REG] %-14s defs=%d < min=%d  (extraction regression?)\n", c->name,
                    r->defs.count, c->min_defs);
            failures++;
        }
        for (int e = 0; c->expect[e]; e++) {
            if (!reg_has_def_any(r, c->expect[e])) {
                fprintf(stderr, "  [REG] %-14s missing def '%s' (defs=%d)\n", c->name, c->expect[e],
                        r->defs.count);
                failures++;
            }
        }
        ani_free_result(r);
    }
    if (failures > 0) {
        fprintf(stderr, "  [REG] %d grammar-regression check(s) failed across %zu languages\n",
                failures, n);
    }
    ASSERT_EQ(failures, 0);
    PASS();
}

void suite_grammar_regression(void) {
    RUN_TEST(grammar_regression_all);
}
