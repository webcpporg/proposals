#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Tests of proposals.py, each over a scratch repository of proposals."""

from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

import proposals

PROPOSAL = '''## Why

Because.

## What Changes

- A library.

## Capabilities

### New Capabilities

- `demo`: a library.

### Modified Capabilities

None.

## Impact

- **Kind:** own.
- **Implementation:** not started.
- **Status:** draft.
'''

README = f'''# Proposals

{proposals.TABLE_BEGIN}
{proposals.TABLE_END}
'''


def metadata(**changes: object) -> str:
    meta = {'library': 'demo', 'title': '"demo: a library"', 'kind': 'own', 'port_of': 'null',
            'status': 'draft', 'release': 'null', 'implementation': 'null', 'decided': 'null'}
    meta.update({key: str(value) for key, value in changes.items()})
    body = ''.join(f'  {key}: {value}\n' for key, value in meta.items() if value is not None)
    return f'schema: spec-driven\ncreated: 2026-10-13\nwebcpp:\n{body}'


class Scratch(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = tempfile.TemporaryDirectory()
        self.root = Path(self.directory.name)
        self.saved = proposals.ROOT
        proposals.ROOT = self.root
        (self.root / 'README.md').write_text(README)

    def tearDown(self) -> None:
        proposals.ROOT = self.saved
        self.directory.cleanup()

    def proposal(self, path: str, meta: str, code: bool = False) -> Path:
        change = self.root / path
        (change / 'specs/demo').mkdir(parents=True)
        (change / '.openspec.yaml').write_text(meta)
        (change / 'proposal.md').write_text(PROPOSAL)
        (change / 'design.md').write_text('## Context\n')
        (change / 'tasks.md').write_text('# Tasks\n')
        (change / 'specs/demo/spec.md').write_text('## ADDED Requirements\n')
        if code:
            (change / 'reference/source').mkdir(parents=True)
            (change / 'reference/source/demo.hpp').write_text('// code\n')
        return change

    def problems(self) -> list[str]:
        return [fault for proposal in proposals.proposals() for fault in proposals.faults(proposal)]


class Faults(Scratch):
    def test_a_complete_draft_has_no_fault(self) -> None:
        self.proposal('libraries/demo/openspec/changes/genesis', metadata())
        self.assertEqual(self.problems(), [])

    def test_an_unknown_status_is_refused(self) -> None:
        self.proposal('libraries/demo/openspec/changes/genesis', metadata(status='maybe'))
        self.assertTrue(any('status' in p for p in self.problems()), self.problems())

    def test_the_library_matches_its_directory(self) -> None:
        self.proposal('libraries/other/openspec/changes/genesis', metadata())
        self.assertTrue(any("directory says 'other'" in p for p in self.problems()), self.problems())

    def test_a_port_names_what_it_ports(self) -> None:
        self.proposal('libraries/demo/openspec/changes/genesis', metadata(kind='port'))
        self.assertTrue(any('port_of' in p for p in self.problems()), self.problems())

    def test_an_accepted_proposal_carries_reference_code(self) -> None:
        self.proposal('libraries/demo/openspec/changes/genesis',
                      metadata(status='accepted', decided='2026-10-13'))
        self.assertTrue(any('reference code' in p for p in self.problems()), self.problems())

    def test_a_plan_with_code_is_reference_code(self) -> None:
        change = self.proposal('libraries/demo/openspec/changes/genesis',
                               metadata(status='accepted', decided='2026-10-13'))
        (change / 'reference').mkdir()
        (change / 'reference/plan.md').write_text('# Plan\n\n```cpp\nint main() {}\n```\n')
        self.assertEqual(self.problems(), [])

    def test_a_spike_with_code_is_reference_code(self) -> None:
        change = self.proposal('libraries/demo/openspec/changes/genesis',
                               metadata(status='accepted', decided='2026-10-13'))
        (change / 'reference').mkdir()
        (change / 'reference/spike.md').write_text('# Spike\n\n```cpp\nint main() {}\n```\n')
        self.assertEqual(self.problems(), [])

    def test_prose_without_code_is_no_reference_code(self) -> None:
        change = self.proposal('libraries/demo/openspec/changes/genesis',
                               metadata(status='accepted', decided='2026-10-13'))
        (change / 'reference').mkdir()
        (change / 'reference/notes.md').write_text('# Notes\n\nNo code here.\n')
        self.assertTrue(any('reference code' in p for p in self.problems()), self.problems())

    def test_an_implemented_proposal_is_archived_with_its_release(self) -> None:
        self.proposal('libraries/demo/openspec/changes/genesis',
                      metadata(status='implemented', decided='2026-10-13'), code=True)
        problems = self.problems()
        self.assertTrue(any('names the release' in p for p in problems), problems)
        self.assertTrue(any('is not open' in p for p in problems), problems)

    def test_an_archived_implemented_proposal_has_no_fault(self) -> None:
        self.proposal('libraries/demo/openspec/changes/archive/2026-10-13-genesis',
                      metadata(status='implemented', release='"0.1.0"', decided='2026-10-13'),
                      code=True)
        self.assertEqual(self.problems(), [])

    def test_a_missing_section_is_named(self) -> None:
        change = self.proposal('libraries/demo/openspec/changes/genesis', metadata())
        (change / 'proposal.md').write_text(PROPOSAL.replace('## Why', '## Reason'))
        self.assertTrue(any('"## Why"' in p for p in self.problems()), self.problems())

    def test_impact_names_the_kind_and_the_implementation(self) -> None:
        change = self.proposal('libraries/demo/openspec/changes/genesis', metadata())
        (change / 'proposal.md').write_text(PROPOSAL.replace('- **Implementation:** not started.\n', ''))
        self.assertTrue(any('**Implementation:**' in p for p in self.problems()), self.problems())


class Text(Scratch):
    def test_forbidden_text_is_found_anywhere(self) -> None:
        change = self.proposal('libraries/demo/openspec/changes/genesis', metadata(), code=True)
        (change / 'reference/source/demo.hpp').write_text('// built in /' + 'Users/someone/dev\n')
        self.assertEqual(len(proposals.forbidden_text()), 1)

    def test_forbidden_text_in_an_agent_skill_is_found(self) -> None:
        skill = self.root / '.agents/skills/demo'
        skill.mkdir(parents=True)
        (skill / 'SKILL.md').write_text('---\nname: demo\n---\nOne ' + chr(0x2014) + ' two\n')
        self.assertEqual(proposals.forbidden_text(),
                         ['.agents/skills/demo/SKILL.md:4: text a proposal may not hold'])

    def test_a_placeholder_home_is_allowed(self) -> None:
        change = self.proposal('libraries/demo/openspec/changes/genesis', metadata(), code=True)
        (change / 'reference/source/demo.hpp').write_text('// /' + 'Users/you/emsdk\n')
        self.assertEqual(proposals.forbidden_text(), [])


class Readme(Scratch):
    def test_the_table_lists_every_proposal_organization_first(self) -> None:
        self.proposal('libraries/demo/openspec/changes/genesis', metadata())
        self.proposal('organization/openspec/changes/genesis',
                      metadata(library='organization', kind='organization'))
        text = proposals.readme_text(proposals.proposals())
        rows = [line for line in text.splitlines() if line.startswith('| ') and '---' not in line][1:]
        self.assertEqual([row.split(' | ')[0] for row in rows], ['| organization', '| demo'])

    def test_a_stale_readme_fails_the_check(self) -> None:
        self.proposal('libraries/demo/openspec/changes/genesis', metadata())
        self.assertEqual(proposals.main(['check']), 1)
        proposals.main(['readme'])
        self.assertEqual(proposals.main(['check']), 0)


if __name__ == '__main__':
    unittest.main()
