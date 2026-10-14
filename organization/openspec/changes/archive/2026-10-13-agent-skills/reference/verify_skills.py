# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

# Snapshot of the Agent skills workflow validator. Run from either repository root.
import re
from pathlib import Path
import yaml

paths = sorted(Path('.agents/skills').glob('*/SKILL.md'))
failures = []
for path in paths:
    try:
        parts = re.split(r'^---\s*$', path.read_text(), maxsplit=2, flags=re.M)
        if len(parts) != 3 or parts[0].strip():
            raise ValueError('expected opening and closing YAML delimiters')
        metadata = yaml.safe_load(parts[1])
        if not isinstance(metadata, dict):
            raise ValueError('front matter must be a mapping')
        name = metadata.get('name')
        if name != path.parent.name or not re.fullmatch(r'[a-z0-9]+(?:-[a-z0-9]+)*', name):
            raise ValueError('name must match its kebab-case directory')
        description = metadata.get('description')
        if not isinstance(description, str) or not description.strip():
            raise ValueError('description must be a nonempty string')
    except (ValueError, yaml.YAMLError) as error:
        failures.append(f'{path}: {error}')
if not paths:
    failures.append('no agent skills found')
for failure in failures:
    print(failure)
print(f'{len(paths)} skills, {len(failures)} problems')
raise SystemExit(bool(failures))
