#!/usr/bin/env python

########################################################################
#
# Module contains platform specific implementation of SONiC Platform
# Base API and provides the EEPROMs' information.
#
# The different EEPROMs available are as follows:
# - System EEPROM : Contains Serial number, Service tag, Base MA
#                   address, etc. in ONIE TlvInfo EEPROM format.
########################################################################

try:
    from sonic_platform_base.sonic_eeprom.eeprom_tlvinfo import TlvInfoDecoder
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Eeprom(TlvInfoDecoder):

    I2C_DIR = "/sys/bus/i2c/devices/"

    def __init__(self, is_psu=False, psu_index=0, is_fan=False, fan_index=0):
        self.is_psu_eeprom = is_psu
        self.is_fan_eeprom = is_fan
        self.is_sys_eeprom = not (is_psu | is_fan)

        if self.is_sys_eeprom:
            self.start_offset = 0
            self.eeprom_path = self._find_sys_eeprom()
            # System EEPROM is in ONIE TlvInfo EEPROM format
            super(Eeprom, self).__init__(self.eeprom_path,
                                         self.start_offset, '', True)
            self._load_system_eeprom()
        else:
            print ("No other eeprom")

    # The Lanaotek CN9130 ComE module has no TlvInfo EEPROM at cp0 i2c0 0x50
    # (the SolidRun module's location); where it lives is still unknown
    # (#35885). Until then a minimal TlvInfo image is synthesised from the
    # U-Boot environment (ethaddr) so that syseepromd and the base MAC work.
    CANDIDATES = ["/sys/bus/i2c/devices/0-0050/eeprom"]
    SYNTH_PATH = "/var/run/platform_cache/ln4348y-8z-v1/syseeprom.bin"

    def _find_sys_eeprom(self):
        import glob
        import os
        import subprocess
        for path in self.CANDIDATES + sorted(glob.glob("/sys/bus/i2c/devices/*-0050/eeprom")):
            try:
                with open(path, "rb") as f:
                    if f.read(8) == b"TlvInfo\x00":
                        return path
            except OSError:
                continue
        # A valid image written earlier in this boot (the platform init script
        # builds it on the host; /var/run/platform_cache is read-only inside pmon)
        try:
            with open(self.SYNTH_PATH, "rb") as f:
                blob = f.read()
            if blob[:8] == b"TlvInfo\x00" and len(blob) >= 11 + ((blob[9] << 8) | blob[10]):
                return self.SYNTH_PATH
        except OSError:
            pass
        mac = None
        try:
            mac = subprocess.check_output(["fw_printenv", "-n", "ethaddr"], timeout=5).decode().strip()
        except Exception:
            pass
        try:
            mac_b = bytes(int(x, 16) for x in mac.split(":")) if mac and mac.count(":") == 5 else b"\x00" * 6
        except ValueError:
            mac_b = b"\x00" * 6
        def tlv(code, value):
            return bytes([code, len(value)]) + value
        body = b""
        body += tlv(0x21, b"LN-4348Y-8Z")                     # product name
        body += tlv(0x22, b"LN4348Y-8Z")                      # part number
        body += tlv(0x23, b"unknown")                         # serial
        body += tlv(0x24, mac_b)                              # base MAC
        body += tlv(0x2B, b"Larch")                           # manufacturer
        body += tlv(0x2D, b"Larch")                           # vendor
        body += tlv(0x28, b"arm64-larch_ln4348y-8z-v1")       # platform
        body += tlv(0x2A, bytes([0x00, 0x01]))                # MAC addresses
        body += tlv(0xFE, bytes(4))                           # CRC placeholder
        hdr = b"TlvInfo\x00" + bytes([1]) + len(body).to_bytes(2, "big")
        blob = hdr + body
        import binascii
        crc = binascii.crc32(blob[:-4]) & 0xFFFFFFFF
        blob = blob[:-4] + crc.to_bytes(4, "big")
        try:
            os.makedirs(os.path.dirname(self.SYNTH_PATH), exist_ok=True)
            with open(self.SYNTH_PATH, "wb") as f:
                f.write(blob)
        except OSError:
            pass
        return self.SYNTH_PATH

    def _load_system_eeprom(self):
        """
        Reads the system EEPROM and retrieves the values corresponding
        to the codes defined as per ONIE TlvInfo EEPROM format and fills
        them in a dictionary.
        """
        try:
            # Read System EEPROM as per ONIE TlvInfo EEPROM format.
            self.eeprom_data = self.read_eeprom()
        except Exception as e:
            self.base_mac = 'NA'
            self.serial_number = 'NA'
            self.part_number = 'NA'
            self.model_str = 'NA'
            self.serial = 'NA'
            self.eeprom_tlv_dict = dict()
        else:
            eeprom = self.eeprom_data
            self.eeprom_tlv_dict = dict()

            if not self.is_valid_tlvinfo_header(eeprom):
                self.base_mac = 'NA'
                self.serial_number = 'NA'
                self.part_number = 'NA'
                self.model_str = 'NA'
                self.serial = 'NA'
                return

            total_length = (eeprom[9] << 8) | eeprom[10]
            tlv_index = self._TLV_INFO_HDR_LEN
            tlv_end = self._TLV_INFO_HDR_LEN + total_length

            while (tlv_index + 2) < len(eeprom) and tlv_index < tlv_end:
                if not self.is_valid_tlv(eeprom[tlv_index:]):
                    break

                tlv = eeprom[tlv_index:tlv_index + 2
                             + eeprom[tlv_index + 1]]
                code = "0x%02X" % (tlv[0])

                if tlv[0] == self._TLV_CODE_VENDOR_EXT:
                    value = str((tlv[2] << 24) | (tlv[3] << 16) |
                                (tlv[4]<< 8) | tlv[5])
                    value += str(tlv[6:6 + tlv[1]])
                else:
                    name, value = self.decoder(None, tlv)

                self.eeprom_tlv_dict[code] = value
                if eeprom[tlv_index] == self._TLV_CODE_CRC_32:
                    break

                tlv_index += eeprom[tlv_index+1] + 2

            self.base_mac = self.eeprom_tlv_dict.get(
                                "0x%X" % (self._TLV_CODE_MAC_BASE), 'NA')
            self.serial_number = self.eeprom_tlv_dict.get(
                                "0x%X" % (self._TLV_CODE_SERIAL_NUMBER), 'NA')
            self.part_number = self.eeprom_tlv_dict.get(
                                "0x%X" % (self._TLV_CODE_PART_NUMBER), 'NA')
            self.model_str = self.eeprom_tlv_dict.get(
                                "0x%X" % (self._TLV_CODE_PRODUCT_NAME), 'NA')
            self.serial = self.eeprom_tlv_dict.get(
                                "0x%X" % (self._TLV_CODE_SERVICE_TAG), 'NA')

    def _get_eeprom_field(self, field_name):
        """
        For a field name specified in the EEPROM format, returns the
        presence of the field and the value for the same.
        """
        field_start = 0
        for field in self.format:
            field_end = field_start + field[2]
            if field[0] == field_name:
                return (True, self.eeprom_data[field_start:field_end])
            field_start = field_end

        return (False, None)

    def serial_number_str(self):
        """
        Returns the serial number.
        """
        return self.serial_number

    def part_number_str(self):
        """
        Returns the part number.
        """
        return self.part_number

    def airflow_fan_type(self):
        """
        Returns the airflow fan type.
        """
        if self.is_psu_eeprom:
            return int(self.psu_type.encode('hex'), 16)
        if self.is_fan_eeprom:
            return int(self.fan_type.encode('hex'), 16)

    # System EEPROM specific methods
    def base_mac_addr(self):
        """
        Returns the base MAC address found in the system EEPROM.
        """
        return self.base_mac

    def modelstr(self):
        """
        Returns the Model name.
        """
        return self.model_str

    def serial_str(self):
        """
        Returns the servicetag number.
        """
        return self.serial

    def system_eeprom_info(self):
        """
        Returns a dictionary, where keys are the type code defined in
        ONIE EEPROM format and values are their corresponding values
        found in the system EEPROM.
        """
        return self.eeprom_tlv_dict
