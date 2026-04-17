#!/usr/bin/env python3
"""
Verify Manual/modules_yaml/*.yaml against plugin.json.

Checks:
  1. All 37+1 (Manual) slugs from plugin.json have a matching YAML.
  2. Every YAML's inner `slug` field matches its filename stem.
  3. Every control has a non-empty `name` and `type`.
  4. Counts empty desc entries per module and totals.

Usage: python3 Manual/verify.py
"""

import json
import sys
from pathlib import Path

import yaml


ROOT = Path(__file__).resolve().parent.parent
YAML_DIR = ROOT / "Manual" / "modules_yaml"
PLUGIN_JSON = ROOT / "plugin.json"


def main():
    plugin = json.loads(PLUGIN_JSON.read_text(encoding="utf-8"))
    expected_slugs = [m["slug"] for m in plugin["modules"]]

    yamls = {}
    for p in sorted(YAML_DIR.glob("*.yaml")):
        with p.open("r", encoding="utf-8") as f:
            doc = yaml.safe_load(f) or {}
        yamls[p.stem] = (p, doc)

    errors = []
    warnings = []
    module_stats = []

    # 1. coverage
    yaml_slugs = set(yamls.keys())
    expected_set = set(expected_slugs)

    missing = expected_set - yaml_slugs
    for s in sorted(missing):
        errors.append(f"missing YAML for slug '{s}'")

    extra = yaml_slugs - expected_set
    for s in sorted(extra):
        warnings.append(f"extra YAML not in plugin.json: '{s}'")

    # 2/3/4. per-module checks
    total_controls = 0
    total_empty = 0
    total_missing_name = 0
    total_missing_type = 0

    for slug, (path, doc) in sorted(yamls.items()):
        inner_slug = doc.get("slug")
        if inner_slug != slug:
            errors.append(f"{path.name}: inner slug '{inner_slug}' != filename stem '{slug}'")

        controls = doc.get("controls", []) or []
        empty = 0
        missing_name = 0
        missing_type = 0
        for idx, c in enumerate(controls):
            if not c.get("name"):
                missing_name += 1
                errors.append(f"{slug}: control #{idx} missing name")
            if c.get("type") not in ("param", "input", "output"):
                missing_type += 1
                errors.append(
                    f"{slug}: control '{c.get('name', '?')}' has invalid type '{c.get('type')}'"
                )
            desc = c.get("desc") or {}
            if not (desc.get("en") or desc.get("zh") or desc.get("ja")):
                empty += 1

        total_controls += len(controls)
        total_empty += empty
        total_missing_name += missing_name
        total_missing_type += missing_type
        module_stats.append((slug, len(controls), empty))

    # Report
    print("=" * 60)
    print(f"Coverage: {len(yaml_slugs)} YAML / {len(expected_set)} plugin.json slugs")
    print(f"Totals:   {total_controls} controls, {total_empty} empty desc, "
          f"{total_missing_name} missing name, {total_missing_type} invalid type")
    print("=" * 60)
    print()
    print(f"{'Slug':<22} {'Controls':>8} {'Empty':>7} {'Fill%':>7}")
    print("-" * 46)
    for slug, count, empty in module_stats:
        fill = (count - empty) / count * 100 if count else 100.0
        print(f"{slug:<22} {count:>8} {empty:>7}  {fill:>5.1f}%")

    if warnings:
        print()
        print("Warnings:")
        for w in warnings:
            print(f"  - {w}")

    if errors:
        print()
        print(f"Errors ({len(errors)}):")
        for e in errors[:50]:
            print(f"  - {e}")
        if len(errors) > 50:
            print(f"  ... and {len(errors) - 50} more")
        sys.exit(1)

    print()
    print("OK: all checks passed.")


if __name__ == "__main__":
    main()
