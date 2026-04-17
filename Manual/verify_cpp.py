#!/usr/bin/env python3
"""
verify_cpp.py

Deterministic verifier: compare MADZINE VCV Rack YAML tooltips against
the ground-truth config{Param,Input,Output,Light,Switch,Button}() calls
in src/{Module}.cpp.

NO LLM judgment. Structural/mechanical checks only.

Checks per module:
  - MISSING IN YAML: cpp has control X but YAML does not
  - EXTRA IN YAML  : YAML has control X but cpp does not
  - TYPE MISMATCH  : cpp and YAML disagree on type (param/input/output/light)
  - DUPLICATE      : same (name, type) appears twice in YAML
                     (same name + different type is allowed: e.g. Trigger
                      as button param AND as trigger input)

Usage:
    python3 Manual/verify_cpp.py

Exit 0 if clean, 1 otherwise.
"""

from __future__ import annotations

import json
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable

import yaml


ROOT = Path(__file__).resolve().parent.parent
SRC_DIR = ROOT / "src"
YAML_DIR = ROOT / "Manual" / "modules_yaml"
PLUGIN_JSON = ROOT / "plugin.json"


# --------------------------------------------------------------------------- #
# cpp tokenisation                                                            #
# --------------------------------------------------------------------------- #

CONFIG_RE = re.compile(
    r"\bconfig(Param|Input|Output|Light|Switch|Button)\s*(?:<[^>]*>)?\s*\("
)

# Config kind -> YAML `type` value
KIND_TO_TYPE = {
    "Param":  "param",
    "Switch": "param",
    "Button": "param",
    "Input":  "input",
    "Output": "output",
    "Light":  "light",
}


@dataclass
class CppCall:
    kind: str           # Param|Input|Output|Light|Switch|Button
    name: str           # expanded (or "<DYNAMIC:...>")
    yaml_type: str      # param|input|output|light
    line: int
    dynamic: bool = False


def _split_args(s: str) -> list[str]:
    """Split a comma-separated argument list, respecting nested ()/{}/[]/"/'."""
    args: list[str] = []
    depth = 0
    buf: list[str] = []
    in_str: str | None = None
    i = 0
    while i < len(s):
        c = s[i]
        if in_str:
            buf.append(c)
            if c == "\\" and i + 1 < len(s):
                buf.append(s[i + 1])
                i += 2
                continue
            if c == in_str:
                in_str = None
        else:
            if c in ('"', "'"):
                in_str = c
                buf.append(c)
            elif c in "([{":
                depth += 1
                buf.append(c)
            elif c in ")]}":
                depth -= 1
                buf.append(c)
            elif c == "," and depth == 0:
                args.append("".join(buf).strip())
                buf = []
            else:
                buf.append(c)
        i += 1
    tail = "".join(buf).strip()
    if tail:
        args.append(tail)
    return args


def _find_matching_paren(text: str, open_idx: int) -> int:
    """Return index of matching ')' for '(' at open_idx, respecting strings."""
    depth = 0
    i = open_idx
    in_str: str | None = None
    while i < len(text):
        c = text[i]
        if in_str:
            if c == "\\" and i + 1 < len(text):
                i += 2
                continue
            if c == in_str:
                in_str = None
        else:
            if c in ('"', "'"):
                in_str = c
            elif c == "(":
                depth += 1
            elif c == ")":
                depth -= 1
                if depth == 0:
                    return i
        i += 1
    return -1


def _strip_comments(src: str) -> str:
    """Remove // and /* */ comments without disturbing line numbering."""
    out = []
    i = 0
    n = len(src)
    in_str: str | None = None
    while i < n:
        c = src[i]
        if in_str:
            out.append(c)
            if c == "\\" and i + 1 < n:
                out.append(src[i + 1])
                i += 2
                continue
            if c == in_str:
                in_str = None
            i += 1
            continue
        if c in ('"', "'"):
            in_str = c
            out.append(c)
            i += 1
            continue
        if c == "/" and i + 1 < n and src[i + 1] == "/":
            # skip to end of line but keep the newline
            while i < n and src[i] != "\n":
                i += 1
            continue
        if c == "/" and i + 1 < n and src[i + 1] == "*":
            i += 2
            while i + 1 < n and not (src[i] == "*" and src[i + 1] == "/"):
                if src[i] == "\n":
                    out.append("\n")
                i += 1
            i += 2
            continue
        out.append(c)
        i += 1
    return "".join(out)


# --------------------------------------------------------------------------- #
# name expression evaluation                                                  #
# --------------------------------------------------------------------------- #

STR_LIT_RE = re.compile(r'"((?:\\.|[^"\\])*)"')
STRING_F_RE = re.compile(r"""(?:rack\s*::\s*)?string\s*::\s*f\s*\(""")
STD_TO_STRING_RE = re.compile(r"std::to_string\s*\(")
STD_STRING_RE    = re.compile(r"std::string\s*\(")


def _decode_c_string(s: str) -> str:
    # minimal decoder for \n \t \" \\
    return (s.replace(r"\n", "\n")
             .replace(r"\t", "\t")
             .replace(r'\"', '"')
             .replace(r"\\", "\\"))


def _apply_printf(fmt: str, args: list) -> str:
    """Very small %d/%s/%i printf implementation. Returns None on failure."""
    out = []
    i = 0
    ai = 0
    while i < len(fmt):
        if fmt[i] == "%" and i + 1 < len(fmt):
            # skip optional width/flags digits, then pick spec
            j = i + 1
            while j < len(fmt) and fmt[j] in "0123456789-+ #.":
                j += 1
            if j >= len(fmt):
                return None  # type: ignore
            spec = fmt[j]
            if spec == "%":
                out.append("%")
                i = j + 1
                continue
            if ai >= len(args):
                return None  # type: ignore
            v = args[ai]
            ai += 1
            if spec in ("d", "i"):
                try:
                    out.append(str(int(v)))
                except Exception:
                    return None  # type: ignore
            elif spec == "s":
                out.append(str(v))
            elif spec in ("f", "g"):
                try:
                    out.append(str(float(v)))
                except Exception:
                    return None  # type: ignore
            else:
                # unsupported specifier
                return None  # type: ignore
            i = j + 1
        else:
            out.append(fmt[i])
            i += 1
    return "".join(out)


def eval_name_expr(expr: str, env: dict[str, int], arrays: dict[str, list[str]] | None = None) -> str | None:
    """
    Try to statically resolve a C++ expression that builds a std::string.
    Supports:
      "literal"
      "a " + "b"
      "a " + std::to_string(expr)
      std::string(voiceNames[i]) + " Foo"
      string::f("fmt %d", i+1)
      FOO[i]    (if FOO is a known const char* array)
      concatenations thereof
    `env` maps loop-var names to their current int values (or None if symbolic).
    `arrays` maps array names to their C-string contents.
    Returns the resolved string, or None if it cannot be statically resolved.
    """
    if arrays is None:
        arrays = {}
    expr = expr.strip()
    if not expr:
        return None

    # Try top-level split on '+'
    parts = _split_plus(expr)
    if len(parts) > 1:
        pieces = []
        for p in parts:
            v = eval_name_expr(p, env, arrays)
            if v is None:
                return None
            pieces.append(v)
        return "".join(pieces)

    # single term
    term = expr.strip()

    # string literal (possibly concatenated implicitly: "a" "b")
    if term.startswith('"'):
        # adjacent string literals are joined in C++: "a" "b" == "ab"
        literals = STR_LIT_RE.findall(term)
        # Only valid if after joining literals (and whitespace) nothing else remains.
        rebuilt = ""
        idx = 0
        clean = True
        for m in STR_LIT_RE.finditer(term):
            between = term[idx:m.start()].strip()
            if between:
                clean = False
                break
            rebuilt += _decode_c_string(m.group(1))
            idx = m.end()
        trailing = term[idx:].strip()
        if clean and not trailing and literals:
            return rebuilt
        # fall through

    # string::f(fmt, args...)
    m = STRING_F_RE.match(term)
    if m:
        open_paren = m.end() - 1
        close_paren = _find_matching_paren(term, open_paren)
        if close_paren == -1 or close_paren != len(term) - 1:
            return None
        inner = term[open_paren + 1:close_paren]
        sub = _split_args(inner)
        if not sub:
            return None
        fmt_str = eval_name_expr(sub[0], env, arrays)
        if fmt_str is None:
            return None
        arg_vals = []
        for a in sub[1:]:
            v = eval_int_or_str(a, env, arrays)
            if v is None:
                return None
            arg_vals.append(v)
        return _apply_printf(fmt_str, arg_vals)

    # std::to_string(expr)
    m = STD_TO_STRING_RE.match(term)
    if m:
        open_paren = m.end() - 1
        close_paren = _find_matching_paren(term, open_paren)
        if close_paren == -1 or close_paren != len(term) - 1:
            return None
        inner = term[open_paren + 1:close_paren]
        v = eval_int_or_str(inner, env, arrays)
        if v is None:
            return None
        try:
            return str(int(v))
        except Exception:
            return str(v)

    # std::string(x)  -> recurse on x (e.g. std::string(voiceNames[i]))
    m = STD_STRING_RE.match(term)
    if m:
        open_paren = m.end() - 1
        close_paren = _find_matching_paren(term, open_paren)
        if close_paren == -1 or close_paren != len(term) - 1:
            return None
        inner = term[open_paren + 1:close_paren]
        return eval_name_expr(inner, env, arrays)

    # Array indexing: FOO[expr]
    m = re.fullmatch(r"(\w+)\s*\[\s*(.+)\s*\]", term)
    if m:
        arr_name = m.group(1)
        idx_expr = m.group(2)
        if arr_name in arrays:
            idx_v = eval_int_or_str(idx_expr, env, arrays)
            try:
                idx_i = int(idx_v)
            except Exception:
                return None
            lst = arrays[arr_name]
            if 0 <= idx_i < len(lst):
                return lst[idx_i]
        return None

    # Bare identifier/array access we can't resolve: give up.
    return None


def _split_plus(expr: str) -> list[str]:
    """Split on '+' at top level (respect parens, strings)."""
    parts: list[str] = []
    depth = 0
    in_str: str | None = None
    buf: list[str] = []
    i = 0
    while i < len(expr):
        c = expr[i]
        if in_str:
            buf.append(c)
            if c == "\\" and i + 1 < len(expr):
                buf.append(expr[i + 1])
                i += 2
                continue
            if c == in_str:
                in_str = None
        else:
            if c in ('"', "'"):
                in_str = c
                buf.append(c)
            elif c in "([{":
                depth += 1
                buf.append(c)
            elif c in ")]}":
                depth -= 1
                buf.append(c)
            elif c == "+" and depth == 0:
                parts.append("".join(buf).strip())
                buf = []
            else:
                buf.append(c)
        i += 1
    tail = "".join(buf).strip()
    if tail:
        parts.append(tail)
    return parts


def eval_int_or_str(expr: str, env: dict[str, int], arrays: dict[str, list[str]] | None = None) -> object | None:
    """
    Evaluate a small C++ expression to an int or str.
    Supports: integer literals, loop vars, i+1, i-1, i*k, simple arithmetic.
    Returns None on failure.
    """
    if arrays is None:
        arrays = {}
    expr = expr.strip()
    if not expr:
        return None

    # try string first
    if expr.startswith('"') or "string::f" in expr or expr.startswith("std::to_string") or expr.startswith("std::string"):
        s = eval_name_expr(expr, env, arrays)
        if s is not None:
            return s

    # array indexing FOO[i]
    m = re.fullmatch(r"(\w+)\s*\[\s*(.+)\s*\]", expr)
    if m and m.group(1) in arrays:
        idx_v = eval_int_or_str(m.group(2), env, arrays)
        try:
            idx_i = int(idx_v)
        except Exception:
            return None
        lst = arrays[m.group(1)]
        if 0 <= idx_i < len(lst):
            return lst[idx_i]
        return None

    # strip trailing 'f'/'u'/'l' from numeric literals handled below

    # pure int?
    try:
        return int(expr)
    except ValueError:
        pass

    # identifier -> env lookup
    if re.fullmatch(r"[A-Za-z_]\w*", expr):
        v = env.get(expr)
        return v  # may be None

    # arithmetic: i+1, i-1, i*6, i * 2, (i+1)
    # Allow only a safe subset (digits, identifiers, + - * /, parens, whitespace)
    if re.fullmatch(r"[\w\s+\-*/()]+", expr):
        # Substitute known identifiers.
        names = set(re.findall(r"[A-Za-z_]\w*", expr))
        # If any name is unresolved, bail
        local = {}
        for n in names:
            if n in env and env[n] is not None:
                local[n] = env[n]
            else:
                return None
        try:
            # Safe: expr is validated to only contain digits/idents/op
            return eval(expr, {"__builtins__": {}}, local)
        except Exception:
            return None

    return None


# --------------------------------------------------------------------------- #
# loop-aware cpp scan                                                         #
# --------------------------------------------------------------------------- #

FOR_RE = re.compile(
    r"for\s*\(\s*(?:int|std::size_t|size_t|unsigned|auto)\s+(\w+)\s*=\s*([^;]+);\s*"
    r"\1\s*(<=|<|>=|>)\s*([^;]+);\s*(?:\1\s*\+\+|\+\+\s*\1|\1\s*--|--\s*\1|\1\s*\+=\s*\d+|\1\s*-=\s*\d+)\s*\)"
)

# #define X 8  or  static const int X = 8;  or  const int X = 8;
# or  constexpr int X = 8;  or  constexpr size_t X = 8;
CONST_INT_RE = re.compile(
    r"(?:#define\s+([A-Z_][A-Z0-9_]*)\s+(-?\d+)\b)"
    r"|(?:(?:static\s+)?(?:constexpr\s+)?const\s+(?:int|size_t|std::size_t|unsigned(?:\s+int)?)\s+"
    r"([A-Za-z_]\w*)\s*=\s*(-?\d+)\s*;)"
)

# const char* FOO[] = { "a", "b", ... };
# static const char* FOO[N] = { "a", "b" };
CHAR_ARRAY_RE = re.compile(
    r"(?:static\s+)?const\s+char\s*\*\s*(\w+)\s*\[[^\]]*\]\s*=\s*\{([^}]*)\}\s*;",
    re.DOTALL,
)


@dataclass
class ForLoop:
    var: str
    start: int
    end: int     # exclusive
    body_start: int
    body_end: int  # exclusive


def _find_loop_body(src: str, open_paren_idx: int) -> tuple[int, int] | None:
    """Given index of '(' after 'for', find body range (start, end_exclusive)."""
    close = _find_matching_paren(src, open_paren_idx)
    if close == -1:
        return None
    # skip whitespace
    j = close + 1
    while j < len(src) and src[j] in " \t\r\n":
        j += 1
    if j >= len(src):
        return None
    if src[j] == "{":
        end = _find_matching_paren_brace(src, j)
        if end == -1:
            return None
        return (j + 1, end)
    # single-statement body: up to next semicolon at depth 0
    depth = 0
    k = j
    in_str: str | None = None
    while k < len(src):
        c = src[k]
        if in_str:
            if c == "\\" and k + 1 < len(src):
                k += 2
                continue
            if c == in_str:
                in_str = None
        else:
            if c in ('"', "'"):
                in_str = c
            elif c in "([{":
                depth += 1
            elif c in ")]}":
                depth -= 1
            elif c == ";" and depth == 0:
                return (j, k + 1)
        k += 1
    return None


def _find_matching_paren_brace(text: str, open_idx: int) -> int:
    depth = 0
    i = open_idx
    in_str: str | None = None
    while i < len(text):
        c = text[i]
        if in_str:
            if c == "\\" and i + 1 < len(text):
                i += 2
                continue
            if c == in_str:
                in_str = None
        else:
            if c in ('"', "'"):
                in_str = c
            elif c == "{":
                depth += 1
            elif c == "}":
                depth -= 1
                if depth == 0:
                    return i
        i += 1
    return -1


def _line_at(src: str, idx: int) -> int:
    return src.count("\n", 0, idx) + 1


def collect_constants(src: str) -> dict[str, int]:
    consts: dict[str, int] = {}
    for m in CONST_INT_RE.finditer(src):
        if m.group(1):
            try:
                consts[m.group(1)] = int(m.group(2))
            except ValueError:
                pass
        else:
            try:
                consts[m.group(3)] = int(m.group(4))
            except ValueError:
                pass
    return consts


def collect_char_arrays(src: str) -> dict[str, list[str]]:
    arrays: dict[str, list[str]] = {}
    for m in CHAR_ARRAY_RE.finditer(src):
        name = m.group(1)
        body = m.group(2)
        items = STR_LIT_RE.findall(body)
        arrays[name] = [_decode_c_string(x) for x in items]
    return arrays


def _resolve_int(expr: str, consts: dict[str, int]) -> int | None:
    expr = expr.strip()
    try:
        return int(expr)
    except ValueError:
        pass
    if expr in consts:
        return consts[expr]
    # simple arithmetic using constants
    if re.fullmatch(r"[\w\s+\-*/()]+", expr):
        local = {}
        for n in set(re.findall(r"[A-Za-z_]\w*", expr)):
            if n in consts:
                local[n] = consts[n]
            else:
                return None
        try:
            return int(eval(expr, {"__builtins__": {}}, local))
        except Exception:
            return None
    return None


def find_loops(src: str, consts: dict[str, int]) -> list[ForLoop]:
    loops = []
    for m in FOR_RE.finditer(src):
        var = m.group(1)
        start_expr = m.group(2).strip()
        op = m.group(3)
        end_expr = m.group(4).strip()
        start = _resolve_int(start_expr, consts)
        if start is None:
            continue
        end = _resolve_int(end_expr, consts)
        if end is None:
            continue
        if op == "<":
            pass
        elif op == "<=":
            end += 1
        elif op == ">":
            start, end = end + 1, start + 1
        elif op == ">=":
            start, end = end, start + 1
        else:
            continue
        # locate body
        paren_open = src.find("(", m.start())
        if paren_open == -1:
            continue
        body = _find_loop_body(src, paren_open)
        if body is None:
            continue
        loops.append(ForLoop(var, start, end, body[0], body[1]))
    return loops


def extract_config_calls(cpp_path: Path) -> tuple[list[CppCall], list[str]]:
    """
    Parse a cpp file and return all configParam/configInput/... calls
    with their resolved names. Names that can't be resolved get a
    "<DYNAMIC:...>" marker and are listed in warnings.
    """
    warnings: list[str] = []
    if not cpp_path.exists():
        return [], [f"cpp file not found: {cpp_path.name}"]

    raw = cpp_path.read_text(encoding="utf-8", errors="replace")
    src = _strip_comments(raw)
    consts = collect_constants(src)
    arrays = collect_char_arrays(src)
    # Also scan for local `const char* voiceNames[N] = {...};` inside function bodies
    # (already covered by CHAR_ARRAY_RE since it doesn't require file-scope).
    loops = find_loops(src, consts)

    calls: list[CppCall] = []

    for m in CONFIG_RE.finditer(src):
        kind = m.group(1)
        open_paren = m.end() - 1
        close_paren = _find_matching_paren(src, open_paren)
        if close_paren == -1:
            continue
        inner = src[open_paren + 1:close_paren]
        args = _split_args(inner)
        if not args:
            continue

        # Name position:
        #   configParam (ID, min, max, def, "name", ...)          -> 4
        #   configInput (ID, "name")                              -> 1
        #   configOutput(ID, "name")                              -> 1
        #   configLight (ID, "name")                              -> 1
        #   configSwitch(ID, min, max, def, "name", {...labels})  -> 4
        #   configButton(ID, "name")                              -> 1
        # But configParam also has a 3-arg form (no name) for legacy use;
        # we only record if a name string exists at the right index.
        if kind in ("Param", "Switch"):
            name_idx = 4
        else:
            name_idx = 1

        if name_idx >= len(args):
            # no name supplied (legacy 4-arg configParam); skip silently
            continue

        name_expr = args[name_idx].strip()
        line = _line_at(src, m.start())

        # find loop variables in scope
        env: dict[str, int | None] = {}
        enclosing: list[ForLoop] = [
            lp for lp in loops if lp.body_start <= m.start() < lp.body_end
        ]

        if not enclosing:
            name = eval_name_expr(name_expr, {}, arrays)
            if name is None:
                # try literal-only extraction (fallback)
                lits = STR_LIT_RE.findall(name_expr)
                if lits and all(STR_LIT_RE.fullmatch(p.strip()) for p in _split_plus(name_expr)):
                    name = "".join(_decode_c_string(x) for x in lits)
            if name is None:
                warnings.append(
                    f"{cpp_path.name}:{line}: cannot resolve {kind} name: {name_expr!r}"
                )
                calls.append(CppCall(kind, f"<DYNAMIC:{name_expr}>", KIND_TO_TYPE[kind], line, True))
            else:
                calls.append(CppCall(kind, name, KIND_TO_TYPE[kind], line))
            continue

        # Enumerate all combinations of enclosing loop values.
        loop_vars = [(lp.var, lp.start, lp.end) for lp in enclosing]

        def _iterate(idx: int, env_build: dict[str, int]):
            if idx == len(loop_vars):
                name = eval_name_expr(name_expr, env_build, arrays)
                if name is None:
                    calls.append(CppCall(kind, f"<DYNAMIC:{name_expr}>", KIND_TO_TYPE[kind], line, True))
                    warnings.append(
                        f"{cpp_path.name}:{line}: cannot resolve {kind} name: "
                        f"{name_expr!r} (env={env_build})"
                    )
                    return
                calls.append(CppCall(kind, name, KIND_TO_TYPE[kind], line))
                return
            var, start, end = loop_vars[idx]
            for v in range(start, end):
                env_build[var] = v
                _iterate(idx + 1, env_build)
            env_build.pop(var, None)

        _iterate(0, {})

    return calls, warnings


# --------------------------------------------------------------------------- #
# comparison                                                                  #
# --------------------------------------------------------------------------- #

@dataclass
class ModuleReport:
    slug: str
    cpp_count: int = 0
    yaml_count: int = 0
    missing: list[tuple[str, str]] = field(default_factory=list)   # in cpp, not yaml
    extra:   list[tuple[str, str]] = field(default_factory=list)   # in yaml, not cpp
    type_mismatch: list[tuple[str, str, str]] = field(default_factory=list)  # name, cpp_type, yaml_type
    duplicates: list[tuple[str, str]] = field(default_factory=list)
    dynamic_names: list[str] = field(default_factory=list)
    note: str = ""

    @property
    def issue_count(self) -> int:
        return (len(self.missing)
                + len(self.extra)
                + len(self.type_mismatch)
                + len(self.duplicates))


def _find_cpp(slug: str) -> Path | None:
    # try exact slug match first
    p = SRC_DIR / f"{slug}.cpp"
    if p.exists():
        return p
    # case-insensitive fallback (e.g. WeiiiDocumenta <-> weiiidocumenta)
    for cand in SRC_DIR.glob("*.cpp"):
        if cand.stem.lower() == slug.lower():
            return cand
    return None


def build_report(slug: str, yaml_path: Path, warnings_out: list[str]) -> ModuleReport:
    rpt = ModuleReport(slug=slug)

    cpp_path = _find_cpp(slug)
    if cpp_path is None:
        rpt.note = "cpp file not found"
        # still load YAML to count
        try:
            ydoc = yaml.safe_load(yaml_path.read_text(encoding="utf-8")) or {}
        except Exception as e:
            rpt.note = f"yaml parse error: {e}"
            return rpt
        ycontrols = ydoc.get("controls") or []
        rpt.yaml_count = len(ycontrols)
        # every yaml entry is 'extra'
        for c in ycontrols:
            rpt.extra.append((c.get("name", "?"), c.get("type", "?")))
        return rpt

    calls, warns = extract_config_calls(cpp_path)
    warnings_out.extend(warns)
    rpt.cpp_count = len(calls)

    # cpp set: (name, yaml_type). Lights are configured but typically not in YAML.
    # We still record them but exclude them from "missing" since YAML schema
    # only allows param/input/output. Collect them as a separate list.
    cpp_items: dict[tuple[str, str], CppCall] = {}
    cpp_dynamic = []
    cpp_lights: list[str] = []
    for c in calls:
        if c.dynamic:
            cpp_dynamic.append(c.name)
            rpt.dynamic_names.append(c.name)
        if c.yaml_type == "light":
            cpp_lights.append(c.name)
            continue
        key = (c.name, c.yaml_type)
        cpp_items.setdefault(key, c)

    # YAML
    try:
        ydoc = yaml.safe_load(yaml_path.read_text(encoding="utf-8")) or {}
    except Exception as e:
        rpt.note = f"yaml parse error: {e}"
        return rpt

    ycontrols = ydoc.get("controls") or []
    rpt.yaml_count = len(ycontrols)

    yaml_items: dict[tuple[str, str], int] = {}
    yaml_by_name: dict[str, list[str]] = {}
    for c in ycontrols:
        n = c.get("name", "")
        t = c.get("type", "")
        key = (n, t)
        yaml_items[key] = yaml_items.get(key, 0) + 1
        yaml_by_name.setdefault(n, []).append(t)

    # duplicates: same (name, type) pair > 1
    for (n, t), cnt in yaml_items.items():
        if cnt > 1:
            rpt.duplicates.append((n, t))

    # missing: cpp has, yaml doesn't
    for (n, t), c in cpp_items.items():
        if (n, t) in yaml_items:
            continue
        # could be a type mismatch: yaml has same name but different type
        other_types = [ot for ot in yaml_by_name.get(n, []) if ot != t]
        if other_types:
            rpt.type_mismatch.append((n, t, ",".join(sorted(set(other_types)))))
        else:
            rpt.missing.append((n, t))

    # extra: yaml has, cpp doesn't
    cpp_by_name: dict[str, list[str]] = {}
    for (n, t) in cpp_items.keys():
        cpp_by_name.setdefault(n, []).append(t)
    # also record cpp lights under the name -> but YAML isn't expected to carry them
    for (n, t), cnt in yaml_items.items():
        if (n, t) in cpp_items:
            continue
        if n in cpp_by_name:
            # counted as type_mismatch above; don't double-report
            continue
        rpt.extra.append((n, t))

    return rpt


# --------------------------------------------------------------------------- #
# main                                                                        #
# --------------------------------------------------------------------------- #

def main() -> int:
    plugin = json.loads(PLUGIN_JSON.read_text(encoding="utf-8"))
    expected_slugs = [m["slug"] for m in plugin["modules"]]

    out_lines: list[str] = []
    def P(s: str = ""):
        out_lines.append(s)
        print(s)

    warnings: list[str] = []
    reports: list[ModuleReport] = []

    for slug in sorted(expected_slugs):
        # Skip the Manual pseudo-module (no configParam tooltips)
        if slug == "Manual":
            continue
        yaml_path = YAML_DIR / f"{slug}.yaml"
        if not yaml_path.exists():
            warnings.append(f"YAML missing for slug '{slug}'")
            continue
        rpt = build_report(slug, yaml_path, warnings)
        reports.append(rpt)

    # Per-module summary
    P("=" * 78)
    P("verify_cpp.py  --  cpp configParam/Input/Output vs YAML controls")
    P("=" * 78)
    P()
    P(f"{'Module':<22} {'cpp':>4} {'yaml':>5} {'miss':>5} {'extra':>6} "
      f"{'tmiss':>6} {'dup':>4} {'dyn':>4}")
    P("-" * 78)

    tot_miss = tot_extra = tot_type = tot_dup = tot_dyn = 0
    for r in reports:
        P(f"{r.slug:<22} {r.cpp_count:>4} {r.yaml_count:>5} "
          f"{len(r.missing):>5} {len(r.extra):>6} "
          f"{len(r.type_mismatch):>6} {len(r.duplicates):>4} "
          f"{len(r.dynamic_names):>4}"
          + (f"  [{r.note}]" if r.note else ""))
        tot_miss += len(r.missing)
        tot_extra += len(r.extra)
        tot_type += len(r.type_mismatch)
        tot_dup += len(r.duplicates)
        tot_dyn += len(r.dynamic_names)

    P("-" * 78)
    P(f"{'TOTAL':<22} {'':>4} {'':>5} {tot_miss:>5} {tot_extra:>6} "
      f"{tot_type:>6} {tot_dup:>4} {tot_dyn:>4}")
    P()

    # Detailed issue listing
    P("=" * 78)
    P("Detailed issues")
    P("=" * 78)
    for r in reports:
        if r.issue_count == 0 and not r.note:
            continue
        P()
        P(f"[{r.slug}]  issues={r.issue_count}" + (f"  note={r.note}" if r.note else ""))
        for (n, t) in r.missing:
            P(f"  MISSING IN YAML    [{t:<6}] {n}")
        for (n, t) in r.extra:
            P(f"  EXTRA IN YAML      [{t:<6}] {n}")
        for (n, ct, yt) in r.type_mismatch:
            P(f"  TYPE MISMATCH      {n!r}: cpp={ct} yaml={yt}")
        for (n, t) in r.duplicates:
            P(f"  DUPLICATE IN YAML  [{t:<6}] {n}")

    # Warnings (dynamic names)
    P()
    P("=" * 78)
    P(f"Warnings: {len(warnings)}")
    P("=" * 78)
    for w in warnings:
        P(f"  - {w}")

    # Top 5 worst
    P()
    P("=" * 78)
    P("Top 5 worst modules by issue count")
    P("=" * 78)
    worst = sorted(reports, key=lambda r: -r.issue_count)[:5]
    for r in worst:
        P(f"  {r.slug:<22} issues={r.issue_count} "
          f"(miss={len(r.missing)} extra={len(r.extra)} "
          f"tmiss={len(r.type_mismatch)} dup={len(r.duplicates)})")

    # Grand totals
    P()
    P("=" * 78)
    P(f"GRAND TOTAL: modules_checked={len(reports)} "
      f"missing={tot_miss} extra={tot_extra} "
      f"type_mismatch={tot_type} duplicate={tot_dup} "
      f"dynamic_names={tot_dyn}")
    P("=" * 78)

    # Write report file
    report_path = Path(__file__).parent / "verify_cpp_report.txt"
    report_path.write_text("\n".join(out_lines) + "\n", encoding="utf-8")

    return 0 if (tot_miss == 0 and tot_extra == 0 and tot_type == 0 and tot_dup == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
