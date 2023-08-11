#!/usr/bin/env python

#############################################################################
# DELLEMC E3224F
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

SFP_PORT_START = 1
SFP_PORT_END = 24
SFPPLUS_PORT_START = 25
SFPPLUS_PORT_END = 28
PORT_END = 30

QSFP_INFO_OFFSET = 128
SFP_INFO_OFFSET = 0
QSFP_DD_PAGE0 = 0

SFP_TYPE_LIST = [
    '0x3' # SFP/SFP+/SFP28 and later
]
QSFP_TYPE_LIST = [
    '0xc', # QSFP
    '0xd', # QSFP+ or later
    '0x11' # QSFP28 or later
]
QSFP_DD_TYPE_LIST = [
    '0x18' #QSFP_DD Type
]

class Sfp(SfpOptoeBase):
    """
    DELLEMC Platform-specific Sfp class
    """

    _port_to_i2c_mapping = {
            1: 27,
            2: 28,
            3: 29,
            4: 30,
            5: 31,
            6: 32,
            7: 33,
            8: 34,
            9: 35,
            10: 36,
            11: 37,
            12: 38,
            13: 39,
            14: 40,
            15: 41,
            16: 42,
            17: 43,
            18: 44,
            19: 45,
            20: 46,
            21: 47,
            22: 48,
            23: 49,
            24: 50,
            25: 20,
            26: 21,
            27: 22,
            28: 23,
            29: 24,
            30: 25,
            }

    def __init__(self, index, sfp_type, eeprom_path):
        SfpOptoeBase.__init__(self)
        self.sfp_type = sfp_type
        self.port_type = sfp_type
        self.index = index
        self.eeprom_path = eeprom_path
        self._initialize_media(delay=False)

    def get_eeprom_path(self):
        return self.eeprom_path

    def get_name(self):
        if not (self.index >= SFP_PORT_START and self.index <= PORT_END):
            return "N/A"
        if self.index <= SFP_PORT_END:
            return "SFP8"
        elif self.index <= SFPPLUS_PORT_END:
            return "SFP/SFP+/SFP28"
        else:
            return "QSFP or later"

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
        del_sfp_path = "/sys/class/i2c-adapter/i2c-{0}/delete_device".format(self._port_to_i2c_mapping[self.index])
        new_sfp_path =    "/sys/class/i2c-adapter/i2c-{0}/new_device".format(self._port_to_i2c_mapping[self.index])
        driver_path  = "/sys/class/i2c-adapter/i2c-{0}/{0}-0050/name".format(self._port_to_i2c_mapping[self.index])

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

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return self.index

    def _get_cpld_register(self, reg):
        reg_file = '/sys/devices/platform/dell-e3224f-cpld.0/' + reg
        try:
            with open(reg_file, 'r') as fd:
                rv = fd.read()
        except IOError : return 'ERR'
        return rv.strip('\r\n').lstrip(' ')

    def _set_cpld_register(self, reg_name, value):
        # On successful write, returns the value will be written on
        # reg_name and on failure returns 'ERR'

        cpld_dir = "/sys/devices/platform/dell-e3224f-cpld.0/"
        cpld_reg_file = cpld_dir + '/' + reg_name

        try:
           with open(cpld_reg_file, 'w') as fd:
                rv = fd.write(str(value))
        except Exception:
            rv = 'ERR'

        return rv

    def get_presence(self):
        """
        Retrieves the presence of the sfp
        Returns : True if sfp is present and false if it is absent
        """
        # Check for invalid port_num
        presence = False
        if not (self.index >= SFP_PORT_START and self.index <= PORT_END):
            return presence
        try:
            if self.index <= SFP_PORT_END:
                bit_mask = 1 << (self.index - SFP_PORT_START)
                sfp_mod_prs = self._get_cpld_register('sfp_modprs')
                if sfp_mod_prs == 'ERR':
                    return presence
                presence = ((int(sfp_mod_prs, 16) & bit_mask) == 0)
            elif self.index <= SFPPLUS_PORT_END:
                bit_mask = 1 << (self.index - SFPPLUS_PORT_START)
                sfpplus_mod_prs = self._get_cpld_register('sfpplus_modprs')
                if sfpplus_mod_prs == 'ERR':
                    return presence
                presence = ((int(sfpplus_mod_prs, 16) & bit_mask) == 0)
            else:
                bit_mask = (1 << (self.index - (SFPPLUS_PORT_START+4)))
                qsfp_mod_prs = self._get_cpld_register('qsfp_modprs')
                if qsfp_mod_prs == 'ERR':
                    return presence
                presence = ((int(qsfp_mod_prs, 16) & bit_mask) == 0)
        except TypeError:
            pass
        return presence

    def tx_disable(self, tx_disable):
        """
        Enable/Disable the TX disable bit of the optics.
        """
        rval = False
        if not (self.index >= SFP_PORT_START and self.index <= SFPPLUS_PORT_END):
            return rval
        if self.sfp_type == 'SFP':
            if self.index <= SFP_PORT_END:
                sfp_txdis = int(self._get_cpld_register('sfp_txdis'), 16)
                if sfp_txdis != 'ERR':
                    bit_mask = 1 << (self.index - SFP_PORT_START)
                    sfp_txdis = sfp_txdis | bit_mask if tx_disable \
                            else sfp_txdis & ~bit_mask
                    rval = (self._set_cpld_register('sfp_txdis', sfp_txdis) != 'ERR')
            elif self.index <= SFPPLUS_PORT_END:
                sfpplus_txdis = int(self._get_cpld_register('sfpplus_txdis'), 16)
                if sfpplus_txdis != 'ERR':
                    bit_mask = 1 << (self.index - SFPPLUS_PORT_START)
                    sfpplus_txdis = sfpplus_txdis | bit_mask if tx_disable \
                            else sfpplus_txdis & ~bit_mask
                    rval = (self._set_cpld_register('sfpplus_txdis', sfpplus_txdis) != 'ERR')
        return rval

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
