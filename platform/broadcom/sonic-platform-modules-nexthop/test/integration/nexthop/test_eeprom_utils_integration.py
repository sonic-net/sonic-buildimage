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
    Eeprom,
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

    def test_decode_known_buggy_custom_serial_number(self, capsys):
        """
        Under full SONiC environment,
        Test decoding and reprogramming EEPROM data when "Custom Serial Number"
        TLV has a known bug, where byte 2 of the TLV contains a garbage value.
        """
        # Given
        root = tempfile.mktemp()
        os.makedirs(root)
        eeprom_path = os.path.join(root, "eeprom")
        self.create_fake_eeprom(eeprom_path)

        # When
        program_data = self.get_standard_eeprom_program_data()
        for k in program_data:
            program_data[k] = None
        program_data["custom_serial_number"] = "123"
        program_eeprom(eeprom_path=eeprom_path, **program_data)

        # Then
        expected = """\
TlvInfo Header:
   Id String:    TlvInfo
   Version:      1
   Total Length: 16
TLV Name                  Code Len Value
------------------------- ---- --- -----
Custom Serial Number      0xFD   8 123
CRC-32                    0xFE   4 0x8F92A23C
"""
        decode_eeprom(eeprom_path)
        out, _ = capsys.readouterr()
        assert expected in out

        # But When
        # Byte 2 of the "Custom Serial Number" TLV contains a garbage value
        with open(eeprom_path, "rb") as f:
            e = bytearray(f.read())
        csn_tlv_start = Eeprom._TLV_INFO_HDR_LEN
        e = e[:csn_tlv_start + 2] + bytearray([0xff]) + e[csn_tlv_start + 2:]
        # Increment payload length by 1
        e[csn_tlv_start + 1] += 1
        # Increment total EEPROM data length by 1
        total_length = e[9] << 8 | e[10]
        total_length += 1
        e[9] = (total_length >> 8) & 0xFF
        e[10] = total_length & 0xFF
        # Write the modified EEPROM data back to file
        with open(eeprom_path, "wb") as f:
            f.write(e)

        # Then can still decode
        expected = """\
TlvInfo Header:
   Id String:    TlvInfo
   Version:      1
   Total Length: 17
TLV Name                  Code Len Value
------------------------- ---- --- -----
Custom Serial Number      0xFD   9 123
CRC-32                    0xFE   4 0x8F92A23C
"""
        decode_eeprom(eeprom_path)
        out, _ = capsys.readouterr()
        assert expected in out

        # And When programming EEPROM again
        program_eeprom(eeprom_path=eeprom_path, **program_data)

        # Then the good format is restored
        expected = """\
TlvInfo Header:
   Id String:    TlvInfo
   Version:      1
   Total Length: 16
TLV Name                  Code Len Value
------------------------- ---- --- -----
Custom Serial Number      0xFD   8 123
CRC-32                    0xFE   4 0x8F92A23C
"""
        decode_eeprom(eeprom_path)
        out, _ = capsys.readouterr()
        assert expected in out

    def test_decode_buggy_regulatory_model_number(self, capsys):
        """
        Under full SONiC environment,
        Test decoding EEPROM data gives invalid output when the known bug
        (byte 2 of the TLV contains a garbage value) is applied on other
        Nexthop custom fields, e.g. "Regulatory Model Number".
        """
        # Given
        root = tempfile.mktemp()
        os.makedirs(root)
        eeprom_path = os.path.join(root, "eeprom")
        self.create_fake_eeprom(eeprom_path)

        # When
        program_data = self.get_standard_eeprom_program_data()
        for k in program_data:
            program_data[k] = None
        program_data["regulatory_model_number"] = "123"
        program_eeprom(eeprom_path=eeprom_path, **program_data)

        # Then
        expected = """\
TlvInfo Header:
   Id String:    TlvInfo
   Version:      1
   Total Length: 16
TLV Name                  Code Len Value
------------------------- ---- --- -----
Regulatory Model Number   0xFD   8 123
CRC-32                    0xFE   4 0x0906D092
"""
        decode_eeprom(eeprom_path)
        out, _ = capsys.readouterr()
        assert expected in out

        # But When
        # Byte 2 of the "Regulatory Model Number" TLV contains a garbage value
        with open(eeprom_path, "rb") as f:
            e = bytearray(f.read())
        csn_tlv_start = Eeprom._TLV_INFO_HDR_LEN
        e = e[:csn_tlv_start + 2] + bytearray([0xff]) + e[csn_tlv_start + 2:]
        # Increment payload length by 1
        e[csn_tlv_start + 1] += 1
        # Increment total EEPROM data length by 1
        total_length = e[9] << 8 | e[10]
        total_length += 1
        e[9] = (total_length >> 8) & 0xFF
        e[10] = total_length & 0xFF
        # Write the modified EEPROM data back to file
        with open(eeprom_path, "wb") as f:
            f.write(e)

        # Then
        expected = """\
TlvInfo Header:
   Id String:    TlvInfo
   Version:      1
   Total Length: 17
TLV Name                  Code Len Value
------------------------- ---- --- -----
Vendor Extension          0xFD   9 Invalid IANA: 4278190326, expected 63074
CRC-32                    0xFE   4 0x0906D092
"""
        decode_eeprom(eeprom_path)
        out, _ = capsys.readouterr()
        assert expected in out

    def test_program_replace_nh_custom_fields(self, capsys):
        """
        Under full SONiC environment,
        Test re-programming EEPROM data with Nexthop custom fields being replaced.
        """
        # Given
        root = tempfile.mktemp()
        os.makedirs(root)
        eeprom_path = os.path.join(root, "eeprom")
        self.create_fake_eeprom(eeprom_path)

        # When
        program_data = self.get_standard_eeprom_program_data()
        for k in program_data:
            program_data[k] = None
        program_data["product_name"] = "NH-9999"
        program_data["custom_serial_number"] = "111"
        program_data["regulatory_model_number"] = "AAA"
        program_eeprom(eeprom_path=eeprom_path, **program_data)

        expected = self.get_expected_tlv_output()

        # Then
        expected = """\
TlvInfo Header:
   Id String:    TlvInfo
   Version:      1
   Total Length: 35
TLV Name                  Code Len Value
------------------------- ---- --- -----
Product Name              0x21   7 NH-9999
Custom Serial Number      0xFD   8 111
Regulatory Model Number   0xFD   8 AAA
CRC-32                    0xFE   4 0xB6CE81FB
"""
        decode_eeprom(eeprom_path)
        out, _ = capsys.readouterr()
        assert expected in out

        # And When programming EEPROM again with different values
        program_data["custom_serial_number"] = "222"
        program_data["regulatory_model_number"] = "BBB"
        program_eeprom(eeprom_path=eeprom_path, **program_data)

        # Then the values are replaced
        expected = """\
TlvInfo Header:
   Id String:    TlvInfo
   Version:      1
   Total Length: 35
TLV Name                  Code Len Value
------------------------- ---- --- -----
Product Name              0x21   7 NH-9999
Custom Serial Number      0xFD   8 222
Regulatory Model Number   0xFD   8 BBB
CRC-32                    0xFE   4 0x314BC9F0
"""
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

