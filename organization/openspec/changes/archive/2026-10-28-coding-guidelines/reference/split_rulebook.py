# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Split webcpp's AGENTS.md into the new AGENTS.md and the base of CODING-GUIDELINES.md.

Reads AGENTS.md as of ddb6cb2 (2714 lines), cuts it by chapter and section, and rewrites
every "chapter N" reference to the chapter's new home. Writes draft/AGENTS.body.md and
draft/GUIDELINES.body.md; the introductions and the design chapters are written from the
sources they cite.
"""
import re
import sys
from pathlib import Path

SRC = Path(sys.argv[1])
OUT = Path(sys.argv[2])
lines = SRC.read_text().split('\n')
assert len(lines) in (2714, 2715), len(lines)

A, G = 'A', 'G'

# Old chapter -> (document, new chapter), when the reference does not say more.
DEFAULT = {1: (A, 1), 2: (G, 10), 3: (G, 3), 4: (G, 11), 5: (G, 12), 6: None, 7: (A, 2),
           8: (A, 3), 9: (A, 4), 10: (G, 1), 11: (G, 2), 12: (A, 6), 13: (G, 16)}

# Line -> {old chapter: (document, new chapter)} where the default is wrong or absent.
OVERRIDE = {
    78: {6: (G, 4)}, 80: {6: (G, 6)}, 115: {9: (G, 8)}, 180: {6: (G, 9)}, 395: {6: (G, 9)},
    399: {11: (A, 5)}, 400: {11: (A, 5)}, 401: {11: (A, 5)}, 511: {6: (G, 6)},
    533: {11: (A, 5)}, 566: {6: (G, 9)}, 585: {6: (G, 6)}, 970: {6: (G, 4)},
    975: {7: (G, 7)}, 984: {11: (A, 5)}, 1291: {11: (A, 5)}, 1424: {7: (G, 7)},
    1454: {11: (A, 5)}, 2406: {7: (G, 7)}, 2643: {6: (G, 6)}, 2656: {6: (G, 4)},
    2661: {6: (G, 4)}, 2702: {6: (G, 4)}, 2706: {6: (G, 4)},
}

# Where each original line goes: (document, new chapter), by inclusive ranges.
PLACES = [
    (37, 402, A, 1), (403, 575, G, 10), (576, 606, G, 3), (607, 624, G, 11),
    (625, 709, G, 12), (712, 805, G, 4), (806, 864, G, 5), (865, 952, G, 6),
    (953, 1032, G, 9), (1035, 1133, A, 2), (1134, 1186, G, 7), (1187, 1232, A, 2),
    (1233, 1456, A, 3), (1457, 1677, A, 4), (1678, 1706, G, 8), (1707, 2347, A, 4),
    (2348, 2397, G, 1), (2398, 2411, G, 13), (2412, 2478, G, 2), (2479, 2571, A, 5),
    (2572, 2636, A, 6), (2637, 2714, G, 16),
]


def place(number):
    for first, last, doc, chapter in PLACES:
        if first <= number <= last:
            return doc, chapter
    return None


fenced = set()
inside = False
for number, text in enumerate(lines, 1):
    if text.lstrip().startswith('```'):
        inside = not inside
        continue
    if inside:
        fenced.add(number)

PATTERN = re.compile(r'\b([Cc])hapters? (\d+)((?:(?:, and |, | and )\d+)*)')
unresolved = []


def name(doc, here, code):
    if doc == here:
        return ''
    if code:
        return 'guidelines ' if doc == G else 'AGENTS.md '
    file = 'CODING-GUIDELINES.md' if doc == G else 'AGENTS.md'
    return f'`{file}` '


def rewrite(number, text):
    where = place(number)
    if where is None:
        return text
    here = where[0]
    code = number in fenced

    def one(match):
        capital, first, rest = match.group(1), int(match.group(2)), match.group(3)
        olds = [first] + [int(n) for n in re.findall(r'\d+', rest)]
        targets = []
        for old in olds:
            target = OVERRIDE.get(number, {}).get(old, DEFAULT[old])
            if target is None:
                unresolved.append((number, text))
                target = ('?', old)
            targets.append(target)
        docs = {doc for doc, _ in targets}
        word = 'Chapter' if capital == 'C' else 'chapter'
        if len(targets) == 1:
            doc, chapter = targets[0]
            prefix = name(doc, here, code)
            return f'{prefix}{word if not prefix else "chapter"} {chapter}'
        if len(docs) == 1:
            doc = targets[0][0]
            numbers = [str(chapter) for _, chapter in targets]
            joined = ', '.join(numbers[:-1]) + ' and ' + numbers[-1]
            return f'{name(doc, here, code)}chapters {joined}'
        parts = [f'{name(doc, here, code)}chapter {chapter}' for doc, chapter in targets]
        return ', '.join(parts[:-1]) + ' and ' + parts[-1]

    result = PATTERN.sub(one, text)
    if result != text:
        before = [m.group(0) for m in PATTERN.finditer(text)]
        after = [m.group(0) for m in PATTERN.finditer(result)]
        LOG.append(f'{number}: {"; ".join(before)} -> {where[0]} {where[1]}: '
                   + '; '.join(_spans(result)))
    return result


def _spans(result):
    found = re.findall(r'(?:`?(?:CODING-GUIDELINES\.md|AGENTS\.md)`? |guidelines )?'
                       r'chapters? \d+(?:(?:, | and )\d+)*', result)
    return found


LOG = []


CHANGED = '\x00'


def take(first, last):
    out = []
    for number in range(first, last + 1):
        text = rewrite(number, lines[number - 1])
        if text != lines[number - 1] and number not in fenced and not text.startswith('|'):
            text += CHANGED
        out.append(text)
    return out


ITEM = re.compile(r'^(\s*(?:- |\d+\. ))')


def boundary(text):
    bare = text.rstrip(CHANGED)
    return (not bare.strip() or bare.startswith('#') or bare.startswith('|')
            or bare.lstrip().startswith('```'))


def rewrap(block, width=78):
    """Wrap again each paragraph or list item that a rewritten reference made too long."""
    import textwrap
    out = list(block)
    index = 0
    while index < len(out):
        text = out[index]
        if not text.endswith(CHANGED):
            index += 1
            continue
        if len(text) - 1 <= width:
            out[index] = text[:-1]
            index += 1
            continue
        start = index
        while start > 0 and not ITEM.match(out[start]) and not boundary(out[start - 1]):
            start -= 1
        end = index
        while end + 1 < len(out) and not boundary(out[end + 1]) and not ITEM.match(out[end + 1]):
            end += 1
        first = out[start].rstrip(CHANGED)
        marker = ITEM.match(first)
        prefix = marker.group(1) if marker else first[:len(first) - len(first.lstrip())]
        following = ' ' * len(prefix)
        words = ' '.join(part.rstrip(CHANGED).strip() for part in out[start:end + 1])
        words = words[len(prefix.strip()):].strip() if marker else words
        wrapped = textwrap.wrap(words, width=width, initial_indent=prefix,
                                subsequent_indent=following, break_long_words=False,
                                break_on_hyphens=False)
        out[start:end + 1] = wrapped
        index = start + len(wrapped)
    return [text.rstrip(CHANGED) for text in out]


FIXES = [
    ('the tests of `tools/` pin every behaviour this file states.',
     'the tests of `tools/` pin every behaviour this file and\n  `AGENTS.md` state.'),
    ('`AGENTS.md` chapter 4 and `AGENTS.md` chapter 3.', '`AGENTS.md` chapters 4 and 3.'),
    ('with a link to this file as `../../AGENTS.md`)',
     'with links to the superproject\'s `AGENTS.md` as `../../AGENTS.md` and to\n'
     '   this file as `../../CODING-GUIDELINES.md`)'),
    ('  AGENTS.md           this rulebook\n',
     '  AGENTS.md           how to work in the tree: the tools, the commands, the\n'
     '                      lanes, the CI and the releases\n'
     '  CODING-GUIDELINES.md  the rules on how code is written and every design\n'
     '                      decision, with its reason: the single source of them\n'),
]


def fix(block):
    text = '\n'.join(block)
    for old, new in FIXES:
        text = text.replace(old, new)
    return text.split('\n')


def strip(block):
    while block and not block[0].strip():
        block = block[1:]
    while block and not block[-1].strip():
        block = block[:-1]
    return block


def section(title, *blocks):
    out = [title, '']
    for block in blocks:
        out += strip(block) + ['']
    return out


agents = []
agents += section('## 1. What webcpp is', take(38, 402))
agents += section(
    '## 2. The API reference',
    ['The rules of a Doc Comment, its format and what every public symbol needs,',
     'are `CODING-GUIDELINES.md` chapter 7. This chapter is how the reference is',
     'generated from them, and where it sits.'],
    take(1035, 1133), take(1187, 1232))
agents += section('## 3. Documentation', take(1234, 1456))
agents += section(
    '## 4. Tests, lanes, the report, the site and CI',
    take(1458, 1677),
    ['### Tests', '',
     'How a test is written, its framework, its checks and the targets it runs on,',
     'is `CODING-GUIDELINES.md` chapter 8.'],
    take(1707, 2347))
agents += section('## 5. Changelogs and releases', take(2479, 2571))
agents += section('## 6. b2 facts', take(2573, 2636))

guidelines = []
guidelines += section('## 1. Process', take(2349, 2397))
guidelines += section('## 2. Commits and text', take(2413, 2478))
guidelines += section('## 3. Names', take(577, 606))
guidelines += section('## 4. The language', take(713, 805))
guidelines += section('## 5. Bringing a C library into webcpp', take(807, 864))
guidelines += section('## 6. Exceptions and RTTI', take(866, 952))
guidelines += section(
    '## 7. Doc Comments',
    ['How the API reference is generated from them, and where it sits, is',
     '`AGENTS.md` chapter 2.'],
    take(1134, 1186))
guidelines += section('## 8. Tests', take(1679, 1706))
guidelines += section('## 9. What the lint enforces', take(954, 1032))
guidelines += section('## 10. Choosing and registering a port', take(404, 575))
guidelines += section('## 11. Fidelity', take(608, 624))
guidelines += section('## 12. Evidence: oracle and twins', take(626, 709))
guidelines += section('## 13. Porting a library, step by step', take(2399, 2411))
guidelines += ['@@DESIGN@@', '']
guidelines += section('## 16. Roadmap', take(2638, 2714))
guidelines += ['@@NOT-BUILT@@']

agents = rewrap(fix(agents))
guidelines = rewrap(fix(guidelines))
joined = '\n'.join(agents + guidelines)
for old, new in FIXES:
    assert new in joined, f'fix not applied: {old!r}'
OUT.mkdir(parents=True, exist_ok=True)
(OUT / 'AGENTS.body.md').write_text('\n'.join(agents))
(OUT / 'GUIDELINES.body.md').write_text('\n'.join(guidelines))
(OUT / 'citations.txt').write_text('\n'.join(LOG) + '\n')
for number, text in unresolved:
    print(f'UNRESOLVED {number}: {text}')
covered = sum(last - first + 1 for first, last, _, _ in PLACES)
print(f'agents {len(agents)} lines, guidelines {len(guidelines)} lines, placed {covered}')
