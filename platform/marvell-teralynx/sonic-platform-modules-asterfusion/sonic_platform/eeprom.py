##############################################################################
# Asterfusion CX-N Devices EEPROM                                            #
#                                                                            #
# Platform and model specific eeprom subclass, inherits from the base class, #
# and provides the followings:                                               #
# - the eeprom format definition                                             #
# - specific encoder/decoder if there is special need                        #
#                                                                            #
##############################################################################

try:
    import binascii
    import struct
    from pathlib import Path

    from .constants import *
    from .helper import Helper
    from .logger import Logger

    from sonic_platform_base.sonic_eeprom.eeprom_tlvinfo import TlvInfoDecoder
except ImportError as err:
    raise ImportError(str(err) + "- required module not found")


class Eeprom(TlvInfoDecoder):
    """Platform-specific EEPROM class"""

    def __init__(self):
        # type: () -> None
        self._helper = Helper()
        self._logger = Logger()
        self._init_system_eeprom()
        TlvInfoDecoder.__init__(self, self._eeprom_sysfs_or_cache_path, 0, "", True)
        self._eeprom_code_value_dict = {}  # type: dict[str, str]
        self._load_system_eeprom()

    def _init_system_eeprom(self):
        if Path(EEPROM_CACHE_PATH).exists():
            self._eeprom_sysfs_or_cache_path = EEPROM_CACHE_PATH
            self._logger.log_debug(
                "Found EEPROM cache at path <{}>".format(
                    self._eeprom_sysfs_or_cache_path
                )
            )
            return
        if Path(EEPROM_I2C_SYSFS_PATH).exists():
            self._eeprom_sysfs_or_cache_path = EEPROM_I2C_SYSFS_PATH
            self._logger.log_debug(
                "Found EEPROM I2C sysfs at path <{}>".format(
                    self._eeprom_sysfs_or_cache_path
                )
            )
            return
        for hwmon_path in Path(EEPROM_UART_FUZZY_MATCH_DIR).glob("*"):
            self._logger.log_debug(
                "Searching under path <{}>".format(hwmon_path.as_posix())
            )
            eeprom_uart_sysfs_dir = Path(hwmon_path, EEPROM_UART_SYSFS_DIR)
            if not Path(eeprom_uart_sysfs_dir, "eeprom_crc32").exists():
                self._logger.log_debug("No valid EEPROM found, skipping")
                continue
            self._logger.log_debug(
                "Found valid EEPROM at path <{}>".format(eeprom_uart_sysfs_dir)
            )
            self._eeprom_sysfs_or_cache_path = EEPROM_CACHE_PATH
            eeprom_cache_file = Path(self._eeprom_sysfs_or_cache_path)
            eeprom_cache_file.parent.mkdir(parents=True, exist_ok=True)
            eeprom_header = b"TlvInfo\x00\x01\x00"
            eeprom_length = 0
            eeprom_data = b""
            for field, code in EEPROM_FIELD_CODE_MAP:
                field_path = Path(eeprom_uart_sysfs_dir, field)
                value = field_path.read_bytes()
                if code == b"\x24":
                    value = binascii.unhexlify(b"".join(value[:17].split(b":")))
                elif code == b"\x26":  # or code == b"\x27":
                    value = binascii.unhexlify(format(int(value), "02x"))
                elif code == b"\x2a":
                    upper = b"\x00"
                    value_length = len(value)
                    if value_length == 0:
                        value = "0"
                    elif value_length > 1:
                        upper = b""
                    value = upper + binascii.unhexlify(format(int(value), "02x"))
                elif code == b"\xfe":
                    value = bytes.fromhex(value[2:].decode())
                value_length = len(value)
                length = struct.pack("B", value_length)
                eeprom_length += value_length + 2
                eeprom_data += code + length + value
                self._logger.log_debug(
                    "Retrieved EEPROM field <{}> code <0x{}> value <{}> from path <{}>".format(
                        " ".join(field.split("_")).title(),
                        code.hex(),
                        value,
                        field_path.as_posix(),
                    )
                )
            eeprom_length = struct.pack("B", eeprom_length)
            eeprom_data_nocrc = (eeprom_header + eeprom_length + eeprom_data)[:-4]
            crc_raw = self.calculate_checksum(eeprom_data_nocrc)
            crc_bytes = bytes(
                (crc_raw & (0xFF << (i * 8))) >> (i * 8) for i in range(0, 4)
            )[::-1]
            eeprom_data_recrc = eeprom_data_nocrc + crc_bytes
            eeprom_cache_file.write_bytes(eeprom_data_recrc)
            self._logger.log_info(
                "Initialized EEPROM cache at path <{}>".format(
                    self._eeprom_sysfs_or_cache_path
                )
            )
            break
        else:
            self._logger.log_fatal("Failed in initializing EEPROM data")
            raise RuntimeError("failed in initializing EEPROM data")

    def _load_system_eeprom(self):
        # type: () -> None
        """
        Reads the system EEPROM and retrieves the values corresponding
        to the codes defined as per ONIE TlvInfo EEPROM format and fills
        them in a dictionary.
        """
        self._eeprom_data = self.read_eeprom()  # type: bytearray
        eeprom_length = (self._eeprom_data[9] << 8) | self._eeprom_data[10]
        tlv_index = self._TLV_INFO_HDR_LEN
        tlv_end = self._TLV_INFO_HDR_LEN + eeprom_length
        while (tlv_index + 2) < len(self._eeprom_data) and tlv_index < tlv_end:
            if not self.is_valid_tlv(self._eeprom_data[tlv_index:]):
                self._logger.log_error(
                    "Found invalid TLV in EEPROM data at <{}>".format(tlv_index)
                )
                break

            tlv = self._eeprom_data[
                tlv_index : tlv_index + 2 + self._eeprom_data[tlv_index + 1]
            ]
            code = "0x{:02X}".format(tlv[0])

            if tlv[0] == self._TLV_CODE_VENDOR_EXT:
                value = str((tlv[2] << 24) | (tlv[3] << 16) | (tlv[4] << 8) | tlv[5])
                value += str(tlv[6 : 6 + tlv[1]])
            else:
                _, value = self.decoder(None, tlv)

            self._logger.log_debug(
                "Decoded EEPROM data code=<{}> value=<{}>".format(code, value)
            )
            self._eeprom_code_value_dict[code] = value
            if self._eeprom_data[tlv_index] == self._TLV_CODE_CRC_32:
                break

            tlv_index += self._eeprom_data[tlv_index + 1] + 2
        # Static information
        self._get_base_mac_addr = self._eeprom_code_value_dict.get(
            "0x{:02X}".format(self._TLV_CODE_MAC_BASE), NOT_AVAILABLE
        )
        self._get_mgmt_addr_str = self._get_base_mac_addr
        self._get_model_str = self._eeprom_code_value_dict.get(
            "0x{:02X}".format(self._TLV_CODE_PRODUCT_NAME), NOT_AVAILABLE
        )
        self._get_part_number_str = self._eeprom_code_value_dict.get(
            "0x{:02X}".format(self._TLV_CODE_PART_NUMBER), NOT_AVAILABLE
        )
        self._get_serial_number_str = self._eeprom_code_value_dict.get(
            "0x{:02X}".format(self._TLV_CODE_SERIAL_NUMBER), NOT_AVAILABLE
        )
        self._get_label_revision_str = self._eeprom_code_value_dict.get(
            "0x{:02X}".format(self._TLV_CODE_LABEL_REVISION), NOT_AVAILABLE
        )

    def get_base_mac_addr(self):
        # type: () -> str
        """
        Returns the base MAC address found in the system EEPROM.
        """
        return self._get_base_mac_addr

    def get_mgmt_addr_str(self):
        # type: () -> str
        """
        Returns the base MAC address to use for the Ethernet
        management interface(s) on the CPU complex.
        """
        return self._get_base_mac_addr

    def get_model_str(self):
        # type: () -> str
        """
        Returns the value field of the Product Name TLV as a string
        """
        return self._get_model_str

    def get_part_number_str(self):
        # type: () -> str
        """
        Returns the value field of the Part Number TLV as a string
        """
        return self._get_part_number_str

    def get_serial_number_str(self):
        # type: () -> str
        """
        Returns the value field of the Serial Number TLV as a string
        """
        return self._get_serial_number_str

    def get_label_revision_str(self):
        # type: () -> str
        """
        Returns the value field of the Serial Number TLV as a string
        """
        return self._get_label_revision_str

    def get_system_eeprom_info(self):
        # type: () -> dict[str, str]
        """
        Returns a dictionary, where keys are the type code defined in
        ONIE EEPROM format and values are their corresponding values
        found in the system EEPROM.
        """
        return self._eeprom_code_value_dict
