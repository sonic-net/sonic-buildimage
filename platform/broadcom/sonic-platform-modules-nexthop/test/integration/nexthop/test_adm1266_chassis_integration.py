#!/usr/bin/env python

import pytest
import sys

from fixtures.fake_swsscommon import fake_swsscommon_modules
from fixtures.mock_imports_unit_tests import mock_syslog_modules
from fixtures.test_helpers_adm1266 import Adm1266TestMixin
from fixtures.test_helpers_chassis import setup_patch_for_chassis_init
from unittest.mock import Mock, patch


@pytest.fixture(scope="module")
def mock_unimportant_modules():
    """Mock modules that aren't important for integration testing."""
    modules = {}
    modules["sonic_py_common"] = Mock()
    modules["sonic_platform.dpm"] = Mock()
    modules["sonic_platform.dpm"].SystemDPMLogHistory = Mock()
    modules.update(mock_syslog_modules())
    modules.update(fake_swsscommon_modules())

    with patch.dict(sys.modules, modules):
        yield


@pytest.fixture(scope="module")
def chassis_module(mock_unimportant_modules):
    """Loads the module before all tests. This is to let conftest.py inject deps first."""
    from sonic_platform import chassis

    yield chassis


class TestAdm1266ChassisIntegration(Adm1266TestMixin):
    """Integration tests for ADM1266 with chassis - reboot cause only."""

    def test_chassis_get_reboot_cause(self, chassis_module):
        # Given
        with setup_patch_for_chassis_init(self.get_pddf_plugin_data()):
            chassis = chassis_module.Chassis()

            # When
            reboot_cause = chassis.get_reboot_cause()

            # Then
            assert reboot_cause[0] == "Hardware - Other"
            assert reboot_cause[1] == ""
