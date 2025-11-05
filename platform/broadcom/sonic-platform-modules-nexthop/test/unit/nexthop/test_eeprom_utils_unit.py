#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Unit tests for the nexthop eeprom_utils module.
These tests run in isolation from the SONiC environment using pytest:
python -m pytest test/unit/nexthop/test_eeprom_utils.py -v
"""

import tempfile
from typing import Counter
import pytest

# Import shared test helpers
from fixtures.test_helpers_eeprom import EepromTestMixin

@pytest.fixture(scope="function", autouse=True)
def eeprom_utils_module():
    """Loads the module before each test. This is to let conftest.py inject deps first."""
    from nexthop import eeprom_utils
    return eeprom_utils


class TestEepromUtils(EepromTestMixin):
    """Test class for EEPROM utilities functionality."""

    def test_get_find_at24_eeprom_paths(self, eeprom_utils_module):
        """Test finding AT24 EEPROM paths."""
        # Given
        root = tempfile.mktemp()
        self.setup_test_i2c_environment(root)

        # When
        eeprom_paths = eeprom_utils_module.get_at24_eeprom_paths(root)

        # Then
        expected_paths = self.get_expected_eeprom_paths(root)
        assert Counter(eeprom_paths) == Counter(expected_paths)

    # NOTE: Full EEPROM programming and decoding tests have been moved to
    # test/integration/nexthop/test_eeprom_utils.py
    # These integration tests require the full SONiC environment with
    # sonic-platform-common and sonic_eeprom.eeprom_tlvinfo modules available.
