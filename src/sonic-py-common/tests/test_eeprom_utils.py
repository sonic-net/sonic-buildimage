"""
Unit tests for sonic_py_common.eeprom_utils.safe_decode_eeprom_text.
"""

import pytest

from sonic_py_common.eeprom_utils import safe_decode_eeprom_text


class TestSafeDecodeEepromText:
    """Tests for safe_decode_eeprom_text()."""

    # ------------------------------------------------------------------
    # Basic str/None/invalid input passthrough
    # ------------------------------------------------------------------

    def test_none_returns_default(self):
        assert safe_decode_eeprom_text(None) == "NA"

    def test_none_custom_default(self):
        assert safe_decode_eeprom_text(None, default="Unknown") == "Unknown"

    def test_invalid_type_returns_default(self):
        assert safe_decode_eeprom_text(12345) == "NA"
        assert safe_decode_eeprom_text([0x41, 0x42]) == "NA"

    def test_str_passthrough(self):
        assert safe_decode_eeprom_text("DPS-200PB") == "DPS-200PB"

    def test_str_strips_padding(self):
        assert safe_decode_eeprom_text("\x00\x00DPS-200PB \x00") == "DPS-200PB"

    def test_str_all_padding_returns_default(self):
        assert safe_decode_eeprom_text("\x00\xff  \x00") == "NA"

    # ------------------------------------------------------------------
    # bytes / bytearray – clean ASCII
    # ------------------------------------------------------------------

    def test_clean_ascii_bytes(self):
        assert safe_decode_eeprom_text(b"DPS-200PB-204") == "DPS-200PB-204"

    def test_clean_ascii_bytearray(self):
        assert safe_decode_eeprom_text(bytearray(b"DPS-200PB-204")) == "DPS-200PB-204"

    def test_leading_trailing_spaces_stripped(self):
        assert safe_decode_eeprom_text(b"  DPS-200PB  ") == "DPS-200PB"

    # ------------------------------------------------------------------
    # Padding bytes stripped
    # ------------------------------------------------------------------

    def test_null_padded_bytes(self):
        assert safe_decode_eeprom_text(b"DPS-200PB\x00\x00\x00") == "DPS-200PB"

    def test_ff_padded_bytes(self):
        """Simulate the observed crash: b'DPS-200PB-204 \\xff'"""
        assert safe_decode_eeprom_text(b"DPS-200PB-204 \xff") == "DPS-200PB-204"

    def test_all_null_bytes_returns_default(self):
        assert safe_decode_eeprom_text(b"\x00\x00\x00") == "NA"

    def test_all_ff_bytes_returns_default(self):
        assert safe_decode_eeprom_text(b"\xff\xff\xff") == "NA"

    def test_empty_bytes_returns_default(self):
        assert safe_decode_eeprom_text(b"") == "NA"

    # ------------------------------------------------------------------
    # Non-UTF-8 interior bytes (latin-1 fallback)
    # ------------------------------------------------------------------

    def test_latin1_interior_bytes(self):
        """Byte 0xE9 is valid latin-1 ('é') but cannot appear in UTF-8 alone."""
        result = safe_decode_eeprom_text(b"Caf\xe9")
        assert result == "Café"

    def test_mixed_padding_and_latin1(self):
        result = safe_decode_eeprom_text(b"\x00Caf\xe9\xff")
        assert result == "Café"

    # ------------------------------------------------------------------
    # Custom default value
    # ------------------------------------------------------------------

    def test_custom_default_on_empty(self):
        assert safe_decode_eeprom_text(b"", default="N/A") == "N/A"

    def test_custom_default_on_all_padding(self):
        assert safe_decode_eeprom_text(b"\xff\xff", default="Unknown") == "Unknown"
