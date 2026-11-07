# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Prepare reproducible, shuffled browser cases after both Release builds finish."""

import argparse
import json
from pathlib import Path
import random

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('build_root', type=Path)
parser.add_argument('output', type=Path)
parser.add_argument('--seed', type=int, default=20261015)
args = parser.parse_args()
root = args.build_root.resolve()
scripts = {}
for exceptions in ('on', 'off'):
    manifests = list((root / f'yield-benchmark-{exceptions}').rglob('*.variants.json'))
    if len(manifests) != 1:
        raise SystemExit(f'expected one manifest for exceptions={exceptions}, got {manifests}')
    manifest = manifests[0]
    for variant, entry in json.loads(manifest.read_text()).items():
        scripts[exceptions, variant] = '/build/' + str(
            (manifest.parent / entry['script']).relative_to(root))

cases = []
limits = (1, 10, 100, 1000, 5000, 10000)


def add(group, rep, variant, mode, *, limit=1, trace=0, control=0,
        exceptions='on', isolate=1, work=0, samples=10000):
    case = dict(group=group, rep=rep, variant=variant, mode=mode, limit=limit,
                trace=trace, control=control, exceptions=exceptions, isolate=isolate,
                work=work, samples=samples)
    case['id'] = '-'.join(str(value) for value in case.values())
    if variant != 'js':
        case['script'] = scripts[exceptions, variant]
    cases.append(case)


for rep in range(1, 8):
    for variant in ('jspi', 'asyncify'):
        for trace in (0, 1):
            add('primary', rep, variant, 'direct', trace=trace)
        for mode in ('clock', 'sync'):
            add('primary', rep, variant, mode, samples=100000)
        for limit in limits:
            # At least 100,000 handlers and 1,024 yield samples per chain.
            samples = max(1024, (100000 + limit - 1) // limit)
            for control in (0, 1):
                add('primary', rep, variant, 'chain', limit=limit,
                    samples=samples, control=control)
    for trace in (0, 1):
        add('js-control', rep, 'js', 'js-direct', trace=trace, exceptions='na')

for rep in range(1, 4):
    for variant in ('jspi', 'asyncify'):
        for trace in (0, 1):
            add('no-exceptions', rep, variant, 'direct', exceptions='off', trace=trace)
            add('ordinary-page', rep, variant, 'direct', isolate=0, trace=trace)
        for limit in limits:
            add('chain-trace', rep, variant, 'chain', limit=limit, trace=1, samples=1024)
            samples = max(1024, (100000 + limit - 1) // limit)
            for control in (0, 1):
                add('no-exceptions', rep, variant, 'chain', limit=limit,
                    exceptions='off', samples=samples, control=control)
                add('work-64', rep, variant, 'chain', limit=limit, work=64,
                    samples=samples, control=control)

random.Random(args.seed).shuffle(cases)
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(json.dumps(dict(build_root=str(root), seed=args.seed,
                                      cases=cases), indent=2) + '\n')
print(f'Prepared {len(cases)} cases, seed {args.seed}.')
