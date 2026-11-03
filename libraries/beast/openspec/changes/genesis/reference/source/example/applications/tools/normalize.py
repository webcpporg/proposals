# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Apply reviewed, reproducible static-analysis changes to the pinned application corpus."""

import argparse
import difflib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PATCH = ROOT / "normalization.patch"


def apply():
    if not PATCH.exists():
        return
    lines = PATCH.read_text().splitlines(True)
    cursor = 0
    while cursor < len(lines):
        assert lines[cursor].startswith("--- a/")
        name = lines[cursor][6:].rstrip("\n")
        path = ROOT / name
        assert path.resolve().is_relative_to(ROOT)
        original = path.read_text().splitlines(True)
        cursor += 2
        output, consumed = [], 0
        while cursor < len(lines) and not lines[cursor].startswith("--- a/"):
            header = re.match(r"@@ -(\d+)(?:,\d+)? \+\d+(?:,\d+)? @@", lines[cursor])
            assert header is not None, lines[cursor]
            start = int(header[1]) - 1
            output.extend(original[consumed:start])
            consumed = start
            cursor += 1
            while cursor < len(lines) and lines[cursor][0] in " +-":
                line = lines[cursor]
                if line.startswith("--- a/"):
                    break
                if line[0] in " -":
                    if original[consumed] != line[1:]:
                        raise ValueError(f"{name}: normalization context changed at {consumed + 1}")
                    consumed += 1
                if line[0] in " +":
                    output.append(line[1:])
                cursor += 1
        output.extend(original[consumed:])
        path.write_text("".join(output))


def record(baseline, replacements):
    changes = json.loads(replacements.read_text())
    for name, edits in changes.items():
        path = ROOT / name
        assert path.resolve().is_relative_to(ROOT)
        data = path.read_bytes()
        end = len(data) + 1
        for edit in sorted(edits, key=lambda item: (item["offset"], item["length"]), reverse=True):
            start, length = edit["offset"], edit["length"]
            if start + length > end:
                continue
            data = data[:start] + edit["text"].encode() + data[start + length:]
            end = start
        path.write_bytes(data)
    patch = []
    for original in sorted(baseline.rglob("*")):
        if not original.is_file() or original.suffix not in (".cpp", ".hpp"):
            continue
        relative = original.relative_to(baseline)
        destination = ROOT / relative
        patch.extend(difflib.unified_diff(original.read_text().splitlines(True),
                     destination.read_text().splitlines(True), fromfile=f"a/{relative}",
                     tofile=f"b/{relative}"))
    PATCH.write_text("".join(patch))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path)
    parser.add_argument("--baseline", type=Path)
    args = parser.parse_args()
    if args.record:
        if args.baseline is None:
            parser.error("--record requires the pristine generated --baseline directory")
        record(args.baseline, args.record)
    else:
        apply()


if __name__ == "__main__":
    main()
