#!/usr/bin/env python

#############################################################################
# DELLEMC N3248TE
#
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
#
#############################################################################

try:
    import os
    import struct
    import mmap
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase

except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SFP_PORT_START = 49
SFP_PORT_END = 54

QSFP_INFO_OFFSET = 128
SFP_INFO_OFFSET = 0
QSFP_DD_PAGE0 = 0

SFP_TYPE_LIST = [
    '0x3' # SFP/SFP+/SFP28 and later
]
QSFP_TYPE_LIST = [
    '0xc', # QSFP
    '0xd', # QSFP+ or later
    '0x11'  # QSFP28 or later
]
QSFP_DD_TYPE_LIST = [
    '0x18' #QSFP_DD Type
]

class Sfp(SfpOptoeBase):
    """
    DELLEMC Platform-specific Sfp class
    """

    def __init__(self, index, sfp_type, eeprom_path):
        SfpOptoeBase.__init__(self)
        self.sfp_type = sfp_type
        self.index = index
        self.eeprom_path = eeprom_path

    def get_eeprom_path(self):
        return self.eeprom_path

    def get_name(self):
        return "SFP/SFP+/SFP28" if self.index < 53 else "QSFP+ or later"

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


    def _get_cpld_register(self, reg):
        reg_file = '/sys/devices/platform/dell-n3248te-cpld.0/' + reg
        try:
            with open(reg_file, 'r') as fd:
                rv = fd.read()
        except IOError : return 'ERR'
        return rv.strip('\r\n').lstrip(' ')

    def get_presence(self):
        """
        Retrieves the presence of the sfp
        Returns : True if sfp is present and false if it is absent
        """
        # Check for invalid port_num
        presence = False
        if not (self.index >= SFP_PORT_START and self.index <= SFP_PORT_END): return presence
        bit_mask = 1 << (self.index - SFP_PORT_START)
        try:
            sfp_mod_prs = self._get_cpld_register('sfp_modprs')
            if sfp_mod_prs == 'ERR' : return presence
            presence =  ((int(sfp_mod_prs, 16) & bit_mask) == 0)
        except Exception:
            pass
        return presence

    def get_reset_status(self):
        """
        Retrives the reset status of SFP
        """
        reset_status = False
        return reset_status

    def get_lpmode(self):
        """
        Retrieves the lpmode(low power mode) of this SFP
        """
        lpmode_state = False
        return lpmode_state

    def reset(self):
        """
        Reset the SFP and returns all user settings to their default state
        """
        return False

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode(low power mode) of this SFP
        """
        return True

    def get_status(self):
        """
        Retrieves the operational status of the device
        """
        reset = self.get_reset_status()
        return (not reset)

    def get_max_port_power(self):
        """
        Retrieves the maximumum power allowed on the port in watts
        """
        return 5.0 if self.sfp_type == 'QSFP' else 2.5

    def is_replaceable(self):
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
