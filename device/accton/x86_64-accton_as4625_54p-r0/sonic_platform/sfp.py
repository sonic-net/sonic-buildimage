#############################################################################
# Edgecore
#
# Sfp contains an implementation of SONiC Platform Base API and
# provides the sfp device status which are available in the platform
#
#############################################################################
import subprocess

try:
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_platform_base.sonic_sfp.sfputilhelper import SfpUtilHelper
    from sonic_py_common import device_info
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

#Edge-core definitions
CPLD1_I2C_PATH = "/sys/bus/i2c/devices/0-0064/"
I2C_EEPROM_PATH = '/sys/bus/i2c/devices/{0}-0050/eeprom'

OPTOE1_TYPE_LIST = [
    0x0D, # QSFP+ or later
    0x11  # QSFP28 or later
]
OPTOE2_TYPE_LIST = [
    0x03 # SFP/SFP+/SFP28 and later
]
OPTOE3_TYPE_LIST = [
    0x18, # QSFP-DD
    0x19, # OSFP
    0x1E  # QSFP+ or later with CMIS
]

class Sfp(SfpOptoeBase):
    """Platform-specific Sfp class"""
    HOST_CHK_CMD = ["which", "systemctl"]

    # Path to sysfs
    PLATFORM_ROOT_PATH = "/usr/share/sonic/device"
    PMON_HWSKU_PATH = "/usr/share/sonic/hwsku"

    SFP_PORT_START = 49
    SFP_PORT_END = 54

    _port_to_i2c_mapping = {
        49: [10],
        50: [11],
        51: [12],
        52: [13],
        53: [14],
        54: [15]
    }

    def __init__(self, sfp_index=1, sfp_name=None):
        SfpOptoeBase.__init__(self)
        self.index = sfp_index
        self.port_num = self.index
        self._api_helper = APIHelper()
        self._name = sfp_name

        if self.SFP_PORT_START <= self.port_num <= self.SFP_PORT_END:
            self.eeprom_path = I2C_EEPROM_PATH.format(self._port_to_i2c_mapping[self.port_num][0])
            SfpOptoeBase.__init__(self)
            self.refresh()

    def get_eeprom_path(self):
        return self.eeprom_path

    def __is_host(self):
        return subprocess.call(self.HOST_CHK_CMD) == 0

    def reset(self):
        """
        Reset SFP and return all user module settings to their default srate.
        Returns:
            A boolean, True if successful, False if not
        """
        return False # SFP port doesn't support this feature

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP
        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        return False # SFP port doesn't support this feature

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        sfputil_helper = SfpUtilHelper()
        port_config_file_path = device_info.get_path_to_port_config_file()
        sfputil_helper.read_porttab_mappings(port_config_file_path)
        name = sfputil_helper.logical[self.index-1] or "Unknown"
        return name

    def get_presence(self):
        """
        Retrieves the presence of the device
        Returns:
            bool: True if device is present, False if not
        """
        if self.port_num < self.SFP_PORT_START:
            return False

        present_path = "{}{}{}".format(CPLD1_I2C_PATH, '/module_present_', self.port_num)
        val = self._api_helper.read_txt_file(present_path)
        if val is not None:
            return int(val, 10) == 1
        else:
            return False

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_presence()

    def refresh(self):
        self.refresh_xcvr_api()
        return True

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP
        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        # SFP doesn't support this feature
        return False

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode (low power mode) of SFP
        Args:
            lpmode: A Boolean, True to enable lpmode, False to disable it
            Note  : lpmode can be overridden by set_power_override
        Returns:
            A boolean, True if lpmode is set successfully, False if not
        """
        # SFP doesn't support this feature
        return False

    def get_power_override(self):
        """
        Retrieves the power-override status of this SFP
        Returns:
            A Boolean, True if power-override is enabled, False if disabled
        """
        # SFP doesn't support this feature
        return False

    def get_position_in_parent(self):
        """Retrieves 1-based relative physical position in parent device."""
        return self.port_num

    def is_replaceable(self):
        """
        Retrieves if replaceable
        Returns:
            A boolean value, True if replaceable
        """
        return True

    def validate_eeprom_sfp(self):
        checksum_test = 0
        eeprom_raw = self.read_eeprom(0, 96)
        if eeprom_raw is None:
            return None

        for i in range(0, 63):
            checksum_test = (checksum_test + eeprom_raw[i]) & 0xFF
        if checksum_test != eeprom_raw[63]:
            return False

        checksum_test = 0
        for i in range(64, 95):
            checksum_test = (checksum_test + eeprom_raw[i]) & 0xFF
        if checksum_test != eeprom_raw[95]:
            return False

        api = self.get_xcvr_api()
        if api is None:
            return False

        if api.is_flat_memory():
            return True

        checksum_test = 0
        eeprom_raw = self.read_eeprom(384, 96)
        if eeprom_raw is None:
            return None

        for i in range(0, 95):
            checksum_test = (checksum_test + eeprom_raw[i]) & 0xFF
        if checksum_test != eeprom_raw[95]:
            return False

        return True

    def validate_eeprom(self):
        id_byte_raw = self.read_eeprom(0, 1)
        if id_byte_raw is None:
            return None

        return self.validate_eeprom_sfp()

    def validate_temperature(self):
        temperature = self.get_temperature()
        if temperature is None:
            return None

        threshold_dict = self.get_transceiver_threshold_info()
        if threshold_dict is None:
            return None

        if isinstance(temperature, float) is not True:
            return True

        if isinstance(threshold_dict['temphighalarm'], float) is not True:
            return True

        return threshold_dict['temphighalarm'] > temperature

    def update_sfp_type(self):
        """
        The sfp type would not change.
        Don't need to update sfp type
        """
        pass

    def __get_error_description(self):
        if not self.get_presence():
            return self.SFP_STATUS_UNPLUGGED

        err_stat = self.SFP_STATUS_BIT_INSERTED

        status = self.validate_eeprom()
        if status is not True:
            err_state |= self.SFP_ERROR_BIT_BAD_EEPROM

        status = self.validate_temperature()
        if status is not True:
            err_state |= self.SFP_ERROR_BIT_HIGH_TEMP

        if err_stat is self.SFP_STATUS_BIT_INSERTED:
            return self.SFP_STATUS_OK
        else:
            err_desc = str()
            for key in self.SFP_ERROR_BIT_TO_DESCRIPTION_DICT:
                if (err_stat & key) != 0:
                    if len(err_desc) > 0:
                        err_desc += '|'
                    err_desc += self.SFP_ERROR_BIT_TO_DESCRIPTION_DICT[key]

            return err_desc

    def get_error_description(self):
        """
        Retrives the error descriptions of the SFP module
        Returns:
            String that represents the current error descriptions of vendor specific errors
            In case there are multiple errors, they should be joined by '|',
            like: "Bad EEPROM|Unsupported cable"
        """
        if self.port_num < 49:
            # RJ45 doesn't support this feature
            return "Not implemented"
        else:
            api = self.get_xcvr_api()
            if api is not None:
                try:
                    return api.get_error_description()
                except NotImplementedError:
                    return self.__get_error_description()
            else:
                return self.__get_error_description()
