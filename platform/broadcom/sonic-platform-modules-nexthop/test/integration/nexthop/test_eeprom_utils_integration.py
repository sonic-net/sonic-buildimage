#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Integration tests for the nexthop eeprom_utils module.

These tests require the full SONiC environment with sonic-platform-common
and sonic_eeprom.eeprom_tlvinfo modules available. They test the complete
EEPROM programming and decoding functionality.

Usage:
    # Run in SONiC environment with sonic-platform-common available
    python -m pytest test/integration/nexthop/test_eeprom_utils.py -v
"""

import os
import sys
import tempfile
from typing import Counter
import pytest

# Add the test directory to Python path for imports
test_root = os.path.join(os.path.dirname(__file__), '../..')
sys.path.insert(0, test_root)

# Add the common directory to path for nexthop modules
common_path = os.path.join(test_root, '../common')
sys.path.insert(0, common_path)

# Import shared test helpers
from fixtures.test_helpers_eeprom import EepromTestMixin, EEPROM_SIZE

CWD = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(CWD, "../../../../../../src/sonic-platform-common/"))
from nexthop.eeprom_utils import (
    clear_eeprom,
    decode_eeprom,
    get_at24_eeprom_paths,
    program_eeprom,
)


class TestEepromUtilsIntegration(EepromTestMixin):
    """Integration test class for EEPROM utilities with full SONiC environment."""

    def test_program_and_decode(self, capsys):
        """Test programming and decoding EEPROM data with full SONiC environment."""
        # Given
        root = tempfile.mktemp()
        os.makedirs(root)
        eeprom_path = os.path.join(root, "eeprom")
        self.create_fake_eeprom(eeprom_path)

        # When
        program_data = self.get_standard_eeprom_program_data()
        program_eeprom(eeprom_path=eeprom_path, **program_data)

        expected = self.get_expected_tlv_output()

        # Then
        decode_eeprom(eeprom_path)
        out, _ = capsys.readouterr()
        assert expected in out

    def test_clear(self, capsys):
        """Test clearing EEPROM data with full SONiC environment."""
        # Given
        root = tempfile.mktemp()
        os.makedirs(root)
        eeprom_path = os.path.join(root, "eeprom")
        self.create_fake_eeprom(eeprom_path)
        program_data = self.get_standard_eeprom_program_data()
        program_eeprom(eeprom_path=eeprom_path, **program_data)

        # When
        clear_eeprom(eeprom_path)

        # Then
        decode_eeprom(eeprom_path)
        out, _ = capsys.readouterr()
        assert "EEPROM does not contain data in a valid TlvInfo format" in out

