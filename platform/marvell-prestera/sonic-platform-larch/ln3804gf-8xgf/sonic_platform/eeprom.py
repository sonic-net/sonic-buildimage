#!/usr/bin/env python

########################################################################
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
    import redis
    import subprocess
    import configparser
    from sonic_py_common.logger import Logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

logger = Logger()
STATE_DB_INDEX = 6
MOUNT_POINT="/mnt/p1"
MOUNT_DEV="/dev/mmcblk0p1"
DEVICE_INFO_PATH=MOUNT_POINT+"/device_info.ini"

# PSU eeprom fields in format required by EepromDecoder
psu_eeprom_format = [
    ('PPID', 's', 20), ('DPN Rev', 's', 3), ('Service Tag', 's', 7),
    ('Part Number', 's', 10), ('Part Num Revision', 's', 3),
    ('Mfg Test', 's', 2), ('Redundant copy', 's', 83), ('PSU Type', 's', 1),
    ('Fab Rev', 's', 2)
    ]

# Fan eeprom fields in format required by EepromDecoder
fan_eeprom_format = [
    ('Model', 's', 12), ('Serial Number', 's', 13)
    ]

eeprom_default_dict = {
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
    0xFC: "Undefined",
    0xFD: "Vendor Extension",
    0xFE: "CRC-32"
}

redis_client = redis.Redis(db=STATE_DB_INDEX)

class Eeprom():

    def __init__(self, is_psu=False, psu_index=0, is_fan=False, fan_index=0):
        self.is_psu_eeprom = is_psu
        self.is_fan_eeprom = is_fan
        self.is_sys_eeprom = not (is_psu | is_fan)
        self.product_name = "N/A"
        self.serial_number = "N/A"
        self.part_number = "N/A"
        self.base_mac = "N/A"
        self.model_str = "N/A"
        self.manufacture_date = "N/A"
        self.manufacturer = "N/A"
        self.paltform_name = "N/A"
        self.eeprom_tlv_dict = {}

        if self.is_sys_eeprom:
            if not self.__is_db_updated():
                self.eeprom_tlv_dict = self.read_eeprom()
                if self.eeprom_tlv_dict:
                    self.update_eeprom_db(self.eeprom_tlv_dict)
        else:
            print ("No other eeprom")

    def __is_db_updated(self):
        db_state = redis_client.hget('EEPROM_INFO|State', 'Initialized')
        if db_state != b'1':
            return False
        return True

    def update_eeprom_db(self, e):
        """
        As the device doesn't have a system EEPROM this function reads
        device information from the file DEVICE_INFO_PATH and fills EEPROM_INFO
        table in Redis DB
        """

        if not e:
            logger.log_error("empty eeprom dictionary passed to update_eeprom_db()")
            return -1

        redis_client.hset('EEPROM_INFO|State', 'Initialized', '1')
        redis_client.hmset('EEPROM_INFO|TlvHeader', {
            'Id String': 'TlvHeader',
            'Version': '1',
            'Total Length': '170'
        })

        redis_client.hset('EEPROM_INFO|Checksum', 'Valid', '1')

        for key, val in e.items():
            eeprom_info_field = "EEPROM_INFO|" + hex(key)
            redis_client.hmset(eeprom_info_field, {"Len": len(val), "Name": eeprom_default_dict[key], "Value": val})

        return 0

    def read_eeprom(self):
        """
        As the device doesn't have a system EEPROM this function reads
        device information from the file DEVICE_INFO_PATH and fills EEPROM_INFO
        table in Redis DB
        """

        if os.geteuid() != 0:
            # if not root user do nothing
            logger.log_warning("root privileges are required to read eeprom file")
            return None

        try:
            subprocess.run(["mkdir", "-p", MOUNT_POINT])
            subprocess.run(["mount", "-o", "ro", MOUNT_DEV, MOUNT_POINT])
            config = configparser.ConfigParser()
            config.read(DEVICE_INFO_PATH)
            subprocess.run(["umount", MOUNT_POINT])
        except():
            logger.log_error(f'Failed to parse {DEVICE_INFO_PATH} file')
            return None

        for section in config.sections():
            for key, val in config[section].items():
                if key == "product name":
                    self.eeprom_tlv_dict[0x21] = val
                    self.product_name = val

                elif key == "part number":
                    self.eeprom_tlv_dict[0x22] = val
                    self.part_number = val

                elif key == "serial number":
                    self.eeprom_tlv_dict[0x23] = val
                    self.serial_number = val

                elif key == "manufacture date":
                    self.eeprom_tlv_dict[0x25] = val
                    self.manufacture_date = val

                elif key == "platform name":
                    self.eeprom_tlv_dict[0x28] = val
                    self.paltform_name = val

                elif key == "manufacturer":
                    self.eeprom_tlv_dict[0x2b] = val
                    self.manufacturer = val

        try:
            self.base_mac = subprocess.check_output(["cat", f"/sys/class/net/eth0/address"]).decode().strip().upper()
            self.eeprom_tlv_dict[0x24] = self.base_mac
        except():
            logger.log_error(f'Failed to read MAC address from file [/sys/class/net/eth0/address]')
            pass

        return self.eeprom_tlv_dict

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
        return self.serial_number

    def system_eeprom_info(self):
        """
        Returns a dictionary, where keys are the type code defined in
        ONIE EEPROM format and values are their corresponding values
        found in the system EEPROM.
        """
        return self.eeprom_tlv_dict
