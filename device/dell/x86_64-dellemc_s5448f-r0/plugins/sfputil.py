# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import struct
    import sys
    import getopt
    import time
    import select
    import os
    import logging
    from sonic_sfp.sfputilbase import SfpUtilBase
    from mmap import *

except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

#from xcvrd
SFP_STATUS_REMOVED = '0'
SFP_STATUS_INSERTED = '1'

HEX_10 = 16
HEX_20 = 32
INDEX_1 = 1
INDEX_2 = 3
INDEX_3 = 47
PORT_START = 1
PORT_6 = 6
PORT_54 = 54
PORT_END = 58
PORTS_IN_BLOCK = 56
BASE_OFFSET = 16388
BASE_OFFSET_2 = 16384

class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    BASE_RES_PATH = "/sys/bus/pci/devices/0000:04:00.0/resource0"

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
        return range(self.PORT_START, self.PORTS_IN_BLOCK + 1)

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    def pci_mem_read(self, mm, offset):
        mm.seek(offset)
        read_data_stream = mm.read(4)
        reg_val = struct.unpack('I', read_data_stream)
        mem_val = str(reg_val)[1:-2]
        # print "reg_val read:%x"%reg_val
        return mem_val

    def pci_mem_write(self, mm, offset, data):
        mm.seek(offset)
        # print "data to write:%x"%data
        mm.write(struct.pack('I', data))

    def pci_set_value(self, resource, val, offset):
        fd = os.open(resource, os.O_RDWR)
        mm = mmap(fd, 0)
        val = self.pci_mem_write(mm, offset, val)
        mm.close()
        os.close(fd)
        return val

    def pci_get_value(self, resource, offset):
        fd = os.open(resource, os.O_RDWR)
        mm = mmap(fd, 0)
        val = self.pci_mem_read(mm, offset)
        mm.close()
        os.close(fd)
        return val

    def init_global_port_presence(self):
        for port_num in range(self.port_start, (self.port_end + 1)):
            presence = self.get_presence(port_num)
            if(presence):
                self._global_port_pres_dict[port_num] = '1'
            else:
                self._global_port_pres_dict[port_num] = '0'

    def mod_pres(self):
        port_pres_mask =0
        for port_num in range(self.port_start, (self.port_end + 1)):
            presence = self.get_presence(port_num)
            if(presence):
                self._global_port_pres_dict[port_num] = '1'
                port_val = (1 << (port_num -1))
                port_pres_mask = (port_pres_mask | port_val)
            else:
                self._global_port_pres_dict[port_num] = '0'
                port_val = ~(1 << (port_num -1))
                port_pres_mask = (port_pres_mask & port_val)

        return port_pres_mask


    def __init__(self):
        eeprom_path = "/sys/class/i2c-adapter/i2c-{0}/{0}-0050/eeprom"

        for x in range(self.port_start, self.port_end + 1):
            self.port_to_eeprom_mapping[x] = eeprom_path.format(x+1)
        self.init_global_port_presence()
        SfpUtilBase.__init__(self)

    def get_presence(self, port_num):

    # Check for invalid self.index
        mask = {'QSFP' : (1 << 4), 'SFP' : (1 << 0), 'QSFP_DD' : (1 << 4)}
        # Port offset starts with 0x4004
        if self.index in range(PORT_START,PORT_6):
            port_offset = BASE_OFFSET + ((self.index-INDEX_1) * HEX_10)
        elif self.index in range(PORT_6,PORT_54):
            port_offset = BASE_OFFSET + ((self.index-INDEX_2) * HEX_20)
        elif self.index in range(PORT_54,PORT_END+1):
            port_offset = BASE_OFFSET + ((self.index+INDEX_3) * HEX_10)
        else:
            sys.stderr.write("Port index {} out of range (1-{})\n".format(self.index,PORT_END))

        status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
        reg_value = int(status)
        # ModPrsL is active low
        if (self.sfp_type in mask) and (reg_value & mask[self.sfp_type] == 0):
            return True
        return False

    def get_low_power_mode(self, port_num):

        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        # Port offset starts with 0x4000
        if port_num in range(PORT_START,PORT_6):
            port_offset = BASE_OFFSET_2 + ((port_num-INDEX_1) * HEX_10)
        elif port_num in range(PORT_6,PORT_54):
            port_offset = BASE_OFFSET_2 + ((port_num-INDEX_2) * HEX_20)
        else:
            port_offset = BASE_OFFSET_2 + ((port_num+INDEX_3) * HEX_10)

        status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
        reg_value = int(status)

        # Absence of status throws error
        if (reg_value == ""):
            return False

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

        # Port offset starts with 0x4000
        if port_num in range(PORT_START,PORT_6):
            port_offset = BASE_OFFSET_2 + ((port_num-INDEX_1) * HEX_10)
        elif port_num in range(PORT_6,PORT_54):
            port_offset = BASE_OFFSET_2 + ((port_num-INDEX_2) * HEX_20)
        else:
            port_offset = BASE_OFFSET_2 + ((port_num+INDEX_3) * HEX_10)


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
        if port_num in range(PORT_START,PORT_6):
            port_offset = BASE_OFFSET_2 + ((port_num-INDEX_1) * HEX_10)
        elif port_num in range(PORT_6,PORT_54):
            port_offset = BASE_OFFSET_2 + ((port_num-INDEX_2) * HEX_20)
        else:
            port_offset = BASE_OFFSET_2 + ((port_num+INDEX_3) * HEX_10)

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
