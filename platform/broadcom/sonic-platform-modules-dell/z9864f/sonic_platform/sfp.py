#!/usr/bin/env python
"""
#############################################################################
# DELLEMC Z9864F
#
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
#
#############################################################################
"""

try:
    import os
    import sys
    import smbus2
    import time
    import mmap
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_platform.hwaccess import get_fpga_buspath

except ImportError as err:
    raise ImportError(str(err) + "- required module not found")

QSFP_INFO_OFFSET = 128
SFP_INFO_OFFSET = 0
QSFP_DD_PAGE0 = 0

SFP_TYPE_LIST = [
    '0x3' # SFP/SFP+/SFP28 and later
]
QSFP_TYPE_LIST = [
    '0x0c', # QSFP
    '0x0d', # QSFP+ or later
    '0x11'  # QSFP28 or later
]
QSFP_DD_TYPE_LIST = [
    '0x18' #QSFP_DD Type
]

OSFP_TYPE_LIST = [
    '0x19' # OSFP 8X Type
]

PORT_CPLD_I2C_BUS = 600

PORT_CPLD_A_ADDR = 0x41
PORT_PRES_01_08_OFFSET = 0x10
PORT_PRES_09_16_OFFSET = 0x11
PORT_PRES_33_40_OFFSET = 0x12
PORT_PRES_41_48_OFFSET = 0x13

PORT_CPLD_B_ADDR = 0x45
PORT_PRES_17_24_OFFSET = 0x10
PORT_PRES_25_32_OFFSET = 0x11
PORT_PRES_49_56_OFFSET = 0x12
PORT_PRES_57_64_OFFSET = 0x13

PORT_PRES_01_08_VAL = 0xff
PORT_PRES_09_16_VAL = 0xff
PORT_PRES_17_24_VAL = 0xff
PORT_PRES_25_32_VAL = 0xff
PORT_PRES_33_40_VAL = 0xff
PORT_PRES_41_48_VAL = 0xff
PORT_PRES_49_56_VAL = 0xff
PORT_PRES_57_64_VAL = 0xff

PORT_PRES_65_66_VAL = 0xff
PORT_PRES_65_66_OFFSET = 0x18

PORT_START = 1
PORT_END = 66
AUX_PORT_START = 65

'''
holds CPLD locations for LPmode configuration. each item is range of ports address is for,
the other two are cpld address and offset to the byte for the port
'''
LPMODE_MAP = [
    (range(1, 9), PORT_CPLD_A_ADDR, 0x44),
    (range(9, 17), PORT_CPLD_A_ADDR, 0x45),
    (range(17, 25), PORT_CPLD_B_ADDR, 0x44),
    (range(25, 33), PORT_CPLD_B_ADDR, 0x45),
    (range(33, 41), PORT_CPLD_A_ADDR, 0x46),
    (range(41, 49), PORT_CPLD_A_ADDR, 0x47),
    (range(49, 57), PORT_CPLD_B_ADDR, 0x46),
    (range(57, AUX_PORT_START), PORT_CPLD_B_ADDR, 0x47),
]

_port_to_i2c_mapping = {
    1:  2,
    2:  3,
    3:  4,
    4:  5,
    5:  6,
    6:  7,
    7:  8,
    8:  9,
    9:  10,
    10: 11,
    11: 12,
    12: 13,
    13: 14,
    14: 15,
    15: 16,
    16: 17,
    17: 18,
    18: 19,
    19: 20,
    20: 21,
    21: 22,
    22: 23,
    23: 24,
    24: 25,
    25: 26,
    26: 27,
    27: 28,
    28: 29,
    29: 30,
    30: 31,
    31: 32,
    32: 33,
    33: 35,
    34: 34,
    35: 37,
    36: 36,
    37: 39,
    38: 38,
    39: 41,
    40: 40,
    41: 43,
    42: 42,
    43: 45,
    44: 44,
    45: 47,
    46: 46,
    47: 49,
    48: 48,
    49: 51,
    50: 50,
    51: 53,
    52: 52,
    53: 55,
    54: 54,
    55: 57,
    56: 56,
    57: 59,
    58: 58,
    59: 61,
    60: 60,
    61: 63,
    62: 62,
    63: 65,
    64: 64,
    65: 66,
    66: 67,
 }



class Sfp(SfpOptoeBase):
    """
    DELLEMC Platform-specific Sfp class
    """
    BASE_RES_PATH = get_fpga_buspath()

    def __init__(self, index, sfp_type, eeprom_path):
        """
        SFP Dunder init
        """
        SfpOptoeBase.__init__(self)
        self.index = index
        self.eeprom_path = eeprom_path
        #port_type is the native port type and sfp_type is the transceiver type
        #sfp_type will be detected in get_transceiver_info
        self.port_type = sfp_type
        self.sfp_type = self.port_type
        self.bus = smbus2.SMBus(PORT_CPLD_I2C_BUS)
        self._initialize_media(delay=False)

    def get_eeprom_path(self):
        """
        Returns SFP eeprom path
        """
        return self.eeprom_path

    def get_name(self):
        """
        Returns native transceiver type
        """
        return "QSFP-DD Double Density 8X Pluggable Transceiver" if self.index < 65 else "SFP/SFP+/SFP28"

    @staticmethod
    def pci_mem_read(mem, offset):
        """
        Returns the desired byte in PCI memory space
        """
        mem.seek(offset)
        return mem.read_byte()


    @staticmethod
    def pci_mem_write(mem, offset, data):
        """
        Writes the desired byte in PCI memory space
        """
        mem.seek(offset)
        # print "data to write:%x"%data
        mem.write_byte(data)

    def pci_set_value(self, resource, val, offset):
        """
        Sets the value in PCI memory space
        """
        filed = os.open(resource, os.O_RDWR)
        mem = mmap.mmap(filed, 0)
        self.pci_mem_write(mem, offset, val)
        mem.close()
        os.close(filed)
        return val

    def pci_get_value(self, resource, offset):
        """
        Retrieves the value from PCI memory space
        """
        filed = os.open(resource, os.O_RDWR)
        mem = mmap.mmap(filed, 0)
        val = self.pci_mem_read(mem, offset)
        mem.close()
        os.close(filed)
        return val

    def _initialize_media(self, delay=False):
        """
        Initialize the media type and eeprom driver for SFP
        """
        if delay:
            time.sleep(1)
            self._xcvr_api = None
            self.get_xcvr_api()

        self.set_media_type()
        self.reinit_sfp_driver()

    def get_presence(self):

        try:
            port_offset = 0xff
            mask = 0
            if self.index in range(PORT_START, 9):
                port_offset = self.bus.read_byte_data(PORT_CPLD_A_ADDR, PORT_PRES_01_08_OFFSET)
            elif self.index in range(9, 17):
                port_offset = self.bus.read_byte_data(PORT_CPLD_A_ADDR, PORT_PRES_09_16_OFFSET)
            elif self.index in range(17, 25):
                port_offset = self.bus.read_byte_data(PORT_CPLD_B_ADDR, PORT_PRES_17_24_OFFSET)
            elif self.index in range(25, 33):
                port_offset = self.bus.read_byte_data(PORT_CPLD_B_ADDR, PORT_PRES_25_32_OFFSET)
            elif self.index in range(33, 41):
                port_offset = self.bus.read_byte_data(PORT_CPLD_A_ADDR, PORT_PRES_33_40_OFFSET)
            elif self.index in range(41, 49):
                port_offset = self.bus.read_byte_data(PORT_CPLD_A_ADDR, PORT_PRES_41_48_OFFSET)
            elif self.index in range(49, 57):
                port_offset = self.bus.read_byte_data(PORT_CPLD_B_ADDR, PORT_PRES_49_56_OFFSET)
            elif self.index in range(57, AUX_PORT_START):
                port_offset = self.bus.read_byte_data(PORT_CPLD_B_ADDR, PORT_PRES_57_64_OFFSET)
            elif self.index in range(AUX_PORT_START, PORT_END+1):
                port_offset = self.bus.read_byte_data(PORT_CPLD_A_ADDR, PORT_PRES_65_66_OFFSET)
            else:
                sys.stderr.write("Port index {} out of range (1-{})\n".format(self.index, PORT_END))
                return False

            reg_value = port_offset
            if self.index in range(PORT_START, AUX_PORT_START):
                mask = (1 << ((self.index - 1) % 8))
            elif self.index in range(AUX_PORT_START, PORT_END+1):
                if self.index == AUX_PORT_START:
                    mask = (1 << 0)
                else:
                    mask = (1 << 4)
            else:
                sys.stderr.write("Port index {} out of range (1-{})\n".format(self.index, PORT_END))
                return False

            if (reg_value & mask) == 0:
                return True

            return False
        except Exception as e:
            sys.stderr.write(f"Error reading port presence: {e}\n")
            return False

    def get_reset_status(self):
        """
        Retrives the reset status of SFP
        """
        reset_status = False
        try:
            if self.port_type == 'QSFP_DD':
                # Port offset starts with 0x4000
                port_offset = 16384 + ((self.index-1) * 16)

                status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
                reg_value = int(status)

                # Mask off 4th bit for reset status
                mask = (1 << 4)
                reset_status = not (reg_value & mask)
        except ValueError:
            pass

        return reset_status

    def reset(self):
        """
        Reset the SFP and returns all user settings to their default state
        """
        try:
            if self.port_type == 'QSFP_DD':
                # Port offset starts with 0x4000
                port_offset = 16384 + ((self.index-1) * 16)

                status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
                reg_value = int(status)

                # Mask off 4th bit for reset
                mask = (1 << 4)

                # ResetL is active low
                reg_value = reg_value & ~mask

                # Convert our register value back to a hex string and write back
                self.pci_set_value(self.BASE_RES_PATH, reg_value, port_offset)

                # Sleep 1 second to allow it to settle
                time.sleep(1)

                reg_value = reg_value | mask

                # Convert our register value back to a hex string and write back
                self.pci_set_value(self.BASE_RES_PATH, reg_value, port_offset)
            else:
                return False
        except ValueError:
            return False
        return True


    def get_intl_state(self):
        """ Sets the intL (interrupt; active low) pin of this SFP """
        intl_state = True
        try:
            if self.sfp_type in ['QSFP_DD', 'OSFP']:
                # Port offset starts with 0x4004
                port_offset = 16388 + ((self.index-1) * 16)

                status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
                reg_value = int(status)

                # Mask off 4th bit for intL
                mask = (1 << 4)

                intl_state = (reg_value & mask)
        except  ValueError:
            pass
        return intl_state

    def set_media_type(self):
        """
        Reads optic eeprom byte to determine media type inserted
        """
        eeprom_raw = []
        eeprom_raw = self._xcvr_api_factory._get_id()
        if eeprom_raw is not None:
            eeprom_raw = hex(eeprom_raw)
            if eeprom_raw in SFP_TYPE_LIST:
                self.sfp_type = 'SFP'
            elif eeprom_raw in QSFP_TYPE_LIST:
                self.sfp_type = 'QSFP'
            elif eeprom_raw in QSFP_DD_TYPE_LIST:
                self.sfp_type = 'QSFP_DD'
            elif eeprom_raw in OSFP_TYPE_LIST:
                self.sfp_type = 'OSFP'
            else:
                #Set native port type if EEPROM type is not recognized/readable
                self.sfp_type = self.port_type
        else:
            self.sfp_type = self.port_type

        return self.sfp_type

    def reinit_sfp_driver(self):
        """
        Changes the driver based on media type detected
        """

        i2c_bus = _port_to_i2c_mapping[self.index]
        del_sfp_path = "/sys/bus/i2c/devices/i2c-{0}/delete_device".format(i2c_bus)
        new_sfp_path = "/sys/bus/i2c/devices/i2c-{0}/new_device".format(i2c_bus)
        driver_path = "/sys/bus/i2c/devices/i2c-{0}/{0}-0050/name".format(i2c_bus)

        if not os.path.isfile(driver_path):
            print(driver_path, "does not exist")
            return False

        try:
            with os.fdopen(os.open(driver_path, os.O_RDONLY)) as filed:
                driver_name = filed.read()
                driver_name = driver_name.rstrip('\r\n')
                driver_name = driver_name.lstrip(" ")

            #Avoid re-initialization of the QSFP/SFP optic on QSFP/SFP port.
            if self.sfp_type == 'SFP' and driver_name in ['optoe1', 'optoe3']:
                with open(del_sfp_path, 'w') as f:
                    f.write('0x50\n')
                time.sleep(0.2)
                with open(new_sfp_path, 'w') as f:
                    f.write('optoe2 0x50\n')
                time.sleep(2)
            elif self.sfp_type == 'QSFP' and driver_name in ['optoe2', 'optoe3']:
                with open(del_sfp_path, 'w') as f:
                    f.write('0x50\n')
                time.sleep(0.2)
                with open(new_sfp_path, 'w') as f:
                    f.write('optoe1 0x50\n')
                time.sleep(2)
            elif self.sfp_type == 'QSFP_DD' and driver_name in ['optoe1', 'optoe2']:
                with open(del_sfp_path, 'w') as f:
                    f.write('0x50\n')
                time.sleep(0.2)
                with open(new_sfp_path, 'w') as f:
                    f.write('optoe3 0x50\n')
                time.sleep(2)

        except IOError as err:
            print("Error: Unable to open file: %s" %str(err))
            return False

        return True

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return self.index

    @staticmethod
    def is_replaceable():
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def get_error_description(self):
        """
        Retrives the error descriptions of the SFP module

        Returns:
            String that represents the current error descriptions of vendor specific errors
            In case there are multiple errors, they should be joined by '|',
            like: "Bad EEPROM|Unsupported cable"
        """
        if not self.get_presence():
            return self.SFP_STATUS_UNPLUGGED
        else:
            if not os.path.isfile(self.eeprom_path):
                return "EEPROM driver is not attached"

            if self.sfp_type == 'SFP':
                offset = SFP_INFO_OFFSET
            elif self.sfp_type == 'QSFP':
                offset = QSFP_INFO_OFFSET
            elif self.sfp_type == 'QSFP_DD':
                offset = QSFP_DD_PAGE0

            try:
                with open(self.eeprom_path, mode="rb", buffering=0) as eeprom:
                    eeprom.seek(offset)
                    eeprom.read(1)
            except OSError as e:
                return "EEPROM read failed ({})".format(e.strerror)

        return self.SFP_STATUS_OK

    def get_cpld_lpmode_location(self):
        '''
        Returns the location for this SFP's low power mode setting in CPLD registers.
        Returns:
            A tuple of (cpld_addr, register_offset, port_offset). the CLPD device address,
            the offset to the byte that contains this port, and the offset within that byte
        '''

        for port_range, cpld_addr, register_offset in LPMODE_MAP:
            if self.index in port_range:
                return cpld_addr, register_offset, (self.index - 1) % 8
        sys.stderr.write("Port index {} out of range (1-{})\n".format(self.index, AUX_PORT_START))
        return None, None, None

    def set_lpmode(self, lpmode):
        '''
        Sets low power mode bit in CPLD for this port's transceiver.
        :param bool lpmode: True to enable low power mode, False to disable
        Returns:
            A boolean, True if lpmode is set successfully, False if not
        '''
        cpld_addr, register_offset, port_offset = self.get_cpld_lpmode_location()
        if cpld_addr is None:
            return False
        try:
            # cpld and offset are found, and port index is validated in range
            existing_val = self.bus.read_byte_data(cpld_addr, register_offset)
            mask = (1 << port_offset)

            # 9864 specs, low power mode on means set to 0
            reg_value = (existing_val & ~mask) if lpmode else (existing_val | mask)

            self.bus.write_byte_data(cpld_addr, register_offset, reg_value)
            return True
        except IOError:
            sys.stderr.write("Error accessing I2C device for Port index {} )\n".format(self.index))
            return False

    def get_lpmode(self):
        '''
        Returns whether this SFP is in low power mode from CPLD. False if there's any exceptions.
        Returns:
            A boolean, True if lpmode is true, False if not
        '''
        try:
            cpld_addr, register_offset, port_offset = self.get_cpld_lpmode_location()
            if cpld_addr is None:
                return False

            existing_byte = self.bus.read_byte_data(cpld_addr, register_offset)
            mask = (1 << port_offset)

            # 9864 specs, low power mode on means set to 0
            if (existing_byte & mask) == 0:
                return True
            return False
        except Exception as e:
            sys.stderr.write(f"Error reading port presence: {e}\n")
            return False
