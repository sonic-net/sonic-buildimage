# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#
try:
    import sys
    import getopt
    import time
    import select
    import io
    from sonic_sfp.sfputilbase import SfpUtilBase
    from os import *
    from mmap import *

except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

# from xcvrd
SFP_STATUS_REMOVED = '0'
SFP_STATUS_INSERTED = '1'
MEDIA_TYPE_OFFSET = 0
MEDIA_TYPE_WIDTH = 1
QSFP_DD_MODULE_ENC_OFFSET = 3
QSFP_DD_MODULE_ENC_WIDTH = 1

SFP_TYPE_LIST = [
    '03' # SFP/SFP+/SFP28 and later
]
QSFP_TYPE_LIST = [
    '0c', # QSFP
    '0d', # QSFP+ or later
    '11'  # QSFP28 or later
]
QSFP_DD_TYPE_LIST = [
    '18' #QSFP_DD Type
]
OSFP_TYPE_LIST=[
    '19' # OSFP 8X Type
]


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 1
    PORT_END = 34
    PORTS_IN_BLOCK = 34

    BASE_RES_PATH = "/sys/bus/pci/devices/0000:09:00.0/resource0"
    _port_to_i2c_mapping = {
        1:  10,
        2:  11,
        3:  12,
        4:  13,
        5:  14,
        6:  15,
        7:  16,
        8:  17,
        9:  18,
        10: 19,
        11: 20,
        12: 21,
        13: 22,
        14: 23,
        15: 24,
        16: 25,
        17: 26,
        18: 27,
        19: 28,
        20: 29,
        21: 30,
        22: 31,
        23: 32,
        24: 33,
        25: 34,
        26: 35,
        27: 36,
        28: 37,
        29: 38,
        30: 39,
        31: 40,
        32: 41,
        33: 1,
        34: 2,
    }

    _port_to_eeprom_mapping = {}

    _global_port_pres_dict = {}

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_ports(self):
        return list(range(self.PORT_START, self.PORTS_IN_BLOCK + 1))

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    def _read_eeprom_bytes(self, eeprom_path, offset, num_bytes):
        eeprom_raw = []
        try:
            eeprom = io.open(eeprom_path, mode="rb", buffering=0)
        except IOError:
            return None

        for i in range(0, num_bytes):
            eeprom_raw.append("0x00")

        try:
            eeprom.seek(offset)
            raw = eeprom.read(num_bytes)
        except IOError:
            eeprom.close()
            return None

        try:
            if isinstance(raw , str):
                for n in range(0, num_bytes):
                    eeprom_raw[n] = hex(ord(raw[n]))[2:].zfill(2)
            else:
                for n in range(0, num_bytes):
                    eeprom_raw[n] = hex(raw[n])[2:].zfill(2)

        except (OSError, IOError):
            eeprom.close()
            return None

        eeprom.close()
        return eeprom_raw

    def _write_eeprom_bytes(self, eeprom_path, offset, num_bytes, value):
        try:
            with io.open(eeprom_path, mode='r+b', buffering=0) as f:
                f.seek(offset)
                f.write(value[0:num_bytes])
        except (OSError, IOError):
            return False
        return True


    def get_media_type(self, port_num):
        """
        Reads optic eeprom byte to determine media type inserted
        """
        eeprom_raw = []
        eeprom_raw = self._read_eeprom_bytes(self.port_to_eeprom_mapping[port_num], MEDIA_TYPE_OFFSET,
                     MEDIA_TYPE_WIDTH)
        if eeprom_raw is not None:
            if eeprom_raw[0] in SFP_TYPE_LIST:
                sfp_type = 'SFP'
            elif eeprom_raw[0] in QSFP_TYPE_LIST:
                sfp_type = 'QSFP'
            elif eeprom_raw[0] in QSFP_DD_TYPE_LIST:
                sfp_type = 'QSFP_DD'
            else:
                #Set native port type if EEPROM type is not recognized/readable
                sfp_type = 'QSFP_DD'
        else:
            sfp_type = 'QSFP_DD'

        return sfp_type

    def pci_mem_read(self, mm, offset):
        mm.seek(offset)
        return mm.read_byte()

    def pci_mem_write(self, mm, offset, data):
        mm.seek(offset)
        mm.write_byte(data)

    def pci_set_value(self, resource, val, offset):
        fd = open(resource, O_RDWR)
        mm = mmap(fd, 0)
        val = self.pci_mem_write(mm, offset, val)
        mm.close()
        close(fd)
        return val

    def pci_get_value(self, resource, offset):
        fd = open(resource, O_RDWR)
        mm = mmap(fd, 0)
        val = self.pci_mem_read(mm, offset)
        mm.close()
        close(fd)
        return val

    def init_global_port_presence(self):
        for port_num in range(self.port_start, (self.port_end + 1)):
            presence = self.get_presence(port_num)
            if(presence):
                self._global_port_pres_dict[port_num] = '1'
            else:
                self._global_port_pres_dict[port_num] = '0'

    def mod_pres(self):
        port_pres_mask = 0
        for port_num in range(self.port_start, (self.port_end + 1)):
            presence = self.get_presence(port_num)
            if(presence):
                self._global_port_pres_dict[port_num] = '1'
                port_val = (1 << (port_num - 1))
                port_pres_mask = (port_pres_mask | port_val)
            else:
                self._global_port_pres_dict[port_num] = '0'
                port_val = ~(1 << (port_num - 1))
                port_pres_mask = (port_pres_mask & port_val)

        return port_pres_mask

    def __init__(self):
        eeprom_path = "/sys/class/i2c-adapter/i2c-{0}/{0}-0050/eeprom"

        for x in range(self.port_start, self.port_end + 1):
            self.port_to_eeprom_mapping[x] = eeprom_path.format(
                self._port_to_i2c_mapping[x])
        self.init_global_port_presence()
        SfpUtilBase.__init__(self)

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        # Port offset starts with 0x4004
        port_offset = 16388 + ((port_num-1) * 16)

        status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
        reg_value = int(status)

        # Absence of status throws error
        if (reg_value == ""):
            return False

        # Mask off 4th bit for presence
        mask = (1 << 4)

        # Mask off 1st bit for presence 33,34
        if (port_num > 32):
            mask = (1 << 0)

        # ModPrsL is active low
        if reg_value & mask == 0:
            return True

        return False

    def get_low_power_mode(self, port_num):

        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False
        if port_num > self.PORTS_IN_BLOCK:
            return False
        if self.get_media_type(port_num) == 'QSFP_DD':
            lpmode = self._read_eeprom_bytes(self.port_to_eeprom_mapping[port_num], QSFP_DD_MODULE_ENC_OFFSET, 
                     QSFP_DD_MODULE_ENC_WIDTH)
            if lpmode is not None:
                if int(lpmode[0])>>1 == 1:
                   return True
            return False
        else:
            # Port offset starts with 0x4000
            port_offset = 16384 + ((port_num-1) * 16)

            status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
            # Absence of status throws error
            if (status == ""):
                return False

            reg_value = int(status)

            # Mask off 4th bit for presence
            mask = (1 << 6)

            # LPMode is active high
            if reg_value & mask == 0:
                return False
            return True

    def set_low_power_mode(self, port_num, lpmode):

        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False
        if port_num > self.PORTS_IN_BLOCK:
            return False

        if self.get_media_type(port_num) == 'QSFP_DD':
            if lpmode is True:
                write_val = 0x10
            else:
                write_val = 0x0

            self._write_eeprom_bytes(self.port_to_eeprom_mapping[port_num], 26, 1, bytearray([write_val]))
        else:
            # Port offset starts with 0x4000
            port_offset = 16384 + ((port_num-1) * 16)
            status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
            reg_value = int(status)

            # Absence of status throws error
            if (reg_value == ""):
                return False

            # Mask off 4th bit for presence
            mask = (1 << 6)

            # LPMode is active high; set or clear the bit accordingly
            if lpmode is True:
                reg_value = reg_value | mask
            else:
                reg_value = reg_value & ~mask

            # Convert our register value back to a hex string and write back
            status = self.pci_set_value(self.BASE_RES_PATH, reg_value, port_offset)
        return True

    def reset(self, port_num):

        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        # Port offset starts with 0x4000
        port_offset = 16384 + ((port_num-1) * 16)

        status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
        reg_value = int(status)

        # Absence of status throws error
        if (reg_value == ""):
            return False

        # Mask off 4th bit for presence
        mask = (1 << 6)

        # ResetL is active low
        reg_value = reg_value & ~mask

        # Convert our register value back to a hex string and write back
        status = self.pci_set_value(self.BASE_RES_PATH, reg_value, port_offset)

        # Sleep 1 second to allow it to settle
        time.sleep(1)

        reg_value = reg_value | mask

        # Convert our register value back to a hex string and write back
        status = self.pci_set_value(self.BASE_RES_PATH, reg_value, port_offset)

        return True

    def get_register(self, reg_file):
        retval = 'ERR'
        if (not path.isfile(reg_file)):
            print(reg_file + ' not found !')
            return retval

        try:
            with fdopen(open(reg_file, O_RDONLY)) as fd:
                retval = fd.read()
        except Exception as error:
            logging.error("Unable to open ", reg_file, "file !")

        retval = retval.rstrip('\r\n')
        retval = retval.lstrip(" ")
        return retval

    def get_transceiver_change_event(self):
        port_dict = {}
        while True:
            for port_num in range(self.port_start, (self.port_end + 1)):
                presence = self.get_presence(port_num)
                if(presence and self._global_port_pres_dict[port_num] == '0'):
                    self._global_port_pres_dict[port_num] = '1'
                    port_dict[port_num] = '1'
                elif(not presence and
                     self._global_port_pres_dict[port_num] == '1'):
                    self._global_port_pres_dict[port_num] = '0'
                    port_dict[port_num] = '0'

                if(len(port_dict) > 0):
                    return True, port_dict

            time.sleep(0.5)
