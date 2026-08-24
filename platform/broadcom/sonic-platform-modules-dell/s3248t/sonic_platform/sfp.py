"""
    DELL S3248T

    Module contains an implementation of SONiC Platform Base API and
    provides the platform information

"""

try:
    from pathlib import Path
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase

except ImportError as err:
    raise ImportError(str(err) + "- required module not found")

SFP_PORT_START = 49
SFP_PORT_END = 52
PORT_END = 54

class Sfp(SfpOptoeBase):
    """
    DELL Platform-specific Sfp class
    """
    def __init__(self, index, sfp_type, eeprom_path):
        SfpOptoeBase.__init__(self)
        self.sfp_type = sfp_type
        self.index = index
        self.eeprom_path = eeprom_path

    def get_eeprom_path(self):
        """
        Returns media EEPROM path
        """
        return self.eeprom_path

    def get_name(self):
        """
        Returns media type
        """
        return "SFP/SFP+" if self.index < 53 else "QSFP+/QSFP28"

    @staticmethod
    def _get_cpld_register(reg):
        reg_file = '/sys/devices/platform/dell-s3248t-cpld.0/' + reg
        try:
            rval = Path(reg_file).read_text()
        except EnvironmentError:
            return 'ERR'
        return rval.strip('\r\n').lstrip(' ')

    @staticmethod
    def _set_cpld_register(reg_name, value):
        # On successful write, returns the value will be written on
        # reg_name and on failure returns 'ERR'

        cpld_dir = "/sys/devices/platform/dell-s3248t-cpld.0/"
        cpld_reg_file = cpld_dir + '/' + reg_name

        try:
            with open(cpld_reg_file, 'w') as filed:
                rval = filed.write(str(value))
        except EnvironmentError:
            rval = 'ERR'
        return rval

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
            else:
                bit_mask = (1 << (self.index - (SFP_PORT_START+4)))
                qsfp_mod_prs = self._get_cpld_register('qsfp_modprs')
                if qsfp_mod_prs == 'ERR':
                    return presence
                presence = ((int(qsfp_mod_prs, 16) & bit_mask) == 0)
        except TypeError:
            pass
        return presence

    @staticmethod
    def get_reset_status():
        """
        Retrives the reset status of SFP
        """
        return False

    @staticmethod
    def reset():
        """
        Reset the SFP and returns all user settings to their default state
        """
        return True

    @staticmethod
    def set_lpmode(lpmode):
        """
        Sets the lpmode(low power mode) of this SFP
        """
        return True

    def hard_tx_disable(self, tx_disable):
        """
        Disable SFP TX for all channels by pulling up hard TX_DISABLE pin

        Args:
        tx_disable : A Boolean, True to enable tx_disable mode, False to disable
        tx_disable mode.

        Returns:
        A boolean, True if tx_disable is set successfully, False if not
        """
        rval = False
        if self.sfp_type == 'SFP':
            sfp_txdis = int(self._get_cpld_register('sfp_txdis'), 16)
            if sfp_txdis != 'ERR':
                bit_mask = 1 << (self.index - SFP_PORT_START)
                sfp_txdis = sfp_txdis | bit_mask if tx_disable \
                            else sfp_txdis & ~bit_mask
                rval = (self._set_cpld_register('sfp_txdis', sfp_txdis) != 'ERR')
        return rval

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
