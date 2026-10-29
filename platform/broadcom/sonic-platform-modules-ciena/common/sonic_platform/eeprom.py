#!/usr/bin/env python

#############################################################################
# Ciena system (MFG) EEPROM.
#
# Content is Ciena ASCII key=value pairs (EA/ER/MS/MP/MR/MD/CC/BC/...),
# not ONIE TLV. Provides the read/decode/checksum helpers plus the
# read_eeprom()/update_eeprom_db() hooks that decode-syseeprom and
# syseepromd expect.
#############################################################################

import logging
import os
import zlib

try:
    from sonic_platform_base.device_base import DeviceBase
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

logger = logging.getLogger(__name__)

# All known Ciena MFG EEPROM keys
CIENA_EEPROM_KEYS = [
    "EA", "ER", "MS", "MP", "MR", "SW", "MD", "CC", "BC", "PI",
    "BS1", "BP1", "BR1", "BS2", "BP2", "BR2", "CS",
]

EEPROM_READ_BYTES = 2048

_CIENA_TO_TLV_CODE = {
    "PI":  0x21,   # Product Name  (Platform Identifier → Product Name)
    "MP":  0x22,   # Part Number
    "MS":  0x23,   # Serial Number (Manufacturing Serial)
    "EA":  0x24,   # Base MAC Address
    "MD":  0x25,   # Manufacture Date
    "MR":  0x27,   # Label Revision
    "ER":  0x2A,   # MAC Addresses
    "CC":  0x2C,   # Manufacture Country
}

# Reverse: TLV code → display name (used by decode-syseeprom)
_TLV_CODE_NAMES = {
    0x21: "Product Name",
    0x22: "Part Number",
    0x23: "Serial Number",
    0x24: "Base MAC Address",
    0x25: "Manufacture Date",
    0x26: "Device Version",
    0x27: "Label Revision",
    0x28: "Platform Name",
    0x29: "ONIE Version",
    0x2A: "MAC Addresses",
    0x2B: "Manufacturer",
    0x2C: "Manufacture Country",
    0x2D: "Vendor Name",
    0x2E: "Diag Version",
    0x2F: "Service Tag",
}


def _get_pddf_json(pddf_obj):
    """Return parsed PDDF JSON dict from PDDF object (or dict input)."""
    if pddf_obj is None:
        return {}
    pddf_json = getattr(pddf_obj, "data", pddf_obj)
    return pddf_json if isinstance(pddf_json, dict) else {}


def _get_eeprom_dev_attr(pddf_obj, attr_name):
    """Get EEPROM dev_attr value from PDDF JSON."""
    pddf_json = _get_pddf_json(pddf_obj)
    dev_attr = pddf_json.get("EEPROM", {}).get("dev_attr", {})
    if isinstance(dev_attr, dict):
        return dev_attr.get(attr_name)
    return None


def _get_platform_identifier(pddf_obj):
    """Get platform identifier from PDDF JSON."""
    pddf_json = _get_pddf_json(pddf_obj)
    return pddf_json.get("PLATFORM", {}).get("identifier")

class Eeprom(DeviceBase):
    """Ciena Platform-specific EEPROM class"""

    def __init__(self, pddf_data=None, pddf_plugin_data=None):
        self.pddf_obj = pddf_data
        self._eeprom_path = _get_eeprom_dev_attr(self.pddf_obj, "EEPROM_PATH")
        self._eeprom_cache_file = (
            _get_eeprom_dev_attr(self.pddf_obj, "EEPROM_CACHE_FILE")
        )
        self._platform_identifier = _get_platform_identifier(self.pddf_obj)
        DeviceBase.__init__(self)
        # Defer the expensive I2C read until data is actually needed.
        # _ensure_loaded() will call _load_eeprom() on first access.
        self._eeprom = None

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------

    def _load_eeprom(self):
        """Read the EEPROM and parse key=value pairs into a dict.

        Uses the /tmp cache when present, else reads HW and caches it.
        """
        raw = None

        # Fast path: read from /tmp cache
        if self._eeprom_cache_file is None:
            logger.error("EEPROM cache file path not found in PDDF JSON")
            return {}
        try:
            if os.path.exists(self._eeprom_cache_file):
                with open(self._eeprom_cache_file, 'rb') as f:
                    raw = f.read()
        except Exception:
            raw = None

        # Slow path: read from hardware and populate cache
        if self._eeprom_path is None:
            logger.error("EEPROM path not found in PDDF JSON")
            return {}
        if raw is None:
            try:
                # Read only a bounded number of bytes from EEPROM.
                with open(self._eeprom_path, 'rb') as f:
                    raw = f.read(EEPROM_READ_BYTES)
                # Persist to /tmp for next CLI invocation
                try:
                    tmp = self._eeprom_cache_file + ".tmp"
                    with open(tmp, 'wb') as f:
                        f.write(raw)
                    os.replace(tmp, self._eeprom_cache_file)
                except Exception as e:
                    logger.debug("Failed to cache EEPROM: %s", e)
            except Exception as e:
                logger.error("EEPROM read error: %s", e)
                return {}

        # Parse ASCII key=value pairs. CS= is the record terminator, so
        # stop after it to avoid picking up stray bytes past the record.
        eeprom_data = {}
        text = raw.decode('ascii', errors='ignore')
        for line in text.split('\n'):
            line = line.strip('\0').strip()
            if not line or '=' not in line:
                continue
            key, value = line.split('=', 1)
            key = key.strip()
            eeprom_data[key] = value.strip()
            if key == "CS":
                break
        return eeprom_data

    def _ensure_loaded(self):
        """Reload EEPROM data if it was not previously loaded."""
        if not self._eeprom:
            self._eeprom = self._load_eeprom()

    # ------------------------------------------------------------------
    # syseepromd compatibility (read_eeprom / update_eeprom_db)
    # ------------------------------------------------------------------

    def read_eeprom(self):
        """Return the raw EEPROM bytes (bytearray), or None on error."""
        if self._eeprom_path is None:
            logger.error("EEPROM path not found in PDDF JSON")
            return None
        try:
            with open(self._eeprom_path, 'rb') as f:
                return bytearray(f.read())
        except Exception as e:
            logger.error("read_eeprom failed: %s", e)
            return None

    def _build_tlv_entries(self):
        """Map Ciena keys to (tlv_code, value) entries.

        Shared by decode_eeprom() and update_eeprom_db() so console and
        STATE_DB output match.
        """
        self._ensure_loaded()
        tlv_entries = []
        for ciena_key, value in self._eeprom.items():
            tlv_code = _CIENA_TO_TLV_CODE.get(ciena_key)
            if tlv_code is None:
                continue  # skip unmapped keys
            tlv_entries.append((tlv_code, str(value)))
        # synthesize Manufacturer (0x2B) and Platform Name (0x28, from PI)
        tlv_entries.append((0x2B, "Ciena"))
        pi_value = self._eeprom.get("PI", self._platform_identifier or "N/A")
        tlv_entries.append((0x28, pi_value))
        return tlv_entries

    def decode_eeprom(self, e=None):
        """Print the decode-syseeprom TlvInfo table for the Ciena EEPROM."""
        tlv_entries = self._build_tlv_entries()
        # 2 bytes (code+len) per entry + value bytes, + 6 for the CRC-32 TLV
        total_length = sum(2 + len(v) for _, v in tlv_entries) + 6

        print('TlvInfo Header:')
        print('   Id String:    {}'.format('Ciena MFG EEPROM'))
        print('   Version:      {}'.format('1'))
        print('   Total Length: {}'.format(total_length))

        header = ['TLV Name', 'Code', 'Len', 'Value']
        body = []
        for tlv_code, value in tlv_entries:
            body.append([
                _TLV_CODE_NAMES.get(tlv_code, "Unknown"),
                "0x{:02X}".format(tlv_code),
                len(value),
                value,
            ])
        try:
            from tabulate import tabulate
            print(tabulate(body, header, tablefmt='simple'))
        except Exception:
            # Fallback if tabulate is unavailable
            print('{:<20} {:<6} {:<4} {}'.format(*header))
            for row in body:
                print('{:<20} {:<6} {:<4} {}'.format(*row))
        print('')
        valid, computed, stored = self._verify_checksum(e)
        if valid:
            print('(checksum valid)')
        else:
            print('(*** checksum invalid: computed {} stored {})'
                  .format(computed, stored))

    def _verify_checksum(self, raw=None):
        """Verify the CS= field (CRC-32/zlib over the body before "CS=").

        Returns (is_valid, computed_hex, stored_hex); reads HW if raw is None.
        """
        if raw is None:
            raw = self.read_eeprom()
        if not raw:
            return (False, None, None)
        data = bytes(raw)
        # anchor on "\nCS=" so a value containing "CS=" can't be mistaken for it
        idx = data.find(b"\nCS=")
        if idx < 0:
            return (False, None, None)
        body = data[:idx + 1]                       # include the trailing '\n'
        stored_str = (data[idx + 4:].split(b"\n", 1)[0]
                                     .split(b"\x00", 1)[0]
                                     .strip()
                                     .decode("ascii", "ignore"))
        computed = zlib.crc32(body) & 0xFFFFFFFF
        try:
            stored = int(stored_str, 16)
        except ValueError:
            return (False, "{:08X}".format(computed), stored_str or None)
        return (computed == stored,
                "{:08X}".format(computed), "{:08X}".format(stored))

    def is_checksum_valid(self, e=None):
        """Return (valid, computed_crc) for the CS= checksum."""
        valid, computed, _stored = self._verify_checksum(e)
        return (valid, computed)

    # ------------------------------------------------------------------
    # decode-syseeprom single-field accessors (-m / -s / model).
    # CLI calls these with no arg first, then retries with data on
    # TypeError, so accept an optional arg.
    # ------------------------------------------------------------------

    def base_mac_addr(self, e=None):
        """Base MAC (EA) for `decode-syseeprom -m`."""
        return self.get_base_mac()

    def serial_number_str(self, e=None):
        """Serial (MS) for `decode-syseeprom -s`."""
        return self.get_serial()

    def modelstr(self, e=None):
        """Model/product (PI) for decode-syseeprom."""
        return self.get_model()

    def update_eeprom_db(self, raw):
        """Write ONIE-TLV-style entries to STATE_DB EEPROM_INFO.

        Called by syseepromd so `decode-syseeprom -d` and
        `show platform syseeprom` work. Returns 0 on success, -1 on error.
        """
        try:
            from sonic_py_common import daemon_base
            from swsscommon import swsscommon

            self._ensure_loaded()

            state_db = daemon_base.db_connect("STATE_DB")
            tbl = swsscommon.Table(state_db, "EEPROM_INFO")

            # shared with decode_eeprom so console and DB output match
            tlv_entries = self._build_tlv_entries()

            # 2 bytes (code+len) per entry + value bytes, + 6 for the CRC-32 TLV
            total_length = sum(2 + len(v) for _, v in tlv_entries) + 6

            # synthetic header (Ciena is key=value, but the CLI needs this)
            header_fvs = swsscommon.FieldValuePairs([
                ("Id String", "Ciena MFG EEPROM"),
                ("Version", "1"),
                ("Total Length", str(total_length)),
            ])
            tbl.set("TlvHeader", header_fvs)

            # individual TLV entries (key is lowercase hex, as the CLI reads)
            for tlv_code, value in tlv_entries:
                code_str = "0x{:02X}".format(tlv_code)
                name = _TLV_CODE_NAMES.get(tlv_code, "Unknown")
                fvs = swsscommon.FieldValuePairs([
                    ("Name", name),
                    ("Code", code_str),
                    ("Len", str(len(value))),
                    ("Value", value),
                ])
                tbl.set("0x{:02x}".format(tlv_code), fvs)

            tbl.set("Checksum", swsscommon.FieldValuePairs([("Valid", "1")]))
            # State.Initialized=1 is what decode-syseeprom checks
            tbl.set("State", swsscommon.FieldValuePairs([("Initialized", "1")]))

            return 0
        except Exception as e:
            logger.error("update_eeprom_db failed: %s", e)
            return -1

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def system_eeprom_info(self):
        """Return the full EEPROM dictionary (all known keys).

        Missing keys are filled with None so callers always see a
        consistent set of fields.  Named to match the standard
        TlvInfoDecoder API.
        """
        self._ensure_loaded()
        result = dict(self._eeprom)
        for key in CIENA_EEPROM_KEYS:
            if key not in result:
                result[key] = None
        return result

    def get_base_mac(self):
        """Return the base MAC address (EA field)."""
        self._ensure_loaded()
        return self._eeprom.get("EA", "N/A")

    def get_serial(self):
        """Return the serial number (MS / manufacturing serial field)."""
        self._ensure_loaded()
        return self._eeprom.get("MS", "N/A")

    def get_model(self):
        """Return the model / product name (PI field)."""
        self._ensure_loaded()
        return self._eeprom.get("PI", "N/A")

    def get_revision(self):
        """Return the manufacturing revision (MR field)."""
        self._ensure_loaded()
        return self._eeprom.get("MR", "N/A")
    
    def get_part_number(self):
        """Return the part number (MP field)."""
        self._ensure_loaded()
        return self._eeprom.get("MP", "N/A")

    def get_field(self, key):
        """Retrieve the value for a specific EEPROM key.

        Args:
            key (str): The EEPROM key (e.g. 'EA', 'MP', etc.)

        Returns:
            The value string, or None if not present.
        """
        self._ensure_loaded()
        return self._eeprom.get(key, None)

    # ------------------------------------------------------------------
    # DeviceBase methods
    # ------------------------------------------------------------------

    def get_name(self):
        return "System EEPROM"

    def get_presence(self):
        return os.path.exists(self._eeprom_path)

    def get_status(self):
        return self.get_presence()

    def get_position_in_parent(self):
        return -1

    def is_replaceable(self):
        return False
