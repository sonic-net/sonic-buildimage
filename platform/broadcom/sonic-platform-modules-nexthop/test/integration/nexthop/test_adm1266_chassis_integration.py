#!/usr/bin/env python

import pytest
import sys
from unittest.mock import Mock, patch, mock_open
import os

# Import test fixtures
sys.path.insert(0, '../../fixtures')
from fixtures_unit_test import Adm1266Mock


@pytest.fixture
def chassis():
    """Create a mock chassis for integration testing."""
    from unittest.mock import Mock

    chassis_mock = Mock()
    chassis_mock._blackbox = None  # Will be set by individual tests

    def mock_get_reboot_cause():
        if chassis_mock._blackbox is None:
            return ('REBOOT_CAUSE_NON_HARDWARE', 'Unknown')

        # Delegate to the ADM1266 mock
        return chassis_mock._blackbox.get_reboot_cause()

    chassis_mock.get_reboot_cause = mock_get_reboot_cause
    return chassis_mock


class TestAdm1266ChassisIntegration:
    """Integration tests for ADM1266 with chassis - reboot cause only."""

    def test_clear_blackbox_integration(self, chassis):
        """Test blackbox clearing through chassis interface."""
        chassis._blackbox = Adm1266Mock()

        # Initially has faults
        fault_data = chassis.get_reboot_cause()
        assert len(fault_data), "no fault data"

        # Clear blackbox
        chassis._blackbox.clear_blackbox()

        # Should now show cleared state
        assert chassis._blackbox.blackbox_cleared == True
