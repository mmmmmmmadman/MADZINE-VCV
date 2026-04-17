#!/usr/bin/env python3
"""
One-shot migration: src/ManualHelpData.hpp + Manual/modules/*.json -> Manual/modules_yaml/{Slug}.yaml

Reads:
  - src/ManualHelpData.hpp  (per-param tooltip, 3-lang)
  - Manual/modules/*.json   (HTML long-form manual, 3-lang)
  - plugin.json             (authoritative slug list)

Writes:
  - Manual/modules_yaml/{Slug}.yaml  (single source of truth per module)
  - Manual/migration_warnings.log    (ambiguous type inference cases)

Usage: python3 Manual/migrate_to_yaml.py
"""

import json
import os
import re
import sys
from pathlib import Path

import yaml


ROOT = Path(__file__).resolve().parent.parent
HPP_PATH = ROOT / "src" / "ManualHelpData.hpp"
JSON_DIR = ROOT / "Manual" / "modules"
YAML_DIR = ROOT / "Manual" / "modules_yaml"
PLUGIN_JSON = ROOT / "plugin.json"
WARN_LOG = ROOT / "Manual" / "migration_warnings.log"


# Slug -> JSON filename stem (matches existing filesystem).
SLUG_TO_FILE = {
    "SwingLFO": "swing_lfo",
    "EuclideanRhythm": "euclidean_rhythm",
    "ADGenerator": "ad_generator",
    "Pinpple": "pinpple",
    "PPaTTTerning": "ppattterning",
    "MADDY": "maddy",
    "TWNC": "twnc",
    "TWNCLight": "twnc_light",
    "TWNC2": "twnc_2",
    "QQ": "q_q",
    "Observer": "observer",
    "U8": "u8",
    "YAMANOTE": "yamanote",
    "KIMO": "kimo",
    "Obserfour": "obserfour",
    "Pyramid": "pyramid",
    "DECAPyramid": "decapyramid",
    "KEN": "ken",
    "Quantizer": "quantizer",
    "EllenRipley": "ellen_ripley",
    "MADDYPlus": "maddy_",
    "NIGOQ": "nigoq",
    "Runshow": "runshow",
    "EnvVCA6": "env_vca_6",
    "WeiiiDocumenta": "weiii_documenta",
    "UniversalRhythm": "universal_rhythm",
    "UniRhythm": "uni_rhythm",
    "SongMode": "songmode",
    "Launchpad": "launchpad",
    "Runner": "runner",
    "Facehugger": "facehugger",
    "Ovomorph": "ovomorph",
    "ALEXANDERPLATZ": "alexanderplatz",
    "SHINJUKU": "shinjuku",
    "Portal": "portal",
    "Drummmmmmer": "drummmmmmer",
    "theKICK": "thekick",
    # Manual module has no JSON (no params either).
    "Manual": None,
}


# Token suffixes that strongly indicate a port type.
INPUT_SUFFIXES = [
    " Trigger",
    " CV",
    " Input",
    " In",
    " Gate",
    " Clock",
    " Reset",
    " V/Oct",
    " V/OCT",
    " (V/Oct)",
    " Audio",
    " Audio L",
    " Audio R",
    " Left",
    " Right",
    " L",
    " R",
    " Return",
    " Return L",
    " Return R",
    " Chain L",
    " Chain R",
]

OUTPUT_SUFFIXES = [
    " Out",
    " Output",
    " Outputs",
    " Envelope",
    " Sum",
    " Send",
    " Send L",
    " Send R",
    " Main L",
    " Main R",
    " Kick Output",
    " CV Output",
    " Gate Output",
    " Mix Output",
]


HPP_WARNINGS = []


def unescape_cpp(s):
    """Reverse escape_cpp: \\n \\\" \\\\ -> actual chars."""
    result = []
    i = 0
    while i < len(s):
        c = s[i]
        if c == "\\" and i + 1 < len(s):
            nxt = s[i + 1]
            if nxt == "n":
                result.append("\n")
            elif nxt == "t":
                result.append("\t")
            elif nxt == "\"":
                result.append("\"")
            elif nxt == "\\":
                result.append("\\")
            else:
                result.append(nxt)
            i += 2
        else:
            result.append(c)
            i += 1
    return "".join(result)


def parse_cpp_string(src, start):
    """Parse a C++ string literal starting at index `start` (must point at opening ")."""
    assert src[start] == "\""
    i = start + 1
    out = []
    while i < len(src):
        c = src[i]
        if c == "\\":
            if i + 1 >= len(src):
                break
            out.append(src[i:i + 2])
            i += 2
        elif c == "\"":
            return "".join(out), i + 1
        else:
            out.append(c)
            i += 1
    raise ValueError(f"Unterminated C++ string starting at {start}")


def parse_triple(src, pos):
    """Parse {"a", "b", "c"} block starting at pos (pointing at '{'). Returns (list[3], end)."""
    assert src[pos] == "{"
    i = pos + 1
    parts = []
    while i < len(src) and src[i] != "}":
        if src[i] == "\"":
            s, i = parse_cpp_string(src, i)
            parts.append(unescape_cpp(s))
        else:
            i += 1
    return parts, i + 1  # skip '}'


def parse_hpp(hpp_text):
    """Parse ManualHelpData.hpp into dict[slug] -> {name, desc:{en,zh,ja}, entries:[(name,{en,zh,ja})]}."""
    modules = {}

    # Split into per-slug blocks by scanning for 'ModuleHelpData m;' sections.
    # Each section ends with `data["Slug"] = std::move(m);`.
    pattern = re.compile(
        r"\{\s*ModuleHelpData m;\s*m\.name\s*=\s*\"([^\"\\]*(?:\\.[^\"\\]*)*)\";\s*",
        re.DOTALL,
    )

    positions = []
    for match in pattern.finditer(hpp_text):
        positions.append((match.start(), match.end(), match.group(1)))

    for i, (blk_start, body_start, name_raw) in enumerate(positions):
        blk_end = positions[i + 1][0] if i + 1 < len(positions) else len(hpp_text)
        block = hpp_text[body_start:blk_end]

        # Module display name.
        mod_name = unescape_cpp(name_raw)

        # m.description = {"en","zh","ja"};
        desc_match = re.search(r"m\.description\s*=\s*\{", block)
        desc = {"en": "", "zh": "", "ja": ""}
        if desc_match:
            d_pos = desc_match.end() - 1  # position of '{'
            parts, _ = parse_triple(block, d_pos)
            if len(parts) >= 3:
                desc = {"en": parts[0], "zh": parts[1], "ja": parts[2]}

        # entries: m.entries.push_back({"Name", {"en","zh","ja"}});
        entries = []
        push_pattern = re.compile(
            r"m\.entries\.push_back\(\{\s*\"",
            re.DOTALL,
        )
        i_pos = 0
        while True:
            m = push_pattern.search(block, i_pos)
            if not m:
                break
            # position of opening quote of the entry name
            q_pos = m.end() - 1
            entry_name, next_pos = parse_cpp_string(block, q_pos)
            entry_name = unescape_cpp(entry_name)
            # Find next '{' after name's comma
            brace_pos = block.find("{", next_pos)
            if brace_pos < 0:
                break
            triple, end_brace = parse_triple(block, brace_pos)
            if len(triple) < 3:
                triple = triple + [""] * (3 - len(triple))
            entries.append(
                (
                    entry_name,
                    {"en": triple[0], "zh": triple[1], "ja": triple[2]},
                )
            )
            i_pos = end_brace

        # Find `data["Slug"] = std::move(m);` after this block.
        slug_match = re.search(
            r"data\[\"([^\"]+)\"\]\s*=\s*std::move\(m\);",
            block,
        )
        if not slug_match:
            continue
        slug = slug_match.group(1)

        modules[slug] = {
            "name": mod_name,
            "description": desc,
            "entries": entries,
        }

    return modules


def infer_type(name):
    """Infer control type from its name. Returns ('param'|'input'|'output', confident:bool)."""
    for suf in OUTPUT_SUFFIXES:
        if name.endswith(suf):
            return "output", True
    for suf in INPUT_SUFFIXES:
        if name.endswith(suf):
            return "input", True
    # Explicit single-letter patterns already covered above; remaining names are params.
    return "param", True


def ambiguous_reasons(name, slug):
    """Known ambiguous suffixes worth logging."""
    reasons = []
    # " L" / " R" might be output (e.g. "Main L") or input (e.g. "Audio L").
    if name.endswith(" L") or name.endswith(" R"):
        reasons.append(f"[{slug}] '{name}': trailing L/R suffix (could be input or output)")
    return reasons


def parse_hp(hp_field):
    """`"12 HP"` -> 12, `12` -> 12, etc."""
    if hp_field is None:
        return None
    if isinstance(hp_field, int):
        return hp_field
    s = str(hp_field).strip()
    m = re.match(r"(\d+(?:\.\d+)?)", s)
    if m:
        val = float(m.group(1))
        return int(val) if val.is_integer() else val
    return None


class LiteralStr(str):
    """Marker for YAML block literal scalar."""
    pass


def literal_str_representer(dumper, data):
    return dumper.represent_scalar("tag:yaml.org,2002:str", data, style="|")


yaml.add_representer(LiteralStr, literal_str_representer)


def str_representer(dumper, data):
    # Multi-line strings -> literal block style.
    if "\n" in data:
        return dumper.represent_scalar("tag:yaml.org,2002:str", data, style="|")
    return dumper.represent_scalar("tag:yaml.org,2002:str", data)


yaml.add_representer(str, str_representer)


def build_yaml_dict(slug, hpp_mod, json_data):
    """Merge hpp module + JSON manual into YAML dict."""
    doc = {}
    doc["slug"] = slug

    name = None
    if hpp_mod:
        name = hpp_mod["name"]
    if not name and json_data:
        name = json_data.get("name", slug)
    doc["name"] = name or slug

    # HP / wide / alt come from JSON.
    hp_val = None
    wide = False
    alt = None
    if json_data:
        hp_val = parse_hp(json_data.get("hp"))
        wide = bool(json_data.get("wide", False))
        alt = json_data.get("alt")
    doc["hp"] = hp_val
    doc["wide"] = wide
    doc["alt"] = alt or f"{doc['name']} panel"

    # Description: prefer hpp (already 3-lang from tooltip source); fallback to JSON.
    desc = {"en": "", "zh": "", "ja": ""}
    if hpp_mod:
        desc["en"] = hpp_mod["description"]["en"]
        desc["zh"] = hpp_mod["description"]["zh"]
        desc["ja"] = hpp_mod["description"]["ja"]
    if json_data:
        if not desc["en"]:
            desc["en"] = json_data.get("description", "") or ""
        if not desc["zh"]:
            desc["zh"] = json_data.get("description_zh", "") or ""
        if not desc["ja"]:
            desc["ja"] = json_data.get("description_ja", "") or ""
    doc["description"] = desc

    # Features (card list) from JSON.
    features = {"en": [], "zh": [], "ja": []}
    if json_data:
        features["en"] = list(json_data.get("features", []) or [])
        features["zh"] = list(json_data.get("features_zh", []) or [])
        features["ja"] = list(json_data.get("features_ja", []) or [])
    doc["features"] = features

    # Raw manual HTML (do not parse in this pass).
    manual_html = {"en": "", "zh": "", "ja": ""}
    if json_data:
        manual_html["en"] = json_data.get("manual", "") or ""
        manual_html["zh"] = json_data.get("manual_zh", "") or ""
        manual_html["ja"] = json_data.get("manual_ja", "") or ""
    doc["manual_html_raw"] = manual_html

    # Controls from hpp entries.
    controls = []
    if hpp_mod:
        for entry_name, tri in hpp_mod["entries"]:
            ctype, _confident = infer_type(entry_name)
            for r in ambiguous_reasons(entry_name, slug):
                HPP_WARNINGS.append(r)
            controls.append(
                {
                    "name": entry_name,
                    "type": ctype,
                    "desc": {
                        "en": tri["en"],
                        "zh": tri["zh"],
                        "ja": tri["ja"],
                    },
                }
            )
    doc["controls"] = controls

    return doc


def count_empty_fields(doc):
    """Return (empty_desc_entries, total_controls)."""
    total = len(doc.get("controls", []))
    empty = sum(
        1
        for c in doc.get("controls", [])
        if not (c["desc"].get("en") or c["desc"].get("zh") or c["desc"].get("ja"))
    )
    return empty, total


def main():
    if not HPP_PATH.exists():
        print(f"ERROR: missing {HPP_PATH}", file=sys.stderr)
        sys.exit(1)
    if not PLUGIN_JSON.exists():
        print(f"ERROR: missing {PLUGIN_JSON}", file=sys.stderr)
        sys.exit(1)

    plugin = json.loads(PLUGIN_JSON.read_text(encoding="utf-8"))
    slugs = [m["slug"] for m in plugin["modules"]]

    hpp_text = HPP_PATH.read_text(encoding="utf-8")
    hpp_modules = parse_hpp(hpp_text)

    YAML_DIR.mkdir(parents=True, exist_ok=True)

    stats = []
    total_controls = 0
    total_empty = 0
    missing_json = []

    for slug in slugs:
        # 'Manual' is a plugin.json stub with no JSON/controls/image — skip it.
        if slug == "Manual":
            continue
        hpp_mod = hpp_modules.get(slug)

        json_stem = SLUG_TO_FILE.get(slug)
        json_data = None
        if json_stem is not None:
            json_path = JSON_DIR / f"{json_stem}.json"
            if json_path.exists():
                try:
                    json_data = json.loads(json_path.read_text(encoding="utf-8"))
                except Exception as e:
                    HPP_WARNINGS.append(f"[{slug}] failed to parse {json_path.name}: {e}")
            else:
                missing_json.append(slug)

        doc = build_yaml_dict(slug, hpp_mod, json_data)
        empty, count = count_empty_fields(doc)
        total_controls += count
        total_empty += empty
        stats.append((slug, count, empty))

        out_path = YAML_DIR / f"{slug}.yaml"
        with out_path.open("w", encoding="utf-8") as f:
            yaml.dump(
                doc,
                f,
                allow_unicode=True,
                default_flow_style=False,
                sort_keys=False,
                width=10_000,
            )

    # Write warnings log.
    if HPP_WARNINGS or missing_json:
        with WARN_LOG.open("w", encoding="utf-8") as f:
            f.write("# Migration warnings\n\n")
            if missing_json:
                f.write("## Slugs without JSON manual\n")
                for s in missing_json:
                    f.write(f"- {s}\n")
                f.write("\n")
            if HPP_WARNINGS:
                f.write("## Type inference ambiguities\n")
                for w in HPP_WARNINGS:
                    f.write(f"- {w}\n")

    # Report.
    print(f"Migrated {len(stats)} modules -> {YAML_DIR}")
    print(f"Total controls: {total_controls}  empty desc entries: {total_empty}")
    print()
    print(f"{'Slug':<22} {'Controls':>8} {'Empty':>7}")
    print("-" * 40)
    for slug, count, empty in stats:
        print(f"{slug:<22} {count:>8} {empty:>7}")
    if missing_json:
        print()
        print(f"Warning: {len(missing_json)} slug(s) without JSON manual: {', '.join(missing_json)}")
    if HPP_WARNINGS:
        print(f"Warning: {len(HPP_WARNINGS)} ambiguous type-inference cases logged to {WARN_LOG}")


if __name__ == "__main__":
    main()
