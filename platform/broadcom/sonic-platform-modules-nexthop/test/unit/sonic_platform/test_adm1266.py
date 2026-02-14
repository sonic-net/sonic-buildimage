#!/usr/bin/env python

import pytest
import sys
import tempfile
import os
from unittest.mock import Mock, patch, mock_open

# Import test fixtures
sys.path.insert(0, '../../fixtures')
from fixtures_unit_test import Adm1266Mock

@pytest.fixture(scope="module")
def decode_power_fault_cause():
    from fixtures_unit_test import Adm1266Mock
    adm = Adm1266Mock()
    _decode_power_fault_cause = adm.adm_get_reboot_cause.__globals__['decode_power_fault_cause']
    return _decode_power_fault_cause

PSU_VIN_LOSS_PDIO_MASK_AND_VALUE = 0x0001
OVER_TEMP_PDIO_MASK_AND_VALUE=0x0002

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
        """Test Adm1266.get_blackbox_records by comparing with expected records.

        We use expected_records to validate the blackbox record parsing functionality.
        """
        print("\n--- Testing get_blackbox_records ---")

        adm = Adm1266Mock()
        blackbox_input = adm.get_blackbox_input()
        expected_records = adm.get_expected_records()
        expected_causes = adm.get_expected_causes()

        records = adm.get_blackbox_records()
        exp = expected_records
        assert exp is not None, "expected_records not provided"
        assert len(records) == len(exp), f"Count mismatch: {len(records)} != {len(exp)}"

        for i, e in enumerate(exp):
            a = records[i]
            for k, v in e.items():
                assert k in a, f"[{i}] missing '{k}' in blackbox record"
                assert a[k] == v, f"[{i}] {k}: {a[k]} != {v}"
        print("   Passed")

    def test_get_name(self):
        """Test get_name method returns DPM name."""
        adm = Adm1266Mock()
        name = adm.adm.get_name()
        assert name == "dpm-mock"

    def test_clear_blackbox(self):
        """Test clear_blackbox method clears data."""
        adm = Adm1266Mock()
        # Verify we have data initially
        initial_data = adm.read_blackbox()
        assert len(initial_data) > 0

        # Clear and verify empty
        adm.clear_blackbox()
        cleared_data = adm.read_blackbox()
        assert len(cleared_data) == 1
        assert cleared_data == b"1"

    def test_get_all_faults(self):
        """Test get_all_faults method returns fault list."""
        adm = Adm1266Mock()
        faults = adm.adm.get_all_faults()
        assert isinstance(faults, list)
        assert len(faults) > 0
        # Each fault should have required fields
        for fault in faults:
            assert 'fault_uid' in fault
            assert 'dpm_name' in fault

    def test_module_get_reboot_cause(self):
        """Test module-level get_reboot_cause function."""
        adm = Adm1266Mock()
        result = adm.get_reboot_cause()
        assert result is not None

        reboot_cause, debug_msg = result
        assert reboot_cause is not None
        assert isinstance(debug_msg, str)

    def test_get_reboot_cause_type(self):
        """Test get_reboot_cause_type function."""
        from fixtures_unit_test import Adm1266Mock
        adm = Adm1266Mock()
        # Import the function from the loaded module
        get_reboot_cause_type = adm.adm_get_reboot_cause.__globals__['get_reboot_cause_type']

        # Test with known reboot causes
        causes = ["REBOOT_CAUSE_POWER_LOSS", "REBOOT_CAUSE_WATCHDOG"]
        result = get_reboot_cause_type(causes)
        assert result is not None

    def test_time_since(self):
        """Test time_since function converts timestamp to readable format."""
        from fixtures_unit_test import Adm1266Mock
        adm = Adm1266Mock()
        time_since = adm.adm_get_reboot_cause.__globals__['time_since']

        # Test with 8-byte timestamp
        timestamp = b'\x79\x2e\xee\x02\x00\x00\x00\x00'
        result = time_since('timestamp', timestamp)
        assert isinstance(result, str)
        assert 'seconds after power-on' in result

    def test_channel_names(self):
        """Test channel_names function formats GPIO/PDIO bits."""
        from fixtures_unit_test import Adm1266Mock
        adm = Adm1266Mock()
        channel_names = adm.adm_get_reboot_cause.__globals__['channel_names']

        # Test GPIO formatting
        result = channel_names('gpio_in', 15391)  # From test data
        assert isinstance(result, str)
        assert 'GPIO' in result or '0b' in result

    def test_decode_power_fault_cause_no_match(self, decode_power_fault_cause):
        """Test decode_power_fault_cause decoding when there is no match """
        dpm_signal_to_fault_cause = [
            {
                "pdio_mask": PSU_VIN_LOSS_PDIO_MASK_AND_VALUE,
                "gpio_mask": 0x0000,
                "pdio_value": PSU_VIN_LOSS_PDIO_MASK_AND_VALUE,
                "gpio_value": 0x0000,
                "hw_cause": "TEST_FAULT",
                "hw_desc": "Test fault description",
                "summary": "Test summary",
                "reboot_cause": "REBOOT_CAUSE_HARDWARE_OTHER"
            }
        ]
        hw_cause, hw_desc, summary, reboot_cause = decode_power_fault_cause(
            dpm_signal_to_fault_cause, 0x0000, 0x0000)
        assert hw_cause == ""
        assert hw_desc == ""
        assert summary == ""
        assert reboot_cause == ""

    def test_decode_power_fault_cause_single_match(self, decode_power_fault_cause):
        """Test decode_power_fault_cause decoding when there is only one match """
        # Test single fault match
        dpm_signal_to_fault_cause = [
            {
                "pdio_mask": PSU_VIN_LOSS_PDIO_MASK_AND_VALUE,
                "gpio_mask": 0x0000,
                "pdio_value": PSU_VIN_LOSS_PDIO_MASK_AND_VALUE,
                "gpio_value": 0x0000,
                "hw_cause": "TEST_FAULT",
                "hw_desc": "Test fault description",
                "summary": "Test summary",
                "reboot_cause": "REBOOT_CAUSE_HARDWARE_OTHER"
            }
        ]
        hw_cause, hw_desc, summary, reboot_cause = decode_power_fault_cause(
            dpm_signal_to_fault_cause, PSU_VIN_LOSS_PDIO_MASK_AND_VALUE, 0x0000)
        assert hw_cause == "TEST_FAULT"
        assert hw_desc == "Test fault description"
        assert summary == "Test summary"
        assert reboot_cause == "REBOOT_CAUSE_HARDWARE_OTHER"

    def test_decode_power_fault_cause_multiple_match(self, decode_power_fault_cause):
        """Test decode_power_fault_cause decoding when there are multiple matches """
        # Test multiple fault matches (comma-separated)
        dpm_signal_to_fault_cause = [
            {
                "pdio_mask": PSU_VIN_LOSS_PDIO_MASK_AND_VALUE,
                "gpio_mask": 0x0000,
                "pdio_value": PSU_VIN_LOSS_PDIO_MASK_AND_VALUE,
                "gpio_value": 0x0000,
                "hw_cause": "PSU_VIN_LOSS",
                "hw_desc": "Both PSUs lost input power",
                "summary": "PSU input power lost",
                "reboot_cause": "REBOOT_CAUSE_POWER_LOSS"
            },
            {
                "pdio_mask": OVER_TEMP_PDIO_MASK_AND_VALUE,
                "gpio_mask": 0x0000,
                "pdio_value": OVER_TEMP_PDIO_MASK_AND_VALUE,
                "gpio_value": 0x0000,
                "hw_cause": "OVER_TEMP",
                "hw_desc": "Temperature exceeded threshold",
                "summary": "Overtemperature event",
                "reboot_cause": "REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER"
            }
        ]
        # Both bits set - should get comma-separated results
        hw_cause, hw_desc, summary, reboot_cause = decode_power_fault_cause(
            dpm_signal_to_fault_cause, 
            PSU_VIN_LOSS_PDIO_MASK_AND_VALUE | OVER_TEMP_PDIO_MASK_AND_VALUE,
            0x0000)
        assert hw_cause == "PSU_VIN_LOSS,OVER_TEMP"
        assert hw_desc == "Both PSUs lost input power,Temperature exceeded threshold"
        assert summary == "PSU input power lost,Overtemperature event"
        assert reboot_cause == "REBOOT_CAUSE_POWER_LOSS,REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER"

    @pytest.mark.parametrize("reboot_cause_str", [
        "REBOOT_CAUSE_POWER_LOSS",
        "REBOOT_CAUSE_POWER_LOSS,REBOOT_CAUSE_WATCHDOG",
        "REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC, REBOOT_CAUSE_HARDWARE_OTHER",
        "INVALID_CAUSE"
    ])
    def test_reboot_cause_str_to_type(self, reboot_cause_str):
        """Test reboot_cause_str_to_type handles single and comma-separated causes."""
        from fixtures_unit_test import Adm1266Mock
        adm = Adm1266Mock()
        reboot_cause_str_to_type = adm.adm_get_reboot_cause.__globals__['reboot_cause_str_to_type']
        ChassisBase = adm.adm_get_reboot_cause.__globals__['ChassisBase']

        reboot_cause_to_type = {
            "REBOOT_CAUSE_POWER_LOSS":
                ChassisBase.REBOOT_CAUSE_POWER_LOSS,
            "REBOOT_CAUSE_POWER_LOSS,REBOOT_CAUSE_WATCHDOG":
                ChassisBase.REBOOT_CAUSE_POWER_LOSS,
            "REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC, REBOOT_CAUSE_HARDWARE_OTHER":
                ChassisBase.REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC,
            "INVALID_CAUSE": ChassisBase.INVALID_CAUSE
        }

        reboot_cause_type = reboot_cause_to_type.get(reboot_cause_str, "")
        assert reboot_cause_str_to_type(reboot_cause_str) == reboot_cause_type
