# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Check units, checksum transport and rejection of incomplete or misaligned observations."""

import copy
import unittest
from typing import Any

from analyze import analyze_case, expected_checksum, percentile


class AnalysisTest(unittest.TestCase):
    def fixture(self):
        case: dict[str, Any] = dict(group='test', variant='jspi', mode='chain', limit=1, trace=0,
                    control=0, exceptions='on', isolate=1, work=1, samples=2, rep=1, id='test')
        data: dict[str, Any] = dict(status=0, completions=1,
                    pairs=[1, 1.005, 2, 2.010], operations=3,
                    checksum=-2129264258, sync_calls=0, elapsed_ms=1.02,
                    environment=dict(visibility='visible', isolated=True),
                    parameters={key: str(value) for key, value in case.items()},
                    js_elapsed_ms=1.02, wall_elapsed_ms=1, clock_checks=[],
                    performance_clock=dict(minimum_positive_ms=0.005,
                                           observed_loop_ns_per_read=20))
        return case, data

    def test_milliseconds_become_microseconds_and_signed_checksum_keeps_bits(self):
        case, data = self.fixture()
        result = analyze_case(case, data)
        self.assertAlmostEqual(result['mean_us'], 7.5)
        self.assertAlmostEqual(result['p95_us'], 10)
        self.assertEqual(expected_checksum(3), 2165703038)

    def test_affine_checksum_matches_sequential_reference(self):
        value = 1
        for i in range(1001):
            self.assertEqual(expected_checksum(i), value)
            value = (value * 1664525 + 1013904223) & 0xffffffff

    def test_missing_sample_and_incorrect_work_are_rejected(self):
        case, original = self.fixture()
        for key, invalid in [('pairs', original['pairs'][:-1]), ('checksum', 1),
                             ('operations', 2), ('completions', 0)]:
            with self.subTest(field=key):
                data = copy.deepcopy(original)
                data[key] = invalid
                with self.assertRaises(AssertionError):
                    analyze_case(case, data)

    def test_misaligned_trace_is_rejected(self):
        case, data = self.fixture()
        case['trace'] = 1
        data['parameters']['trace'] = '1'
        data['trace'] = [1.001, 1.002, 1.003, 1.004, 1.005, 1.006] * 2
        with self.assertRaises(AssertionError):
            analyze_case(case, data)

    def test_nearest_rank_does_not_invent_sub_resolution_percentiles(self):
        self.assertEqual(percentile([0, 0, 5, 10], 0.5), 0)
        self.assertEqual(percentile([0, 0, 5, 10], 0.95), 10)


if __name__ == '__main__':
    unittest.main()
