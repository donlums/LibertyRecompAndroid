#!/usr/bin/env python3
"""Validate the safe-mode registry and required renderer rollback settings."""
from collections import Counter
from pathlib import Path
import re
import sys

HEADER = Path(__file__).resolve().parents[1] / "src" / "gta4_hack_registry.h"
ENTRY = re.compile(
    r'\{\s*"([A-Za-z_][A-Za-z_0-9]*)"\s*,\s*Kind::(k\w+)\s*,'
    r'\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*\}'
)
REQUIRED = {
    "gta4_native_dedupe_vertex_bindings": ("kShortcut", "false"),
    "gta4_native_dedupe_index_bindings": ("kShortcut", "false"),
}
KINDS = {"kShortcut", "kGameplay", "kGuard", "kArchitecture", "kSetting", "kDiagnostic"}


def main():
    source = HEADER.read_text(encoding="utf-8")
    registry = source.split("kRegistry = std::to_array<Entry>({", 1)[1].split("});", 1)[0]
    registry = re.sub(r"(?m)^\s*//[^\n]*", "", registry)
    entries = ENTRY.findall(registry)
    leftover = ENTRY.sub("", registry)
    errors = []
    if not entries or leftover.strip(" ,\t\r\n"):
        errors.append("Unparsed registry entry or empty registry")
    duplicates = [name for name, count in Counter(row[0] for row in entries).items() if count > 1]
    if duplicates:
        errors.append("Duplicate entries: " + ", ".join(sorted(duplicates)))
    for name, kind, value, summary in entries:
        if kind not in KINDS:
            errors.append(f"{name}: invalid kind {kind}")
    values = {name: (kind, value) for name, kind, value, _ in entries}
    for name, expected in REQUIRED.items():
        if values.get(name) != expected:
            errors.append(f"{name}: missing or incorrect safe-mode value (expected {expected})")
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    print(f"Hack registry: validated {len(entries)} entries and {len(REQUIRED)} required rollback settings")
    return 0


if __name__ == "__main__":
    sys.exit(main())
