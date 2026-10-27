#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Unit tests for this platform's thermal.py.
These tests run in isolation from the SONiC environment using pytest:
python -m pytest test/unit/sonic_platform/test_thermal.py -v

The thermal policy infos, conditions and actions now come from sonic-platform-common and
are tested there, in tests/common_infos_test.py, tests/common_conditions_test.py,
tests/common_actions_test.py and tests/pid_controller_test.py.
"""

import importlib
import types
from unittest.mock import Mock, call, patch
from fixtures.test_helpers_common import mock_data_in_swsscommon

import pytest


@pytest.fixture
def thermal_module():
    """Loads the module before each test. This is to let conftest.py inject deps first."""
    from sonic_platform import thermal

    yield thermal


class TestPortIndexMapper:
    def test_get_interface_name_picks_lowest_and_ignores_invalid(self, thermal_module):
        """Verify PortIndexMapper builds mapping and picks lowest Ethernet name for same index."""
        # Given a PORT table with two Ethernet ports and a non-Ethernet entry on index 1
        mock_data_in_swsscommon(
            "CONFIG_DB",
            "PORT",
            {
                "Ethernet4": {"index": "1"},
                "Ethernet0": {"index": "1"},
                "NotAnEthernet": {"index": "1"},
            },
        )

        # When the mapper is built afresh from it (the singleton is reset to rebuild the mapping)
        thermal_module.PortIndexMapper._instance = None
        mapper = thermal_module.PortIndexMapper()

        # Then index 1 maps to the lowest Ethernet name, and an index with no port to None
        assert mapper.get_interface_name(1) == 'Ethernet0'
        assert mapper.get_interface_name(2) is None


class TestSfpThermal:
    @pytest.fixture
    def pddf_platform(self):
        # Provide minimal PLATFORM data to avoid None .lower() in PidThermalMixin
        return types.SimpleNamespace(data={'PLATFORM': {
            'nexthop_thermal_xcvr_setpoint_override': None,
            'nexthop_thermal_xcvr_pid_domain': 'none'
        }})

    def test_default_setpoint_when_thresholds_unavailable(self, thermal_module, pddf_platform):
        """When thresholds are not yet available but SFP is present, default setpoint is used."""
        # Given a present transceiver with no thresholds in STATE_DB yet
        sfp = Mock()
        sfp.get_name.return_value = 'sfp1'
        sfp.get_presence.return_value = True
        sfp.get_position_in_parent.return_value = 1

        with patch.object(thermal_module.PortIndexMapper, 'get_interface_name', return_value='Ethernet0'):
            sfp_th = thermal_module.SfpThermal(sfp, pddf_platform)
            # When its PID setpoint is read
            setpoint = sfp_th.get_pid_setpoint()
            # Then it is the default setpoint
            assert setpoint == thermal_module.SfpThermal.DEFAULT_SETPOINT

    def test_invalid_computed_setpoint_logs_once_and_uses_default(self, thermal_module, pddf_platform):
        """If computed setpoint < MIN_VALID_SETPOINT, fallback to default and log once."""
        # Given a present transceiver whose threshold gives a setpoint below MIN_VALID_SETPOINT
        mock_data_in_swsscommon(
            "STATE_DB",
            "TRANSCEIVER_DOM_THRESHOLD",
            {
                # temphighwarning - margin (10) => 25 < 30 -> invalid
                "Ethernet4": {"temphighwarning": "35"},
            },
        )

        sfp = Mock()
        sfp.get_name.return_value = 'sfp2'
        sfp.get_presence.return_value = True
        sfp.get_position_in_parent.return_value = 2

        with patch.object(thermal_module.PortIndexMapper, 'get_interface_name', return_value='Ethernet4'):
            sfp_th = thermal_module.SfpThermal(sfp, pddf_platform)
            logger = thermal_module.thermal_syslogger
            before = getattr(logger, 'log_warning').call_count

            # When its PID setpoint is read
            sp1 = sfp_th.get_pid_setpoint()
            # Then it is the default setpoint, and a warning is logged
            assert sp1 == thermal_module.SfpThermal.DEFAULT_SETPOINT
            assert getattr(logger, 'log_warning').call_count == before + 1

            # When it is read again
            # Then it is still the default, and the warning is not logged again
            sp2 = sfp_th.get_pid_setpoint()
            assert sp2 == thermal_module.SfpThermal.DEFAULT_SETPOINT
            assert getattr(logger, 'log_warning').call_count == before + 1

    @pytest.mark.parametrize("temphighwarning, falls_back", [
        pytest.param("40", False, id="at_minimum"),
        pytest.param("39.9", True, id="just_below_minimum")])
    def test_setpoint_at_minimum_is_used_and_below_it_falls_back(
            self, thermal_module, pddf_platform, temphighwarning, falls_back):
        # Given a transceiver whose high warning threshold, less the margin (10), is exactly
        # MIN_VALID_SETPOINT (30), or just below it
        mock_data_in_swsscommon(
            "STATE_DB",
            "TRANSCEIVER_DOM_THRESHOLD",
            {"Ethernet12": {"temphighwarning": temphighwarning}},
        )
        sfp = Mock()
        sfp.get_name.return_value = 'sfp4'
        sfp.get_presence.return_value = True
        sfp.get_position_in_parent.return_value = 4

        with patch.object(thermal_module.PortIndexMapper, 'get_interface_name', return_value='Ethernet12'):
            sfp_th = thermal_module.SfpThermal(sfp, pddf_platform)

            # When its PID setpoint is read
            setpoint = sfp_th.get_pid_setpoint()

        # Then a setpoint at the minimum is used as is, and one below it falls back to the default
        cls = thermal_module.SfpThermal
        assert setpoint == (cls.DEFAULT_SETPOINT if falls_back else cls.MIN_VALID_SETPOINT)

    def test_thresholds_parsing_and_cache(self, thermal_module, pddf_platform):
        """State DB threshold values are parsed to float and cached for THRESHOLDS_CACHE_INTERVAL_SEC."""
        # Given a transceiver with string thresholds in STATE_DB, plus an irrelevant field
        mock_data_in_swsscommon(
            "STATE_DB",
            "TRANSCEIVER_DOM_THRESHOLD",
            {
                "Ethernet8": {
                    "temphighwarning": "75.0",
                    "templowwarning": "10.5",
                    "temphighalarm": "90",
                    "templowalarm": "5",
                    "irrelevant": "N/A",
                },
            },
        )

        sfp = Mock()
        sfp.get_name.return_value = 'sfp3'
        sfp.get_presence.return_value = True
        sfp.get_position_in_parent.return_value = 3

        with patch.object(thermal_module.PortIndexMapper, 'get_interface_name', return_value='Ethernet8'):
            sfp_th = thermal_module.SfpThermal(sfp, pddf_platform)

            # When its thresholds are first read
            # Then they come from STATE_DB as floats, and are cached
            assert sfp_th.get_high_threshold() == 75.0
            assert sfp_th.get_low_threshold() == 10.5
            assert sfp_th.get_high_critical_threshold() == 90.0
            assert sfp_th.get_low_critical_threshold() == 5.0

            # When STATE_DB changes and they are read again within the cache interval
            mock_data_in_swsscommon(
                "STATE_DB",
                "TRANSCEIVER_DOM_THRESHOLD",
                {
                    "Ethernet8": {
                        "temphighwarning": "10",
                        "templowwarning": "1",
                        "temphighalarm": "20",
                        "templowalarm": "0",
                    },
                },
            )
            # Then the cached values are returned, unchanged
            assert sfp_th.get_high_threshold() == 75.0
            assert sfp_th.get_low_threshold() == 10.5


class TestPidHooks:
    """The PID hooks on ThermalBase default to None; the platform's own must win."""

    @pytest.mark.parametrize("module_name, class_name", [
        ("thermal", "Thermal"), ("thermal", "NexthopFpgaAsicThermal"), ("thermal", "SfpThermal"),
        ("asic_thermal", "AsicThermal")])
    def test_pid_domain_comes_from_the_mixin(self, thermal_module, module_name, class_name):
        # Given a platform thermal class, which also inherits ThermalBase's default hooks
        cls = getattr(importlib.import_module("sonic_platform." + module_name), class_name)

        # When its get_pid_domain is resolved
        # Then it is the mixin's, which reads the domain from the PDDF data
        assert cls.get_pid_domain is thermal_module.PidThermalMixin.get_pid_domain

    @pytest.mark.parametrize("module_name, class_name", [
        ("thermal", "Thermal"), ("thermal", "NexthopFpgaAsicThermal"),
        ("asic_thermal", "AsicThermal")])
    def test_pid_setpoint_comes_from_the_mixin(self, thermal_module, module_name, class_name):
        # Given a platform thermal class that does not define its own setpoint
        cls = getattr(importlib.import_module("sonic_platform." + module_name), class_name)

        # When its get_pid_setpoint is resolved
        # Then it is the mixin's, which derives the setpoint from the high threshold
        assert cls.get_pid_setpoint is thermal_module.PidThermalMixin.get_pid_setpoint

    def test_asic_thermal_reports_its_pid_domain_and_setpoint(self):
        # Given PDDF data for an ASIC sensor as the 5010 configures ASIC_TEMP1
        from sonic_platform.asic_thermal import AsicThermal
        pddf_data = types.SimpleNamespace(data={"ASIC_TEMP1": {
            "dev_info": {"nexthop_thermal_setpoint_margin": 15,
                         "nexthop_thermal_pid_domain": "asic"},
            "dev_attr": {"display_name": "ASIC D0_VTMON_CORE",
                         "temp1_high_threshold": 105,
                         "temp1_high_crit_threshold": 112}}})

        # When an ASIC thermal is built from it
        asic_th = AsicThermal(0, 0, pddf_data)

        # Then it is in the asic PID domain, with its setpoint the margin below the high threshold
        assert asic_th.get_pid_domain() == "asic"
        assert asic_th.get_pid_setpoint() == 90.0

    @pytest.mark.parametrize("domain, expected", [
        ("main", "main"), (None, None), ("None", None), ("none", None)])
    def test_sfp_thermal_reports_the_configured_domain(self, thermal_module, domain, expected):
        # Given transceiver PDDF data with a PID domain, none, or the string "None"
        platform = types.SimpleNamespace(data={'PLATFORM': {
            'nexthop_thermal_xcvr_pid_domain': domain}})
        sfp = Mock()
        sfp.get_position_in_parent.return_value = 1

        # When a transceiver thermal is built from it
        sfp_th = thermal_module.SfpThermal(sfp, platform)

        # Then its PID domain is the configured one, or None when it is not PID-controlled
        assert sfp_th.get_pid_domain() == expected
