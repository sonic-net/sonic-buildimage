#!/usr/bin/env python

#############################################################################
# DELLEMC
#
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
#
#############################################################################

try:
    import os
    import time
    import struct
    import mmap
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase

except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SFP_TYPE_LIST = [
    '0x3' # SFP/SFP+/SFP28 and later
]

QSFP_TYPE_LIST = [
    '0xc', # QSFP
    '0xd', # QSFP+ or later
    '0x11'  # QSFP28 or later
]

QSFP_DD_TYPE_LIST = [
   '0x18' #QSFP-DD Type
]

class Sfp(SfpOptoeBase):
    """
    DELLEMC Platform-specific Sfp class
    """
    BASE_RES_PATH = "/sys/bus/pci/devices/0000:04:00.0/resource0"
    OIR_FD_PATH = "/sys/bus/pci/devices/0000:04:00.0/port_msi"

    def __init__(self, index, sfp_type, eeprom_path):
        SfpOptoeBase.__init__(self)
        self.sfp_type = sfp_type
        self.index = index
        self.eeprom_path = eeprom_path
        self._initialize_media(delay=False)

    def get_eeprom_path(self):
        return self.eeprom_path

    def get_name(self):
        return "QSFP28 or later" if self.index < 65 else "SFP/SFP+/SFP28"

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
        mm = mmap.mmap(fd, 0)
        val = self.pci_mem_write(mm, offset, val)
        mm.close()
        os.close(fd)
        return val

    def pci_get_value(self, resource, offset):
        fd = os.open(resource, os.O_RDWR)
        mm = mmap.mmap(fd, 0)
        val = self.pci_mem_read(mm, offset)
        mm.close()
        os.close(fd)
        return val

    def _initialize_media(self,delay=False):
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
        mask = {'QSFP' : (1 << 4), 'SFP' : (1 << 0)}
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
        if (self.port_type == 'QSFP'):
            # Port offset starts with 0x4000
            port_offset = 16384 + ((self.index-1) * 16)

            status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
            reg_value = int(status)

            # Absence of status throws error
            if (reg_value == ""):
                return reset_status

            # Mask off 4th bit for reset status
            mask = (1 << 4)

            if ((reg_value & mask) == 0):
                reset_status = True
            else:
                reset_status = False

        return reset_status

    def get_lpmode(self):
        """
        Retrieves the lpmode(low power mode) of this SFP
        """
        lpmode_state = False
        if (self.port_type == 'QSFP'):

            # Port offset starts with 0x4000
            port_offset = 16384 + ((self.index-1) * 16)

            status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
            reg_value = int(status)

            # Absence of status throws error
            if (reg_value == ""):
                return lpmode_state

            # Mask off 6th bit for lpmode
            mask = (1 << 6)

            # LPMode is active high
            if reg_value & mask == 0:
                lpmode_state = False
            else:
                lpmode_state = True

        return lpmode_state

    def reset(self):
        """
        Reset the SFP and returns all user settings to their default state
        """
        if (self.port_type == 'QSFP'):
            # Port offset starts with 0x4000
            port_offset = 16384 + ((self.index-1) * 16)

            status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
            reg_value = int(status)

            # Absence of status throws error
            if (reg_value == ""):
                return False

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

            return True

        else:
            return False

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode(low power mode) of this SFP
        """
        if (self.port_type == 'QSFP'):
            # Port offset starts with 0x4000
            port_offset = 16384 + ((self.index-1) * 16)

            status = self.pci_get_value(self.BASE_RES_PATH, port_offset)
            reg_value = int(status)

            # Absence of status throws error
            if (reg_value == ""):
                return False

            # Mask off 6th bit for lowpower mode
            mask = (1 << 6)

            # LPMode is active high; set or clear the bit accordingly
            if lpmode is True:
                reg_value = reg_value | mask
            else:
                reg_value = reg_value & ~mask

            # Convert our register value back to a hex string and write back
            self.pci_set_value(self.BASE_RES_PATH, reg_value, port_offset)

            return True

        else:
            return False

    def get_status(self):
        """
        Retrieves the operational status of the device
        """
        reset = self.get_reset_status()

        if (reset is True):
            status = False
        else:
            status = True

        return status

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
        del_sfp_path = "/sys/bus/i2c/devices/i2c-{0}/delete_device".format(self.index+1)
        new_sfp_path = "/sys/bus/i2c/devices/i2c-{0}/new_device".format(self.index+1)
        driver_path = "/sys/bus/i2c/devices/i2c-{0}/{0}-0050/name".format(self.index+1)

        if not os.path.isfile(driver_path):
            print(driver_path, "does not exist")
            return False

        try:
            with os.fdopen(os.open(driver_path, os.O_RDONLY)) as fd:
                driver_name = fd.read()
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

        except IOError as e:
            print("Error: Unable to open file: %s" % str(e))
            return False

        return True
