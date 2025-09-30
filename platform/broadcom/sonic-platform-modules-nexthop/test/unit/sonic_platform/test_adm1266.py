#!/usr/bin/env python

import pytest
import sys
import tempfile
import os
from unittest.mock import Mock, patch, mock_open

# Import test fixtures
sys.path.insert(0, '../../fixtures')
from fixtures_unit_test import Adm1266Mock

class TestAdm1266Basic:
    """Test ADM1266 basic properties and interface."""
    def test_read_blackbox(self):
        """Test read_blackbox method"""

        adm = Adm1266Mock()
        blackbox_input = adm.get_blackbox_input()
        expected_records = adm.get_expected_records()
        expected_causes = adm.get_expected_causes()

        print("\n--- Testing read_blackbox ---")
        blackbox_data = adm.read_blackbox()
        assert len(blackbox_data) == len(blackbox_input), \
            "Size mismatch: {len(blackbox_data)} != {len(blackbox_input)}"
        assert blackbox_data == blackbox_input, "Blackbox Data mismatch"
        print("   Passed")

    def test_parse_blackbox(self):
        """Test parse_blackbox method"""
        print("\n--- Testing parse_blackbox ---")
        adm = Adm1266Mock()
        blackbox_input = adm.get_blackbox_input()
        expected_records = adm.get_expected_records()
        expected_causes = adm.get_expected_causes()

        blackbox_data = adm.read_blackbox()
        faults = adm.parse_blackbox(blackbox_data)
        exp = expected_records
        assert exp is not None, "expected_records not provided"
        assert len(faults) == len(exp), f"Fault count mismatch: {len(faults)} != {len(exp)}"
        for i, e in enumerate(exp):
            a = faults[i]
            for k, v in e.items():
                ak = 'uid' if k == 'fault_uid' else k
                assert ak in a, f"[{i}] missing '{ak}' in parsed fault"
                assert a[ak] == v, f"[{i}] {ak} mismatch: {a[ak]} != {v}"
        print("   Passed")

    def test_get_blackbox_records(self):
        """Integration test for Adm1266.get_blackbox_records with optional JSON expectations."""
        print("\n--- Testing get_blackbox_records ---")

        adm = Adm1266Mock()
        blackbox_input = adm.get_blackbox_input()
        expected_records = adm.get_expected_records()
        expected_causes = adm.get_expected_causes()


        records = adm.get_blackbox_records()
        assert len(records) == len(expected_records),\
                f"Count mismatch: {len(records)} != {len(expected_records)}"

        for i, exp in enumerate(expected_records):
            a = records[i]
            for k, v in exp.items():
                assert k in a, f"[{i}] missing '{k}'"
                assert a[k] == v, f"[{i}] {k}: {a[k]} != {v}"
        print("   Passed")

    def test_get_reboot_causes(self):
        """Test Adm1266.get_reboot_causes by rendering expected numerics with same RENDERers.

        We reuse expected_causes (numeric) and render them using RENDER to compare with
        the human-friendly output, avoiding spec duplication.
        """
        print("\n--- Testing get_reboot_causes ---")

        adm = Adm1266Mock()
        blackbox_input = adm.get_blackbox_input()
        expected_records = adm.get_expected_records()
        expected_causes = adm.get_expected_causes()

        causes = adm.get_reboot_causes()
        exp = expected_causes
        assert exp is not None, "expected_causes not provided"
        assert len(causes) == len(exp), f"Count mismatch: {len(causes)} != {len(exp)}"

        print(f"expected_causes: {expected_causes}")
        print(f"actual_causes: {causes}")

        for i, e in enumerate(exp):
            a = causes[i]
            for k, v in e.items():
                assert k in a, f"[{i}] missing '{k}' in reboot cause"
                assert a[k] == v, f"[{i}] {k}: {a[k]} != {v}"
        print("   Passed")
