# System EEPROM for the LX2160ARDB.
# The reference board has no ONIE TlvInfo EEPROM, so the TlvInfo image is
# built in software from what the board does provide: the management port's
# MAC address (dpni.1, eth0 under SONiC; found by device, not by name) as
# the base MAC, and the SoC/board identity.
# The image is kept in memory and served through read_eeprom_bytes(), so
# decode-syseeprom and syseepromd use the standard TlvInfoDecoder path as on
# hardware, and no file is written (CLI tools also run as non-root users).
# Replace with the real EEPROM device on a product board.
import glob

from sonic_platform_base.sonic_eeprom.eeprom_tlvinfo import TlvInfoDecoder

EEPROM_NAME = "lx2160ardb-soft-syseeprom"
MGMT_DPNI = "dpni.1"


def _read(path, default=""):
    try:
        with open(path) as f:
            return f.read().strip().strip("\0")
    except OSError:
        return default


def _mgmt_mac():
    for a in glob.glob("/sys/bus/fsl-mc/devices/%s/net/*/address" % MGMT_DPNI):
        return _read(a)
    return "00:00:00:00:00:00"


class Eeprom(TlvInfoDecoder):
    def __init__(self):
        TlvInfoDecoder.__init__(self, EEPROM_NAME, 0, "", True)
        self._image = None

    def _fields(self):
        soc = _read("/sys/devices/soc0/soc_id", "unknown")
        rev = _read("/sys/devices/soc0/revision", "")
        return [
            (self._TLV_CODE_PRODUCT_NAME, "LX2160ARDB"),
            (self._TLV_CODE_PART_NUMBER, "LX2160A-RDB"),
            (self._TLV_CODE_SERIAL_NUMBER, "N/A"),
            (self._TLV_CODE_MAC_BASE, _mgmt_mac()),
            (self._TLV_CODE_DEVICE_VERSION, "2"),
            (self._TLV_CODE_LABEL_REVISION, "B"),
            (self._TLV_CODE_PLATFORM_NAME, "arm64-nxp_lx2160ardb-r0"),
            (self._TLV_CODE_MAC_SIZE, "1"),
            (self._TLV_CODE_MANUF_NAME, "NXP"),
            (self._TLV_CODE_VENDOR_NAME, "NXP"),
            (self._TLV_CODE_DIAG_VERSION, "%s rev %s" % (soc, rev)),
        ]

    def _build(self):
        # TlvInfo header, TLVs, then the CRC-32 TLV over everything before it
        tlvs = bytearray()
        for code, value in self._fields():
            tlvs += self.encoder((code,), value)
        tlvs += bytearray([self._TLV_CODE_CRC_32, 4])
        e = self._TLV_INFO_ID_STRING + bytearray([self._TLV_INFO_VERSION]) + \
            bytearray([(len(tlvs) + 4) >> 8 & 0xFF, (len(tlvs) + 4) & 0xFF]) + tlvs
        return e + self.encode_checksum(self.calculate_checksum(e))

    def read_eeprom_bytes(self, byteCount, offset=0):
        if self._image is None:
            self._image = self._build()
        return self._image[offset:offset + byteCount]

    def _e(self):
        return self.read_eeprom()

    def base_mac_addr(self, e=None):
        return TlvInfoDecoder.base_mac_addr(self, e or self._e())

    def modelstr(self, e=None):
        return TlvInfoDecoder.modelstr(self, e or self._e())

    def serial_number_str(self, e=None):
        return TlvInfoDecoder.serial_number_str(self, e or self._e())

    def get_eeprom_dict(self):
        # {"0x21": "LX2160ARDB", ...}, values rendered as decode-syseeprom
        # prints them
        e = self._e()
        result = {}
        for code, _ in self._fields():
            found, tlv = self.get_tlv_field(e, code)
            if found:
                result["0x%02X" % code] = self.decoder(None, bytearray([tlv[0], tlv[1]]) + tlv[2])[1]
        return result
