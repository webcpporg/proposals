#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""The proposals of webcpp: their index and the checks the CI runs.

Usage: proposals.py readme        rewrites README.md's table from every proposal's metadata
       proposals.py check         every check below; exit 1 when one fails

Every OpenSpec root is `organization/` or `libraries/<name>/`; a proposal is a directory of its
`openspec/changes/` (open) or `openspec/changes/archive/` (archived, named `YYYY-MM-DD-<id>`).
"""

from __future__ import annotations

import re
import sys
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

STATUSES = ('draft', 'proposed', 'accepted', 'implementing', 'implemented', 'rejected', 'withdrawn')
KINDS = ('own', 'port', 'wrapper', 'organization')
# A proposal past these statuses carries reference code an implementer starts from.
WITHOUT_CODE = ('draft', 'proposed', 'rejected', 'withdrawn')
METADATA_KEYS = ('library', 'title', 'kind', 'port_of', 'status', 'release', 'implementation', 'decided')
PROPOSAL_SECTIONS = ('## Why', '## What Changes', '## Capabilities', '## Impact')
IMPACT_FIELDS = ('**Kind:**', '**Implementation:**', '**Status:**')
CODE_SUFFIXES = {'.hpp', '.cpp', '.h', '.c', '.py', '.js', '.mjs', '.ts', '.jam', '.sh'}
# Text no proposal may hold: the name the superproject's lint bans, written so that this file
# does not hold it, and personal data. Files a proposal quotes from elsewhere are checked alike.
FORBIDDEN = (re.compile('v' + 'erus', re.IGNORECASE), re.compile('/' + 'Users/(?!you/)'),
             re.compile('pinho' + 'pro'), re.compile('\\u2014'))
TABLE_BEGIN = '<!-- proposals:begin -->'
TABLE_END = '<!-- proposals:end -->'


@dataclass
class Proposal:
    root: str            # 'organization' or the library's name
    id: str              # the change's id
    path: Path           # its directory
    archived: bool
    metadata: dict

    @property
    def meta(self) -> dict:
        return self.metadata.get('webcpp', {})


def roots() -> list[Path]:
    found = [ROOT / 'organization'] if (ROOT / 'organization/openspec').is_dir() else []
    found += sorted(p for p in (ROOT / 'libraries').glob('*') if (p / 'openspec').is_dir())
    return found


def proposals() -> list[Proposal]:
    found = []
    for root in roots():
        name = root.name
        changes = root / 'openspec/changes'
        for path in sorted(p for p in changes.glob('*') if p.is_dir() and p.name != 'archive'):
            found.append(Proposal(name, path.name, path, False, load(path)))
        for path in sorted(p for p in (changes / 'archive').glob('*') if p.is_dir()):
            change = re.sub(r'^\d{4}-\d{2}-\d{2}-', '', path.name)
            found.append(Proposal(name, change, path, True, load(path)))
    return found


def scalar(text: str) -> str | None:
    """A YAML scalar as .openspec.yaml writes one: null, a quoted string, or a bare word."""
    text = text.strip()
    if text in ('', 'null', '~'):
        return None
    if len(text) >= 2 and text[0] == text[-1] and text[0] in '"\'':
        return text[1:-1]
    return text


def load(path: Path) -> dict:
    """.openspec.yaml: its top-level keys, and its webcpp block of key: value lines."""
    file = path / '.openspec.yaml'
    if not file.exists():
        return {}
    data: dict = {}
    block: dict | None = None
    for line in file.read_text().splitlines():
        if not line.strip() or line.lstrip().startswith('#'):
            continue
        key, _, value = line.strip().partition(':')
        if line.startswith((' ', '\t')) and block is not None:
            block[key] = scalar(value)
        elif value.strip():
            data[key], block = scalar(value), None
        else:
            block = data[key] = {}
    return data


def has_code(path: Path) -> bool:
    reference = path / 'reference'
    if not reference.is_dir():
        return False
    for file in reference.rglob('*'):
        if file.is_file() and file.suffix in CODE_SUFFIXES:
            return True
    # A document of the reference whose fenced blocks hold the code: a plan's tasks, a spike's probes.
    fence = re.compile(r'^```(cpp|c\+\+|c|python|jam|sh|js|mjs)\b', re.M)
    return any(fence.search(document.read_text()) for document in reference.rglob('*.md'))


def faults(proposal: Proposal) -> list[str]:
    where = proposal.path.relative_to(ROOT)
    found = []
    meta = proposal.meta
    if not meta:
        return [f'{where}: .openspec.yaml has no webcpp metadata']
    for key in METADATA_KEYS:
        if key not in meta:
            found.append(f'{where}: the metadata lacks {key}')
    library = 'organization' if proposal.root == 'organization' else proposal.root
    if meta.get('library') != library:
        found.append(f'{where}: library is {meta.get("library")!r}, its directory says {library!r}')
    if meta.get('status') not in STATUSES:
        found.append(f'{where}: status {meta.get("status")!r} is none of {", ".join(STATUSES)}')
    if meta.get('kind') not in KINDS:
        found.append(f'{where}: kind {meta.get("kind")!r} is none of {", ".join(KINDS)}')
    if meta.get('kind') == 'port' and not meta.get('port_of'):
        found.append(f'{where}: a port names what it ports in port_of')
    status = meta.get('status')
    if status == 'implemented' and not meta.get('release'):
        found.append(f'{where}: an implemented proposal names the release it enters')
    if status == 'implemented' and not meta.get('implementation'):
        found.append(f'{where}: an implemented proposal links the commit that holds it')
    if status in ('accepted', 'implementing', 'implemented') and not meta.get('decided'):
        found.append(f'{where}: an accepted proposal names the date it was decided')
    if proposal.archived != (status in ('implemented', 'rejected', 'withdrawn')):
        state = 'archived' if proposal.archived else 'open'
        found.append(f'{where}: a proposal {status} is not {state}')
    if status not in WITHOUT_CODE and not has_code(proposal.path):
        found.append(f'{where}: a proposal {status} carries reference code in reference/')
    text_file = proposal.path / 'proposal.md'
    if not text_file.exists():
        found.append(f'{where}: no proposal.md')
    else:
        text = text_file.read_text()
        for section in PROPOSAL_SECTIONS:
            if not re.search(rf'^{re.escape(section)}\s*$', text, re.M):
                found.append(f'{where}/proposal.md: no section "{section}"')
        impact = text.split('## Impact', 1)[-1]
        for field in IMPACT_FIELDS:
            if field not in impact:
                found.append(f'{where}/proposal.md: Impact names no {field}')
    for name in ('design.md', 'tasks.md'):
        if not (proposal.path / name).exists():
            found.append(f'{where}: no {name}')
    if not list(proposal.path.glob('specs/*/spec.md')):
        found.append(f'{where}: no specs/<capability>/spec.md')
    return found


def forbidden_text() -> list[str]:
    found = []
    for path in sorted(p for p in ROOT.rglob('*') if p.is_file() and '.git' not in p.parts
                       and 'node_modules' not in p.parts):
        try:
            text = path.read_text()
        except (UnicodeDecodeError, OSError):
            continue
        for number, line in enumerate(text.splitlines(), 1):
            if any(pattern.search(line) for pattern in FORBIDDEN):
                found.append(f'{path.relative_to(ROOT)}:{number}: text a proposal may not hold')
                break
    return found


def link(proposal: Proposal) -> str:
    return f'[{proposal.id}]({proposal.path.relative_to(ROOT).as_posix()})'


def table(found: list[Proposal]) -> str:
    order = {name: index for index, name in enumerate(STATUSES)}
    rows = sorted(found, key=lambda p: (p.root != 'organization', p.root, p.id != 'genesis',
                                        order.get(str(p.meta.get('status')), 99), p.id))
    lines = ['| Library | Proposal | Title | Kind | Status | Release | Decided | Implementation |',
             '| --- | --- | --- | --- | --- | --- | --- | --- |']
    for proposal in rows:
        meta = proposal.meta
        kind = meta.get('kind', '')
        if meta.get('port_of'):
            kind = f'{kind} of {meta["port_of"]}'
        implementation = meta.get('implementation')
        implemented = f'[code]({implementation})' if implementation else ''
        lines.append(f'| {proposal.root} | {link(proposal)} | {meta.get("title", "")} | {kind} | '
                     f'{meta.get("status", "")} | {meta.get("release") or ""} | '
                     f'{meta.get("decided") or ""} | {implemented} |')
    return '\n'.join(lines)


def readme_text(found: list[Proposal]) -> str:
    readme = (ROOT / 'README.md').read_text()
    begin, end = readme.index(TABLE_BEGIN) + len(TABLE_BEGIN), readme.index(TABLE_END)
    return readme[:begin] + '\n' + table(found) + '\n' + readme[end:]


def main(arguments: list[str]) -> int:
    if arguments not in (['readme'], ['check']):
        print(__doc__, file=sys.stderr)
        return 2
    found = proposals()
    if arguments == ['readme']:
        (ROOT / 'README.md').write_text(readme_text(found))
        print(f'README.md: {len(found)} proposals')
        return 0
    problems = [fault for proposal in found for fault in faults(proposal)]
    problems += forbidden_text()
    if readme_text(found) != (ROOT / 'README.md').read_text():
        problems.append('README.md: its table is not what `tools/proposals.py readme` writes')
    for problem in problems:
        print(problem)
    print(f'{len(found)} proposals, {len(problems)} problems')
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
