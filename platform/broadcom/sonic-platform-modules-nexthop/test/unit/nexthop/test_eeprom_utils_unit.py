#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Unit tests for the nexthop eeprom_utils module.
These tests run in isolation from the SONiC environment using pytest:
python -m pytest test/unit/nexthop/test_eeprom_utils.py -v
"""

import os
import sys
import tempfile
from typing import Counter
import pytest

# Add the test directory to Python path for imports
test_root = os.path.join(os.path.dirname(__file__), '../..')
sys.path.insert(0, test_root)

# Import shared test helpers
from fixtures.test_helpers_eeprom import EepromTestMixin


class TestEepromUtils(EepromTestMixin):
    """Test class for EEPROM utilities functionality."""

    def test_get_find_at24_eeprom_paths(self, nexthop_eeprom_utils):
        """Test finding AT24 EEPROM paths."""
        # Given
        root = tempfile.mktemp()
        self.setup_test_i2c_environment(root)

        # When
        eeprom_paths = nexthop_eeprom_utils.get_at24_eeprom_paths(root)

        # Then
        expected_paths = self.get_expected_eeprom_paths(root)
        assert Counter(eeprom_paths) == Counter(expected_paths)

    # NOTE: Full EEPROM programming and decoding tests have been moved to
    # test/integration/nexthop/test_eeprom_utils.py
    # These integration tests require the full SONiC environment with
    # sonic-platform-common and sonic_eeprom.eeprom_tlvinfo modules available.
