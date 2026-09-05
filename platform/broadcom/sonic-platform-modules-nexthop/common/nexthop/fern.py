#!/usr/bin/env python3

# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
"""
Fern POWER-DELIVERY-CARD EEPROM inventory decoder.

The Fern card stores a FRU inventory record as a Nexthop "Vendor Extension"
TLV (ONIE TLV type 0xFD, NEXTHOP IANA, custom field code FRU_INVENTORY).
The payload is::

    <version:1B> <record0:4B ASCII> <record1:4B ASCII> ...

Each 4-byte record is ASCII decimal digits: inv_type, dev_type, slot,
variant_id.
"""

import logging
import logging.handlers
from dataclasses import dataclass, asdict
from sonic_eeprom import eeprom_tlvinfo
from nexthop_utils import eeprom_utils
from sonic_platform_pddf_base.pddf_platform_hooks import ChildCardEepromUnprogrammed


logger = logging.getLogger("nexthop.fern")
if not logger.handlers:
    _handler = logging.StreamHandler()
    _handler.setFormatter(logging.Formatter(
        "%(levelname)s: [%(name)s:%(funcName)s:%(lineno)d] %(message)s"
    ))
    logger.addHandler(_handler)
    _syslog = logging.handlers.SysLogHandler(address="/dev/log")
    _syslog.setFormatter(logging.Formatter(
        "fern: %(levelname)s [%(name)s:%(funcName)s:%(lineno)d] %(message)s"
    ))
    _syslog.setLevel(logging.WARNING)
    logger.addHandler(_syslog)
    logger.setLevel(logging.INFO)
    logger.propagate = False


# --- Constants -------------------------------------------------------------

INVENTORY_TYPE_FERN = 1
DEV_TYPE_IBV = 1

FERN_INVENTORY_PAYLOAD_VERSION = 0x01
FERN_INVENTORY_RECORD_SIZE = 4


@dataclass(frozen=True)
class FernIbvRecord:
    slot: int
    variant_id: int
    vendor: str
    model: str


FERN_IBV_VARIANT_MAP: dict[tuple[int, int], dict] = {
    (DEV_TYPE_IBV, 0x01): {"vendor": "DELTA", "model": "Q54SW120A7"},
    (DEV_TYPE_IBV, 0x02): {"vendor": "BELF",  "model": "TQNM0M12B"},
}


class FernEepromMalformed(Exception):
    """Raised when Fern EEPROM bytes are present but cannot be parsed."""


class RawTlvDecoder(eeprom_tlvinfo.TlvInfoDecoder):
    def __init__(self):
        super().__init__("", 0, "", True)

    def decoder(self, s, t):
        return ("", bytes(t[2:2 + t[1]]))


class VendorExtCollector(eeprom_tlvinfo.EepromDefaultVisitor):
    def __init__(self):
        self.values = []
        self.error = None

    def visit_tlv(self, name, code, length, value):
        if code == eeprom_tlvinfo.TlvInfoDecoder._TLV_CODE_VENDOR_EXT:
            self.values.append(value)

    def set_error(self, error):
        self.error = error


# --- ONIE TLV walking ------------------------------------------------------


def _iter_fru_inventory_payloads(blob):
    """Yield the FRU-inventory payload bytes of every Nexthop Vendor
    Extension TLV in ``blob`` whose IANA matches NEXTHOP and whose custom
    field code is ``CustomField.FRU_INVENTORY``.
    """
    visitor = VendorExtCollector()
    RawTlvDecoder().visit_eeprom(bytearray(blob), visitor)
    if visitor.error:
        logger.warning("Fern EEPROM walk: %s", visitor.error)

    nexthop_iana = int(eeprom_utils.NEXTHOP_IANA)
    fru_code = eeprom_utils.CustomField.FRU_INVENTORY.code
    vendor_ext_code = eeprom_tlvinfo.TlvInfoDecoder._TLV_CODE_VENDOR_EXT
    for value in visitor.values:
        # tlv_to_custom_field_struct expects the full TLV bytes
        # (type + len + value). Reconstruct from the value bytes the
        # visitor handed us.
        tlv = bytearray([vendor_ext_code, len(value)]) + bytearray(value)
        cfs, _err = eeprom_utils.tlv_to_custom_field_struct(tlv)
        if cfs is None:
            continue
        if eeprom_utils.big_endian_to_int(cfs.iana) != nexthop_iana:
            continue
        if cfs.code != fru_code:
            continue
        yield bytes(cfs.payload)


def decode_fern_inventory_blob(blob) -> list[FernIbvRecord]:
    """Decode a Fern EEPROM blob into a slot-sorted list of FernIbvRecord."""
    if not isinstance(blob, (bytes, bytearray)):
        raise TypeError("blob must be bytes or bytearray")
    blob = bytearray(blob)

    ibvs: list[FernIbvRecord] = []
    saw_any_tlv = False
    for payload in _iter_fru_inventory_payloads(blob):
        saw_any_tlv = True
        if len(payload) < 1:
            raise FernEepromMalformed("FRU inventory payload is empty")
        version = payload[0]
        if version != FERN_INVENTORY_PAYLOAD_VERSION:
            raise FernEepromMalformed(
                f"unsupported FRU inventory payload version 0x{version:02x}"
            )
        records = payload[1:]
        if len(records) % FERN_INVENTORY_RECORD_SIZE != 0:
            raise FernEepromMalformed(
                f"FRU inventory record body length {len(records)} is not a "
                f"multiple of {FERN_INVENTORY_RECORD_SIZE}"
            )
        for i in range(0, len(records), FERN_INVENTORY_RECORD_SIZE):
            rec = records[i:i + FERN_INVENTORY_RECORD_SIZE]
            try:
                inv_type, dev_type, slot, variant_id = (int(chr(b)) for b in rec)
            except ValueError as exc:
                raise FernEepromMalformed(
                    f"FRU record at offset {i} has non-ASCII-digit byte: "
                    f"{list(rec)!r}"
                ) from exc

            if inv_type != INVENTORY_TYPE_FERN:
                logger.debug(
                    "skipping FRU record with inventory_type=%d (not Fern)",
                    inv_type,
                )
                continue
            if dev_type != DEV_TYPE_IBV:
                logger.debug(
                    "skipping non-IBV FRU record dev_type=%d slot=%d variant_id=%d",
                    dev_type, slot, variant_id,
                )
                continue
            meta = FERN_IBV_VARIANT_MAP.get((dev_type, variant_id))
            if meta is None:
                logger.warning(
                    "unknown Fern IBV variant (dev_type=%d, variant_id=%d) at "
                    "slot %d -- skipping",
                    dev_type, variant_id, slot,
                )
                continue
            ibvs.append(FernIbvRecord(
                slot=slot,
                variant_id=variant_id,
                vendor=meta["vendor"],
                model=meta["model"],
            ))
            logger.info(
                "slot %d -> variant_id=0x%02x vendor=%s model=%s",
                slot, variant_id, meta["vendor"], meta["model"],
            )

    if not saw_any_tlv:
        logger.warning(
            "no Nexthop Vendor Extension TLV with code 0x%02x (%s) found "
            "in Fern EEPROM",
            eeprom_utils.CustomField.FRU_INVENTORY.code,
            eeprom_utils.CustomField.FRU_INVENTORY.display_name,
        )
    ibvs.sort(key=lambda r: r.slot)
    return ibvs


# --- Public entry point (PddfPlatformHooks contract) -----------------------


def decode(eeprom_bytes: bytes, slot: int) -> dict:
    """Return the identity record for the IBV at ``slot``.

    Called by :class:`sonic_platform.pddf_hooks.PlatformHooks.decode_eeprom`
    (which is called by :func:`pddfparse.PddfParse.expand_child_cards`) once
    per FERN CHILD_CARDS entry, with ``slot`` distinguishing which of the
    three IBV records to return.
    """
    records = decode_fern_inventory_blob(eeprom_bytes)
    if not records:
        raise ChildCardEepromUnprogrammed(
            f"Fern EEPROM has no FRU inventory records "
            f"(unprogrammed/blank); cannot resolve slot {slot}"
        )
    for r in records:
        if r.slot == slot:
            return asdict(r)
    raise KeyError(
        f"Fern EEPROM contains no IBV record for slot {slot} "
        f"(decoded slots: {[r.slot for r in records]})"
    )
