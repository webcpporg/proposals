# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Validate every case, retain per-run statistics, and summarize independent repetitions."""

import argparse
from collections import defaultdict
import csv
import json
import math
from pathlib import Path
import statistics as stats
from typing import Any

KEYS = ('group', 'variant', 'mode', 'limit', 'trace', 'control', 'exceptions',
        'isolate', 'work')
PARTS = ('entry', 'channel_create', 'handler_setup', 'post_message',
         'post_to_delivery', 'message_handler', 'resume')


def expected_checksum(steps):
    """Compose the affine LCG in logarithmic time, without rerunning the CPU workload."""
    multiplier, increment, value = 1664525, 1013904223, 1
    mask = (1 << 32) - 1
    while steps:
        if steps & 1:
            value = (multiplier * value + increment) & mask
        increment = (multiplier * increment + increment) & mask
        multiplier = (multiplier * multiplier) & mask
        steps >>= 1
    return value


def percentile(ordered, fraction):
    # Nearest rank: no interpolation that suggests finer clock resolution.
    return ordered[max(0, math.ceil(len(ordered) * fraction) - 1)]


def analyze_case(case, data):
    samples = case['samples']
    pairs = data['pairs']
    assert data['status'] == 0 and data['completions'] == 1
    assert len(pairs) == 2 * samples
    assert data['environment']['visibility'] == 'visible'
    assert data['environment']['isolated'] == bool(case['isolate'])
    for key, value in case.items():
        assert str(value) == data['parameters'][key], (case['id'], key)
    expected_ops = samples * case['limit'] + 1 if case['mode'] == 'chain' else samples
    assert data['operations'] == expected_ops
    assert data['sync_calls'] == (samples if case['mode'] == 'sync' else 0)
    if case['mode'] == 'chain':
        # An i32 EM_JS argument preserves the bits but can arrive signed.
        assert data['checksum'] & 0xffffffff == expected_checksum(expected_ops * case['work'])
    durations = [(b - a) * 1000 for a, b in zip(pairs[::2], pairs[1::2])]
    assert all(math.isfinite(x) and x >= 0 for x in durations)
    ordered = sorted(durations)
    row = {key: case[key] for key in KEYS}
    row.update(id=case['id'], rep=case['rep'], samples=samples,
               operations=expected_ops, elapsed_ms=data['elapsed_ms'],
               mean_us=stats.mean(durations), p50_us=percentile(ordered, 0.5),
               p95_us=percentile(ordered, 0.95), p99_us=percentile(ordered, 0.99),
               max_us=max(durations), zero_percent=100 * durations.count(0) / samples,
               million_ops_per_second=expected_ops / data['elapsed_ms'] / 1000,
               js_elapsed_ms=data['js_elapsed_ms'], wall_elapsed_ms=data['wall_elapsed_ms'],
               clock_step_us=data['performance_clock']['minimum_positive_ms'] * 1000,
               js_to_cpp_sync_ns=data.get('js_wasm_sync_ns_per_call', 0))
    for clock in ('performance_clock', 'cpp_clock', 'wall_clock'):
        if clock in data:
            row[clock + '_loop_ns'] = data[clock]['observed_loop_ns_per_read']
    for check in data['clock_checks']:
        assert check['max_outside_bracket_ms'] < 0.002
        assert check['wall_max_outside_bracket_ms'] < 1.01
    if case['trace']:
        trace = data['trace']
        expected = 0 if case['control'] or case['mode'] in ('clock', 'sync') else samples
        assert len(trace) == 6 * expected
        if expected:
            components = [[] for _ in PARTS]
            for i in range(samples):
                ticks = [pairs[2 * i], *trace[6 * i:6 * i + 6], pairs[2 * i + 1]]
                for j, values in enumerate(components):
                    delta = (ticks[j + 1] - ticks[j]) * 1000
                    # C++ rounds milliseconds to integer nanoseconds and back.
                    assert delta >= -0.001, (case['id'], i, j, delta)
                    values.append(delta)
            for name, values in zip(PARTS, components):
                row[name + '_us'] = stats.mean(values)
            assert abs(sum(row[name + '_us'] for name in PARTS) - row['mean_us']) < 1e-6
    return row


def summarize(rows):
    grouped = defaultdict(list)
    indexed = {}
    for row in rows:
        key = tuple(row[name] for name in KEYS)
        grouped[key].append(row)
        indexed[(key, row['rep'])] = row
    output = []
    for key, values in sorted(grouped.items()):
        group: dict[str, Any] = dict(zip(KEYS, key))
        group['repetitions'] = len(values)
        group['total_samples'] = sum(row['samples'] for row in values)
        for metric in ('mean_us', 'p50_us', 'p95_us', 'p99_us', 'max_us', 'elapsed_ms',
                       'million_ops_per_second', 'zero_percent', 'clock_step_us',
                       'js_to_cpp_sync_ns', 'performance_clock_loop_ns',
                       'cpp_clock_loop_ns', 'wall_clock_loop_ns'):
            observed = [row[metric] for row in values if metric in row]
            if observed:
                group[metric] = dict(median=stats.median(observed), min=min(observed),
                                     max=max(observed), mean=stats.mean(observed))
        if PARTS[0] + '_us' in values[0]:
            # Means add to the mean round trip; medians generally do not.
            group['component_means_us'] = {
                name: stats.mean(row[name + '_us'] for row in values) for name in PARTS}
        if group['mode'] == 'chain' and not group['control'] and not group['trace']:
            controls = list(key)
            controls[KEYS.index('control')] = 1
            ratios, net = [], []
            for row in values:
                control = indexed[(tuple(controls), row['rep'])]
                ratios.append(row['elapsed_ms'] / control['elapsed_ms'])
                net.append((row['elapsed_ms'] - control['elapsed_ms']) * 1000 / row['samples'])
            group['paired_slowdown'] = dict(median=stats.median(ratios),
                                            min=min(ratios), max=max(ratios))
            group['paired_net_us_per_yield'] = dict(median=stats.median(net),
                                                    min=min(net), max=max(net))
        output.append(group)
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    environment = json.loads((args.input / 'environment.json').read_text())
    cases = environment['config']['cases']
    expected = {case['id'] + '.json' for case in cases} | {'environment.json'}
    assert {p.name for p in args.input.glob('*.json')} == expected
    rows = [analyze_case(case, json.loads((args.input / (case['id'] + '.json')).read_text()))
            for case in cases]
    args.output.mkdir(parents=True, exist_ok=True)
    with (args.output / 'runs.csv').open('w', newline='') as stream:
        fields = list(dict.fromkeys(key for row in rows for key in row))
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)
    summary = dict(browser=environment['version']['Browser'], started=environment['started'],
                   seed=environment['config']['seed'], cases=len(rows),
                   total_operations=sum(row['operations'] for row in rows),
                   total_samples=sum(row['samples'] for row in rows), groups=summarize(rows))
    (args.output / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
    print(f'Validated {len(rows)} cases; {summary["total_operations"]:,} operations; '
          f'{summary["total_samples"]:,} timing pairs.')
    for group in summary['groups']:
        if group['group'] == 'primary' and not group['trace'] and not group['control']:
            print(group['variant'], group['mode'], group['limit'],
                  'mean us', round(group['mean_us']['median'], 3),
                  'p95 us', round(group['p95_us']['median'], 3),
                  'slowdown', round(group.get('paired_slowdown', {}).get('median', 0), 3))


if __name__ == '__main__':
    main()
