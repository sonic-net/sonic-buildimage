#!/usr/bin/env python
"""
#############################################################################
# DELLEMC S5448
#
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
#
#############################################################################
"""

try:
    import os
    import time
    import mmap
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase

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

class Sfp(SfpOptoeBase):
    """
    DELLEMC Platform-specific Sfp class
    """
    BASE_RES_PATH = "/sys/bus/pci/devices/0000:04:00.0/resource0"

    def __init__(self, index, sfp_type, eeprom_path):
        """
        SFP Dunder init
        """
        SfpOptoeBase.__init__(self)
        self.index = index
        self.eeprom_path = eeprom_path
        self.sfp_eeprom_path = eeprom_path
        #port_type is the native port type and sfp_type is the transceiver type
        #sfp_type will be detected in get_transceiver_info
        self.port_type = sfp_type
        self.sfp_type = self.port_type
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
        return "QSFP-DD Double Density 8X Pluggable Transceiver" if self.index < 33 else "SFP/SFP+/SFP28"

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
        """
        Retrieves the presence of the sfp
        Returns : True if sfp is present and false if it is absent
        """
        # Check for invalid port_num
        mask = {'QSFP_DD' : (1 << 4), 'SFP' : (1 << 0)}
        # Port offset starts with 0x4004
        port_offset = 16388 + ((self.index-1) * 16)

        try:
            status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
            reg_value = int(status)
            # ModPrsL is active low
            if reg_value & mask[self.port_type] == 0:
                return True
        except ValueError:
            pass

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

    def get_lpmode(self):
        """
        Retrieves the lpmode(low power mode) of this SFP
        """
        lpmode_state = False
        try:
            if self.sfp_type == 'QSFP_DD':
                return SfpOptoeBase.get_lpmode(self)
            else:
                # Port offset starts with 0x4000
                port_offset = 16384 + ((self.index-1) * 16)

                status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
                reg_value = int(status)

                # Mask off 6th bit for lpmode
                mask = (1 << 6)

                lpmode_state = (reg_value & mask)
        except ValueError:
            pass
        return bool(lpmode_state)

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

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode(low power mode) of this SFP
        """
        try:
            if self.sfp_type == 'QSFP_DD':
                return SfpOptoeBase.set_lpmode(self, lpmode)
            else:
                # Port offset starts with 0x4000
                port_offset = 16384 + ((self.index-1) * 16)

                status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
                reg_value = int(status)

                # Mask off 6th bit for lowpower mode
                mask = (1 << 6)

                # LPMode is active high; set or clear the bit accordingly
                if lpmode is True:
                    reg_value = reg_value | mask
                else:
                    reg_value = reg_value & ~mask

                # Convert our register value back to a hex string and write back
                self.pci_set_value(self.BASE_RES_PATH, reg_value, port_offset)
        except ValueError:
            return  False
        return True

    def get_intl_state(self):
        """
        Sets the intL (interrupt; active low) pin of this SFP
        """
        intl_state = True
        try:
            if self.port_type == 'QSFP_DD':
                # Port offset starts with 0x4004
                port_offset = 16388 + ((self.index-1) * 16)

                status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
                reg_value = int(status)

                # Mask off 4th bit for intL
                mask = (1 << 4)

                intl_state = (reg_value & mask)
        except ValueError:
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

        i2c_bus = self.sfp_eeprom_path[27:].split('/')[0]
        del_sfp_path = "/sys/class/i2c-adapter/i2c-{0}/delete_device".format(i2c_bus)
        new_sfp_path = "/sys/class/i2c-adapter/i2c-{0}/new_device".format(i2c_bus)
        driver_path = "/sys/class/i2c-adapter/i2c-{0}/{0}-0050/name".format(i2c_bus)

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
