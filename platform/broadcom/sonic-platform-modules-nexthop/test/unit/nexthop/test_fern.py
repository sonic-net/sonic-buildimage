#!/usr/bin/env python

# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the Fern EEPROM inventory decoder"""

import sys
import types
import pytest


@pytest.fixture(scope="function")
def fern(monkeypatch):
    """Inject concrete stubs for fern's imports and import it fresh."""
    # sonic_eeprom.eeprom_tlvinfo -- concrete base classes so fern's
    # RawTlvDecoder / VendorExtCollector subclasses are real classes.
    tlv = types.ModuleType("sonic_eeprom.eeprom_tlvinfo")

    class TlvInfoDecoder:
        _TLV_CODE_VENDOR_EXT = 0xFD

        def __init__(self, *a, **k):
            pass

        def visit_eeprom(self, *a, **k):
            pass

    class EepromDefaultVisitor:
        def __init__(self, *a, **k):
            pass

    tlv.TlvInfoDecoder = TlvInfoDecoder
    tlv.EepromDefaultVisitor = EepromDefaultVisitor
    se = types.ModuleType("sonic_eeprom")
    se.eeprom_tlvinfo = tlv
    monkeypatch.setitem(sys.modules, "sonic_eeprom", se)
    monkeypatch.setitem(sys.modules, "sonic_eeprom.eeprom_tlvinfo", tlv)

    # nexthop_utils.eeprom_utils -- only the FRU_INVENTORY code/display_name
    # are read on the no-TLV path; the rest is used inside the (monkeypatched)
    # TLV walk.
    eu = types.ModuleType("nexthop_utils.eeprom_utils")

    class _Fru:
        code = 0x04
        display_name = "FRU Inventory"

    class CustomField:
        FRU_INVENTORY = _Fru()

    eu.NEXTHOP_IANA = "63074"
    eu.CustomField = CustomField
    eu.tlv_to_custom_field_struct = lambda t: (None, "")
    eu.big_endian_to_int = lambda b: 0
    nu = sys.modules.get("nexthop_utils") or types.ModuleType("nexthop_utils")
    nu.eeprom_utils = eu
    monkeypatch.setitem(sys.modules, "nexthop_utils", nu)
    monkeypatch.setitem(sys.modules, "nexthop_utils.eeprom_utils", eu)

    # pddf_platform_hooks -- fern raises ChildCardEepromUnprogrammed, so it
    # must be a real exception class (the conftest mock is a MagicMock).
    hooks = types.ModuleType("sonic_platform_pddf_base.pddf_platform_hooks")

    class ChildCardEepromUnprogrammed(Exception):
        pass

    hooks.ChildCardEepromUnprogrammed = ChildCardEepromUnprogrammed
    monkeypatch.setitem(
        sys.modules, "sonic_platform_pddf_base.pddf_platform_hooks", hooks
    )

    monkeypatch.delitem(sys.modules, "nexthop.fern", raising=False)
    from nexthop import fern

    yield fern


def _payload(version, *records):
    """Build a FRU-inventory payload from a version byte and 4-char records."""
    body = "".join(records).encode("ascii")
    return bytes([version]) + body


def _feed(monkeypatch, fern, *payloads):
    """Make _iter_fru_inventory_payloads yield the given payloads."""
    monkeypatch.setattr(
        fern, "_iter_fru_inventory_payloads", lambda blob: iter(list(payloads))
    )


# --- decode_fern_inventory_blob -------------------------------------------


def test_valid_single_record(fern, monkeypatch):
    # inv_type=1 (Fern), dev_type=1 (IBV), slot=1, variant_id=1 -> DELTA.
    _feed(monkeypatch, fern, _payload(0x01, "1111"))
    recs = fern.decode_fern_inventory_blob(b"")
    assert len(recs) == 1
    assert (recs[0].slot, recs[0].vendor, recs[0].model) == (1, "DELTA", "Q54SW120A7")


def test_records_sorted_by_slot(fern, monkeypatch):
    # slot 2 (BELF) then slot 1 (DELTA) -> returned sorted by slot.
    _feed(monkeypatch, fern, _payload(0x01, "1122", "1111"))
    recs = fern.decode_fern_inventory_blob(b"")
    assert [r.slot for r in recs] == [1, 2]
    assert [r.vendor for r in recs] == ["DELTA", "BELF"]


def test_unknown_variant_skipped(fern, monkeypatch):
    # dev_type=1, variant_id=3 -> not in FERN_IBV_VARIANT_MAP -> skipped.
    _feed(monkeypatch, fern, _payload(0x01, "1113"))
    assert fern.decode_fern_inventory_blob(b"") == []


def test_non_fern_inventory_type_skipped(fern, monkeypatch):
    # inv_type=2 -> not Fern -> skipped.
    _feed(monkeypatch, fern, _payload(0x01, "2111"))
    assert fern.decode_fern_inventory_blob(b"") == []


def test_non_ibv_dev_type_skipped(fern, monkeypatch):
    # dev_type=2 -> not IBV -> skipped.
    _feed(monkeypatch, fern, _payload(0x01, "1211"))
    assert fern.decode_fern_inventory_blob(b"") == []


def test_bad_version_raises(fern, monkeypatch):
    _feed(monkeypatch, fern, _payload(0x02, "1111"))
    with pytest.raises(fern.FernEepromMalformed, match="version"):
        fern.decode_fern_inventory_blob(b"")


def test_record_body_not_multiple_of_four_raises(fern, monkeypatch):
    _feed(monkeypatch, fern, _payload(0x01, "111"))  # 3 bytes, not a multiple of 4
    with pytest.raises(fern.FernEepromMalformed, match="multiple of"):
        fern.decode_fern_inventory_blob(b"")


def test_non_ascii_digit_byte_raises_malformed(fern, monkeypatch):
    _feed(monkeypatch, fern, _payload(0x01, "11a1"))  # 'a' is not a decimal digit
    with pytest.raises(fern.FernEepromMalformed, match="non-ASCII-digit"):
        fern.decode_fern_inventory_blob(b"")


def test_multiple_payloads_all_processed(fern, monkeypatch):
    _feed(monkeypatch, fern, _payload(0x01, "1111"), _payload(0x01, "1122"))
    recs = fern.decode_fern_inventory_blob(b"")
    assert {r.slot for r in recs} == {1, 2}


def test_no_tlv_returns_empty(fern, monkeypatch):
    _feed(monkeypatch, fern)  # no payloads
    assert fern.decode_fern_inventory_blob(b"") == []


def test_non_bytes_blob_raises_type_error(fern):
    with pytest.raises(TypeError):
        fern.decode_fern_inventory_blob("not-bytes")


# --- decode() entry point --------------------------------------------------


def test_decode_returns_requested_slot(fern, monkeypatch):
    _feed(monkeypatch, fern, _payload(0x01, "1111", "1122"))
    rec = fern.decode(b"", slot=2)
    assert rec["slot"] == 2 and rec["vendor"] == "BELF"


def test_decode_unprogrammed_raises_child_card(fern, monkeypatch):
    from sonic_platform_pddf_base.pddf_platform_hooks import ChildCardEepromUnprogrammed
    _feed(monkeypatch, fern)  # blank / no records
    with pytest.raises(ChildCardEepromUnprogrammed):
        fern.decode(b"", slot=1)


def test_decode_missing_slot_raises_key_error(fern, monkeypatch):
    _feed(monkeypatch, fern, _payload(0x01, "1111"))  # only slot 1 present
    with pytest.raises(KeyError):
        fern.decode(b"", slot=9)
