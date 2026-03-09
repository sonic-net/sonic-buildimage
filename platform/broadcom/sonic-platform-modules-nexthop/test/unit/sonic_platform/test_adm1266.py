#!/usr/bin/env python

import pytest
import sys
from unittest.mock import Mock, patch
from fixtures.test_helpers_adm1266 import Adm1266TestMixin


@pytest.fixture
def mock_dpm():
    """Injects and returns a mock DPM module for testing."""
    dpm_mock = Mock()
    dpm_mock.save = Mock()
    with patch.dict(sys.modules, {"sonic_platform.dpm": dpm_mock}):
        yield dpm_mock


@pytest.fixture
def adm1266_module(mock_dpm):
    """Injects and returns a mock ADM1266 module for testing."""
    from sonic_platform import adm1266

    yield adm1266


PSU_VIN_LOSS_PDIO_MASK_AND_VALUE = 0x0001
OVER_TEMP_PDIO_MASK_AND_VALUE=0x0002

class TestAdm1266Basic(Adm1266TestMixin):
    """Test ADM1266 basic properties and interface."""
    def test_read_blackbox(self, adm1266_module):
        """Test read_blackbox method"""

        blackbox_input = self.get_blackbox_input()

        adm = adm1266_module.Adm1266(self.get_fake_adm1266_platform_spec())

        print("\n--- Testing read_blackbox ---")
        blackbox_data = adm.read_blackbox()
        assert len(blackbox_data) == len(blackbox_input), \
            "Size mismatch: {len(blackbox_data)} != {len(blackbox_input)}"
        assert blackbox_data == blackbox_input, "Blackbox Data mismatch"
        print("   Passed")

    def test_parse_blackbox(self, adm1266_module):
        """Test parse_blackbox method"""
        print("\n--- Testing parse_blackbox ---")
        expected_records = self.get_expected_records()

        adm = adm1266_module.Adm1266(self.get_fake_adm1266_platform_spec())

        blackbox_data = adm.read_blackbox()
        faults = adm._parse_blackbox(blackbox_data)
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

    def test_get_blackbox_records(self, adm1266_module):
        """Integration test for Adm1266.get_blackbox_records with optional JSON expectations."""
        print("\n--- Testing get_blackbox_records ---")
        expected_records = self.get_expected_records()

        adm = adm1266_module.Adm1266(self.get_fake_adm1266_platform_spec())

        records = adm.get_blackbox_records()
        assert len(records) == len(expected_records),\
                f"Count mismatch: {len(records)} != {len(expected_records)}"

        for i, exp in enumerate(expected_records):
            a = records[i]
            for k, v in exp.items():
                assert k in a, f"[{i}] missing '{k}'"
                assert a[k] == v, f"[{i}] {k}: {a[k]} != {v}"
        print("   Passed")

    def test_get_reboot_causes(self, adm1266_module):
        """Test Adm1266.get_blackbox_records by comparing with expected records.

        We use expected_records to validate the blackbox record parsing functionality.
        """
        print("\n--- Testing get_blackbox_records ---")

        expected_records = self.get_expected_records()

        adm = adm1266_module.Adm1266(self.get_fake_adm1266_platform_spec())

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

    def test_get_name(self, adm1266_module):
        """Test get_name method returns DPM name."""
        adm = adm1266_module.Adm1266(self.get_fake_adm1266_platform_spec())
        assert adm.get_name() == "dpm-mock"

    def test_clear_blackbox(self, adm1266_module):
        """Test clear_blackbox method clears data."""
        adm = adm1266_module.Adm1266(self.get_fake_adm1266_platform_spec())

        # Verify we have data initially
        initial_data = adm.read_blackbox()
        assert len(initial_data) > 0

        # Clear and verify empty
        adm.clear_blackbox()
        cleared_data = adm.read_blackbox()
        assert len(cleared_data) == 1
        assert cleared_data == b"1"

    def test_get_all_faults(self, adm1266_module):
        """Test get_all_faults method returns fault list."""
        adm = adm1266_module.Adm1266(self.get_fake_adm1266_platform_spec())

        faults = adm.get_all_faults()
        assert isinstance(faults, list)
        assert len(faults) > 0
        # Each fault should have required fields
        for fault in faults:
            assert 'fault_uid' in fault
            assert 'dpm_name' in fault

    def test_module_get_reboot_cause(self, adm1266_module):
        """Test module-level get_reboot_cause function."""
        fake_adm1266_platform_spec = self.get_fake_adm1266_platform_spec()

        adm1266_platform_spec = Mock()
        adm1266_platform_spec.Adm1266PlatformSpec = lambda name, pddf_data: fake_adm1266_platform_spec
        with patch.dict(sys.modules, {"sonic_platform.adm1266_platform_spec": adm1266_platform_spec}):
            result = adm1266_module.get_reboot_cause(self.get_test_pddf_path())
            assert result is not None

            reboot_cause, debug_msg = result
            assert reboot_cause is not None
            assert isinstance(debug_msg, str)

    def test_get_reboot_cause_type(self, adm1266_module):
        """Test get_reboot_cause_type function."""
        # Test with known reboot causes
        causes = ["REBOOT_CAUSE_POWER_LOSS", "REBOOT_CAUSE_WATCHDOG"]
        result = adm1266_module.get_reboot_cause_type(causes)
        assert result is not None

    def test_time_since(self, adm1266_module):
        """Test time_since function converts timestamp to readable format."""
        # Test with 8-byte timestamp
        timestamp = b'\x79\x2e\xee\x02\x00\x00\x00\x00'
        result = adm1266_module.time_since('timestamp', timestamp)
        assert isinstance(result, str)
        assert 'seconds after power-on' in result

    def test_channel_names(self, adm1266_module):
        """Test channel_names function formats GPIO/PDIO bits."""
        # Test GPIO formatting
        result = adm1266_module.channel_names('gpio_in', 15391)  # From test data
        assert isinstance(result, str)
        assert 'GPIO' in result or '0b' in result

    def test_decode_power_fault_cause_no_match(self, adm1266_module):
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
        hw_cause, hw_desc, summary, reboot_cause = adm1266_module.decode_power_fault_cause(
            dpm_signal_to_fault_cause, 0x0000, 0x0000)
        assert hw_cause == ""
        assert hw_desc == ""
        assert summary == ""
        assert reboot_cause == ""

    def test_decode_power_fault_cause_single_match(self, adm1266_module):
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
        hw_cause, hw_desc, summary, reboot_cause = adm1266_module.decode_power_fault_cause(
            dpm_signal_to_fault_cause, PSU_VIN_LOSS_PDIO_MASK_AND_VALUE, 0x0000)
        assert hw_cause == "TEST_FAULT"
        assert hw_desc == "Test fault description"
        assert summary == "Test summary"
        assert reboot_cause == "REBOOT_CAUSE_HARDWARE_OTHER"

    def test_decode_power_fault_cause_multiple_match(self, adm1266_module):
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
        hw_cause, hw_desc, summary, reboot_cause = adm1266_module.decode_power_fault_cause(
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
    def test_reboot_cause_str_to_type(self, adm1266_module, reboot_cause_str):
        """Test reboot_cause_str_to_type handles single and comma-separated causes."""
        reboot_cause_to_type = {
            "REBOOT_CAUSE_POWER_LOSS":
                adm1266_module.ChassisBase.REBOOT_CAUSE_POWER_LOSS,
            "REBOOT_CAUSE_POWER_LOSS,REBOOT_CAUSE_WATCHDOG":
                adm1266_module.ChassisBase.REBOOT_CAUSE_POWER_LOSS,
            "REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC, REBOOT_CAUSE_HARDWARE_OTHER":
                adm1266_module.ChassisBase.REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC,
            "INVALID_CAUSE": adm1266_module.ChassisBase.INVALID_CAUSE
        }

        reboot_cause_type = reboot_cause_to_type.get(reboot_cause_str, "")
        assert adm1266_module.reboot_cause_str_to_type(reboot_cause_str) == reboot_cause_type
