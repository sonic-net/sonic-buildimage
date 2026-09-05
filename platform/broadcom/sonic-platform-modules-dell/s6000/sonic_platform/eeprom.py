#!/usr/bin/env python

########################################################################
# DellEMC S6000
#
# Module contains platform specific implementation of SONiC Platform
# Base API and provides the EEPROMs' information.
#
# The different EEPROMs available are as follows:
# - System EEPROM : Contains Serial number, Service tag, Base MA
#                   address, etc. in ONIE TlvInfo EEPROM format.
# - PSU EEPROM : Contains Serial number, Part number, Service Tag,
#                PSU type, Revision.
# - Fan EEPROM : Contains Serial number, Part number, Service Tag,
#                Fan type, Number of Fans in Fantray, Revision.
########################################################################


try:
    import os
    import binascii
    import redis
    import struct
    from collections import OrderedDict
    from sonic_platform_base.sonic_eeprom.eeprom_base import EepromDecoder
    from sonic_platform_base.sonic_eeprom.eeprom_tlvinfo import TlvInfoDecoder
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

STATE_DB_INDEX = 6

# PSU eeprom fields in format required by EepromDecoder
psu_eeprom_format = [
    ('PPID', 's', 20), ('DPN Rev', 's', 3), ('Service Tag', 's', 7),
    ('Part Number', 's', 10), ('Part Num Revision', 's', 3),
    ('Mfg Test', 's', 2), ('Redundant copy', 's', 83), ('PSU Type', 's', 1),
    ('Fab Rev', 's', 2)
    ]

# FanTray eeprom fields in format required by EepromDecoder
fantray_eeprom_format = [
    ('PPID', 's', 20), ('DPN Rev', 's', 3), ('Service Tag', 's', 7),
    ('Part Number', 's', 10), ('Part Num Revision', 's', 3),
    ('Mfg Test', 's', 2), ('Redundant copy', 's', 83),
    ('Number of Fans', 's', 1), ('Fan Type', 's', 1),
    ('Fab Rev', 's', 2)
    ]


class Eeprom(TlvInfoDecoder):
    """DellEMC Platform-specific EEPROM class"""

    I2C_DIR = "/sys/bus/i2c/devices/"

    def __init__(self, is_psu=False, psu_index=0, is_fantray=False, fantray_index=0):
        self.is_psu_eeprom = is_psu
        self.is_fantray_eeprom = is_fantray
        self.is_sys_eeprom = not (is_psu | is_fantray)

        if self.is_sys_eeprom:
            self.start_offset = 0
            self.eeprom_path = self.I2C_DIR + "i2c-10/10-0053/eeprom"
            # System EEPROM is in ONIE TlvInfo EEPROM format
            super(Eeprom, self).__init__(self.eeprom_path,
                                         self.start_offset, '', True)
            self._load_system_eeprom()
        else:
            self.start_offset = 6
            if self.is_psu_eeprom:
                self.index = psu_index
                self.eeprom_path = self.I2C_DIR \
                    + "i2c-1/1-005{}/eeprom".format(2 - self.index)
                self.format = psu_eeprom_format
            else:
                self.index = fantray_index
                self.eeprom_path = self.I2C_DIR \
                    + "i2c-11/11-005{}/eeprom".format(4 - self.index)
                self.format = fantray_eeprom_format
            EepromDecoder.__init__(self, self.eeprom_path, self.format,
                                   self.start_offset, '', True)
            self._load_device_eeprom()

    def _load_system_eeprom(self):
        """
        Reads the system EEPROM and retrieves the values corresponding
        to the codes defined as per ONIE TlvInfo EEPROM format and fills
        them in a dictionary.
        """
        try:
            # Read System EEPROM as per ONIE TlvInfo EEPROM format.
            self.eeprom_data = self.read_eeprom()
        except:
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
                self.revision = 'NA'
                return

            total_length = (eeprom[9] << 8) | (eeprom[10])
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
                                (tlv[4] << 8) | tlv[5])
                    value += tlv[6:6 + tlv[1]].decode('ascii')
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
            self.revision = self.eeprom_tlv_dict.get(
                                "0x%X" % (self._TLV_CODE_LABEL_REVISION), 'NA')

    def _load_device_eeprom(self):
        """
        Reads the Fan/PSU EEPROM and retrieves the serial number and
        model number of the device.
        """
        try:
            # Read Fan/PSU EEPROM as per the specified format.
            self.eeprom_data = EepromDecoder.read_eeprom(self)
        except:
            self.serial_number = 'NA'
            self.part_number = 'NA'
            self.revision = 'NA'
            if self.is_psu_eeprom:
                self.psu_type = 'NA'
            else:
                self.fan_type = 'NA'
        else:
            (valid, data) = self._get_eeprom_field("PPID")
            if valid:
                ppid = data
                self.serial_number = (ppid[:2] + "-" + ppid[2:8] + "-"
                                      + ppid[8:13] + "-" + ppid[13:16]
                                      + "-" + ppid[16:])
                (valid, data) = self._get_eeprom_field("DPN Rev")
                if valid:
                    self.revision = data
                    self.serial_number += "-" + data
                else:
                    self.revision = 'NA'
            else:
                self.serial_number = 'NA'
                self.revision = 'NA'

            (valid, data) = self._get_eeprom_field("Part Number")
            if valid:
                self.part_number = data
            else:
                self.part_number = 'NA'

            if self.is_psu_eeprom:
                (valid, data) = self._get_eeprom_field("PSU Type")
                if valid:
                    self.psu_type = data
                else:
                    self.psu_type = 'NA'
            else:
                (valid, data) = self._get_eeprom_field("Fan Type")
                if valid:
                    self.fan_type = data
                else:
                    self.fan_type = 'NA'

    def _get_eeprom_field(self, field_name, decode=True):
        """
        For a field name specified in the EEPROM format, returns the
        presence of the field and the value for the same.
        """
        field_start = 0
        for field in self.format:
            field_end = field_start + field[2]
            if field[0] == field_name:
                if decode:
                    return (True, self.eeprom_data[field_start:field_end].decode('ascii'))
                else:
                    return (True, self.eeprom_data[field_start:field_end])
            field_start = field_end

        return (False, None)

    def get_serial_number(self):
        """
        Returns the serial number.
        """
        return self.serial_number

    def get_part_number(self):
        """
        Returns the part number.
        """
        return self.part_number

    def get_revision(self):
        """
        Returns the hardware revision.
        """
        return self.revision

    def airflow_fan_type(self):
        """
        Returns the airflow fan type.
        """
        if self.is_psu_eeprom:
            return int(binascii.hexlify(self.psu_type.encode('utf-8')), 16)
        else:
            return int(binascii.hexlify(self.fan_type.encode('utf-8')), 16)

    # System EEPROM specific methods
    def get_base_mac(self):
        """
        Returns the base MAC address found in the system EEPROM.
        """
        return self.base_mac

    def get_model(self):
        """
        Returns the Model name.
        """
        return self.model_str

    def get_serial(self):
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


class EepromS6000(EepromDecoder):

    _EEPROM_MAX_LEN = 128

    _BLK_HDR_LEN   = 6
    _BLK_HDR_MAGIC = b'\x3a\x29'
    _BLK_HDR_REVID = 1

    _BLK_CODE_MFG = 0x20
    _BLK_CODE_SW  = 0x1F
    _BLK_CODE_MAC = 0x21

    _BLK_MFG_FORMAT = [
        ("PPID", 20), ("DPN Rev", 3), ("Service Tag", 7), ("Part Number", 10),
        ("Part Number Rev", 3), ("Mfg Test Results", 2)
    ]

    _BLK_SW_FORMAT = [("Card ID", 4), ("Module ID", 2)]
    _BLK_MAC_FORMAT = [("Base MAC address", 6)]

    _BLK_INFO = OrderedDict([
        (_BLK_CODE_MFG, {"name": "MFG BLOCK", "size": 64,
                         "offset": 0x0, "format": _BLK_MFG_FORMAT}),
        (_BLK_CODE_SW, {"name": "SW BLOCK", "size": 48,
                        "offset": 0x40, "format": _BLK_SW_FORMAT}),
        (_BLK_CODE_MAC, {"name": "MAC BLOCK", "size": 16,
                         "offset": 0x70, "format": _BLK_MAC_FORMAT})
    ])

    def __init__(self, is_plugin=False):
        self.eeprom_path = "/sys/bus/i2c/devices/i2c-10/10-0053/eeprom"
        super(EepromS6000, self).__init__(self.eeprom_path, None, 0, '', True)

        if not is_plugin:
            self.eeprom_data = "N/A"  if os.geteuid() != 0 else self.read_eeprom()

    def _is_valid_block_checksum(self, e):
        crc = self.compute_dell_crc(e[:-2])
        return crc == struct.unpack('<H', e[-2:])[0]

    def _is_valid_block(self, e, blk_code):
        return (e[:2] == self._BLK_HDR_MAGIC
                and struct.unpack('<H', e[2:4])[0] == self._BLK_INFO[blk_code]["size"]
                and e[4] == blk_code
                and e[5] == self._BLK_HDR_REVID)

    def _get_eeprom_field(self, e, blk_code, field_name, decode=True):
        """
        For a field name specified in the EEPROM format, returns the
        presence of the field and the value for the same.
        """
        blk_start = self._BLK_INFO[blk_code]["offset"]
        blk_end = blk_start + self._BLK_INFO[blk_code]["size"]
        if not self._is_valid_block(e[blk_start:blk_end], blk_code):
            return (False, None)

        field_start = blk_start + self._BLK_HDR_LEN
        for field in self._BLK_INFO[blk_code]["format"]:
            field_end = field_start + field[1]
            if field[0] == field_name:
                if decode:
                    return (True, e[field_start:field_end].decode('ascii'))
                else:
                    return (True, e[field_start:field_end])
            field_start = field_end

        return (False, None)

    def decode_eeprom(self, e):
        """
        Decode and print out the contents of the EEPROM.
        """
        print("     Field Name      Len        Value")
        print("-------------------- --- --------------------")
        for blk_code in list(self._BLK_INFO.keys()):
            blk_start = self._BLK_INFO[blk_code]["offset"]
            blk_end = blk_start + self._BLK_INFO[blk_code]["size"]
            if not self._is_valid_block(e[blk_start:blk_end], blk_code):
                print("Invalid Block starting at EEPROM offset %d" % (blk_start))
                return

            offset = blk_start + self._BLK_HDR_LEN
            for f in self._BLK_INFO[blk_code]["format"]:
                if f[0] == "Num MACs" or f[0] == "Module ID":
                    data = str(struct.unpack('<H', e[offset:offset+f[1]])[0])
                elif f[0] == "Card ID":
                    data = hex(struct.unpack('<I', e[offset:offset+f[1]])[0])
                elif f[0] == "Base MAC address":
                    data = ":".join(["{:02x}".format(T) for T in e[offset:offset+f[1]]]).upper()
                else:
                    data = e[offset:offset+f[1]].decode('ascii')
                print("{:<20s} {:>3d} {:<s}".format(f[0], f[1], data))
                offset += f[1]

            if not self._is_valid_block_checksum(e[blk_start:blk_end]):
                print("(*** block checksum invalid)")

    def read_eeprom(self):
        """
        Read the EEPROM contents.
        """
        return self.read_eeprom_bytes(self._EEPROM_MAX_LEN)

    def read_eeprom_db(self):
        """
        Print out the contents of the EEPROM from database.
        """
        client = redis.Redis(db=STATE_DB_INDEX)
        db_state = client.hget('EEPROM_INFO|State', 'Initialized')
        if db_state != '1':
            return -1

        print("     Field Name      Len        Value")
        print("-------------------- --- --------------------")
        for blk_code in list(self._BLK_INFO.keys()):
            blk_name = self._BLK_INFO[blk_code]["name"]
            blk_start = self._BLK_INFO[blk_code]["offset"]
            is_valid = client.hget('EEPROM_INFO|{}'.format(blk_name), 'Valid')
            if is_valid == '0':
                print("Invalid Block starting at EEPROM offset %d" % (blk_start))
                break

            for f in self._BLK_INFO[blk_code]["format"]:
                data = client.hget('EEPROM_INFO|{}'.format(f[0]), 'Value')
                print("{:<20s} {:>3d} {:<s}".format(f[0], f[1], data))

            is_checksum_valid = client.hget('EEPROM_INFO|{}'.format(blk_name), 'Checksum_Valid')
            if is_checksum_valid == '0':
                print("(*** block checksum invalid)")

        return 0

    def update_eeprom_db(self, e):
        """
        Decode the contents of the EEPROM and update the contents to database
        """
        client = redis.Redis(db=STATE_DB_INDEX)
        for blk_code in list(self._BLK_INFO.keys()):
            blk_name = self._BLK_INFO[blk_code]["name"]
            blk_start = self._BLK_INFO[blk_code]["offset"]
            blk_end = blk_start + self._BLK_INFO[blk_code]["size"]
            if not self._is_valid_block(e[blk_start:blk_end], blk_code):
                client.hset('EEPROM_INFO|{}'.format(blk_name), 'Valid', '0')
                print("Invalid Block starting at EEPROM offset %d" % (blk_start))
                break
            else:
                client.hset('EEPROM_INFO|{}'.format(blk_name), 'Valid', '1')

            offset = blk_start + self._BLK_HDR_LEN
            for f in self._BLK_INFO[blk_code]["format"]:
                if f[0] == "Num MACs" or f[0] == "Module ID":
                    data = str(struct.unpack('<H', e[offset:offset+f[1]])[0])
                elif f[0] == "Card ID":
                    data = hex(struct.unpack('<I', e[offset:offset+f[1]])[0])
                elif f[0] == "Base MAC address":
                    data = ":".join(["{:02x}".format(T) for T in e[offset:offset+f[1]]]).upper()
                else:
                    data = e[offset:offset+f[1]].decode('ascii')
                client.hset('EEPROM_INFO|{}'.format(f[0]), 'Value', data.strip('\x00'))
                offset += f[1]

            if not self._is_valid_block_checksum(e[blk_start:blk_end]):
                client.hset('EEPROM_INFO|{}'.format(blk_name), 'Checksum_Valid', '0')
            else:
                client.hset('EEPROM_INFO|{}'.format(blk_name), 'Checksum_Valid', '1')

        client.hset('EEPROM_INFO|State', 'Initialized', '1')
        return 0

    def get_base_mac(self):
        """
        Returns the base MAC address found in the system EEPROM.
        """
        (valid, data) = self._get_eeprom_field(self.eeprom_data, self._BLK_CODE_MAC,
                                               "Base MAC address", False)
        if valid:
            return ":".join(["{:02x}".format(T) for T in data]).upper()
        else:
            return 'NA'

    def get_model(self):
        """
        Returns the Model name.
        """
        return 'S6000'

    def get_part_number(self):
        """
        Returns the part number.
        """
        (valid, data) = self._get_eeprom_field(self.eeprom_data,
                                               self._BLK_CODE_MFG, "Part Number")
        if valid:
            return data.rstrip('\x00')
        else:
            return 'NA'

    def get_serial(self):
        """
        Returns the Service tag.
        """
        (valid, data) = self._get_eeprom_field(self.eeprom_data,
                                               self._BLK_CODE_MFG, "Service Tag")
        if valid:
            return data
        else:
            return 'NA'

    def get_serial_number(self):
        """
        Returns the serial number.
        """
        (valid, data) = self._get_eeprom_field(self.eeprom_data,
                                               self._BLK_CODE_MFG, "PPID")
        if valid:
            return data
        else:
            return 'NA'

    def get_revision(self):
        """
        Returns the hardware revision.
        """
        (valid, data) = self._get_eeprom_field(self.eeprom_data,
                                               self._BLK_CODE_MFG, "DPN Rev")
        if valid:
            return data
        else:
            return 'NA'

    # EEPROM Plugin specific methods
    def is_checksum_valid(self, e):
        # Checksum is already calculated before
        return (True, 0)

    def mgmtaddrstr(self, e):
        """
        Returns the base MAC address.
        """
        (valid, data) = self._get_eeprom_field(e, self._BLK_CODE_MAC, "Base MAC address", False)
        if valid:
            return ":".join(["{:02x}".format(T) for T in data]).upper()
        else:
            return 'NA'

    def modelstr(self, e):
        """
        Returns the Model name.
        """
        return 'S6000'

    def serial_number_str(self, e):
        """
        Returns the Service Tag.
        """
        (valid, data) = self._get_eeprom_field(e, self._BLK_CODE_MFG, "Service Tag")

        if valid:
            return data
        else:
            return 'NA'
