#!/usr/bin/env python3
"""Conservative registry integrity check for the Android experimental build.

The repository's build_native.sh calls this path, but the script is absent
from the upstream tree at the time of this branch. This *limited* checker
guards the two PowerVR settings plus registry-entry uniqueness; it is NOT
a substitute for a future complete upstream audit of all cvar categories.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REGISTRY = ROOT / "src" / "gta4_hack_registry.h"
GRAPHICS = ROOT.parent / "src" / "graphics" / "gta4_native" / "graphics_system.cpp"

REQUIRED = {
    "gta4_native_device_features",
    "gta4_native_force_vertex_location_compaction",
}

def main() -> int:
    if not REGISTRY.is_file() or not GRAPHICS.is_file():
        print("Registry validation: required source file is missing", file=sys.stderr)
        return 1
    registry = REGISTRY.read_text(encoding="utf-8")
    source = GRAPHICS.read_text(encoding="utf-8")
    registered = re.findall(r'\{\s*"([a-z][a-zA-Z0-9_]*)"\s*,\s*Kind::', registry)
    if len(registered) != len(set(registered)):
        dupes = sorted({k for k in registered if registered.count(k) > 1})
        print(f"Registry validation: duplicate entries: {dupes}", file=sys.stderr)
        return 1
    defined = set(re.findall(r'REXCVAR_DEFINE_[A-Z0-9_]+\s*\(\s*([a-z][a-zA-Z0-9_]*)', source))
    missing_registry = REQUIRED - set(registered)
    missing_source = REQUIRED - defined
    if missing_registry or missing_source:
        print(f"Registry validation: missing registry={sorted(missing_registry)}, missing declarations={sorted(missing_source)}", file=sys.stderr)
        return 1
    print(f"Limited Android registry check passed ({len(registered)} unique entries; PowerVR settings registered).")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
