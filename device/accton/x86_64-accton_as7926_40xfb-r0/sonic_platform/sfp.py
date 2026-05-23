#############################################################################
# Accton
#
# Sfp contains an implementation of SONiC Platform Base API and
# provides the sfp device status which are available in the platform
#
#############################################################################
import time
import os

try:
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_platform_base.sonic_sfp.sfputilhelper import SfpUtilHelper
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

#Edge-core definitions
CPLD2_I2C_PATH = "/sys/bus/i2c/devices/12-0062/"
CPLD3_I2C_PATH = "/sys/bus/i2c/devices/13-0063/"
CPLD4_I2C_PATH = "/sys/bus/i2c/devices/20-0064/"

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
    HOST_CHK_CMD = "which systemctl > /dev/null 2>&1"
    PLATFORM = "x86_64-accton_as7926_40xfb-r0"
    HWSKU = "Accton-AS7926-40XFB"

    # Path to sysfs
    PLATFORM_ROOT_PATH = "/usr/share/sonic/device"
    PMON_HWSKU_PATH = "/usr/share/sonic/hwsku"

    CPLD2_PORT_START = 1
    CPLD2_PORT_END = 16
    CPLD3_PORT_START = 17
    CPLD3_PORT_END = 32

    _port_to_i2c_mapping = {
        1: [33],
        2: [34],
        3: [37],
        4: [38],
        5: [41],
        6: [42],
        7: [45],
        8: [46],
        9: [49],
        10: [50],
        11: [53],
        12: [54],
        13: [57],
        14: [58],
        15: [61],
        16: [62],
        17: [65],
        18: [66],
        19: [69],
        20: [70],
        21: [35],
        22: [36],
        23: [39],
        24: [40],
        25: [43],
        26: [44],
        27: [47],
        28: [48],
        29: [51],
        30: [52],
        31: [55],
        32: [56],
        33: [59],
        34: [60],
        35: [63],
        36: [64],
        37: [67],
        38: [68],
        39: [71],
        40: [72],
        41: [85],
        42: [76],
        43: [75],
        44: [74],
        45: [73],
        46: [78],
        47: [77],
        48: [80],
        49: [79],
        50: [82],
        51: [81],
        52: [84],
        53: [83],
        54: [30],
        55: [31],
    }

    def __init__(self, sfp_index=0, sfp_name=None):
        self._index = sfp_index
        self._port_num = self._index + 1
        self._api_helper = APIHelper()
        self._name = sfp_name

        self.eeprom_path = I2C_EEPROM_PATH.format(self._port_to_i2c_mapping[self._port_num][0])
        SfpOptoeBase.__init__(self)
        self.refresh_optoe_dev_class()

    def get_eeprom_path(self):
        return self.eeprom_path

    def __is_host(self):
        return os.system(self.HOST_CHK_CMD) == 0

    def __get_path_to_port_config_file(self):
        platform_path = "/".join([self.PLATFORM_ROOT_PATH, self.PLATFORM])
        hwsku_path = "/".join([platform_path, self.HWSKU]
                              ) if self.__is_host() else self.PMON_HWSKU_PATH
        return "/".join([hwsku_path, "port_config.ini"])

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP
        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        if not self.get_presence():
            return False

        if 1 <= self._port_num <= 10 or 21 <= self._port_num <= 30:
            reset_path = "{}{}{}".format(CPLD2_I2C_PATH, '/module_reset_', self._port_num)
        elif 11 <= self._port_num <= 20 or 31 <= self._port_num <= 40:
            reset_path = "{}{}{}".format(CPLD3_I2C_PATH, '/module_reset_', self._port_num)
        elif 41 <= self._port_num <= 53:
            reset_path = "{}{}{}".format(CPLD4_I2C_PATH, '/module_reset_', self._port_num)
        else:
            return False

        val = self._api_helper.read_txt_file(reset_path)
        if val is not None:
            return int(val, 10)==1
        else:
            return False

    def reset(self):
        """
        Reset SFP and return all user module settings to their default srate.
        Returns:
            A boolean, True if successful, False if not
        """
        # Check for invalid port_num

        if 54 <= self._port_num <= 55:
            return False # SFP doesn't support this feature

        if not self.get_presence():
            return False

        if 1 <= self._port_num <= 10 or 21 <= self._port_num <= 30:
            reset_path = "{}{}{}".format(CPLD2_I2C_PATH, '/module_reset_', self._port_num)
        elif 11 <= self._port_num <= 20 or 31 <= self._port_num <= 40:
            reset_path = "{}{}{}".format(CPLD3_I2C_PATH, '/module_reset_', self._port_num)
        elif 41 <= self._port_num <= 53:
            reset_path = "{}{}{}".format(CPLD4_I2C_PATH, '/module_reset_', self._port_num)
        else:
            return False

        ret = self._api_helper.write_txt_file(reset_path, 1) #sysfs 1: enable reset
        if ret is not True:
            return ret

        time.sleep(0.2)
        ret = self._api_helper.write_txt_file(reset_path, 0) #sysfs 0: disable reset
        time.sleep(0.2)

        return ret

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        sfputil_helper = SfpUtilHelper()
        sfputil_helper.read_porttab_mappings(
            self.__get_path_to_port_config_file())
        name = sfputil_helper.logical[self._index] or "Unknown"
        return name

    def get_presence(self):
        """
        Retrieves the presence of the device
        Returns:
            bool: True if device is present, False if not
        """
        if 1 <= self._port_num <= 10 or 21 <= self._port_num <= 30 or 54 <= self._port_num <= 55:
            present_path = "{}{}{}".format(CPLD2_I2C_PATH, '/module_present_', self._port_num)
        elif 11 <= self._port_num <= 20 or 31 <= self._port_num <= 40:
            present_path = "{}{}{}".format(CPLD3_I2C_PATH, '/module_present_', self._port_num)
        elif 41 <= self._port_num <= 53:
            present_path = "{}{}{}".format(CPLD4_I2C_PATH, '/module_present_', self._port_num)
        else:
            return False

        val=self._api_helper.read_txt_file(present_path)
        if val is not None:
            return int(val, 10)==1
        else:
            return False

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_presence() and not self.get_reset_status()

    def _get_id(self):
        id_byte_raw = self.read_eeprom(0, 1)
        if id_byte_raw is None:
            return None
        return id_byte_raw[0]

    def refresh_optoe_dev_class(self):
        id = self._get_id()
        if id is None:
            return None

        if id in OPTOE1_TYPE_LIST:
            dev_class_val = '1'
        elif id in OPTOE2_TYPE_LIST:
            dev_class_val = '2'
        elif id in OPTOE3_TYPE_LIST:
            dev_class_val = '3'
        else:
            return False

        dev_class_path = self.eeprom_path.replace('eeprom', 'dev_class')

        try:
            with open(dev_class_path, mode='r+') as sysfsfile:
                dev_class_old = sysfsfile.read()
                if dev_class_old == dev_class_val:
                    return True

                sysfsfile.seek(0)
                sysfsfile.write(dev_class_val)
        except IOError:
            return False

        self.refresh_xcvr_api()
        return True

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP
        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        if not self.get_presence():
            return False

        id = self._get_id()
        if id in OPTOE3_TYPE_LIST:
            return super().get_lpmode()

        api = self.get_xcvr_api()
        if api is None:
            return False

        if api.get_power_override_support() is False:
            return False

        if self.get_power_override() is True:
            return api.get_power_set()

        # High Power Mode if one of the following conditions is matched:
        # 1. "Power override" bit is 0
        # 2. "Power override" bit is 1 and "Power set" bit is 0
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
        if not self.get_presence():
            return False

        id = self._get_id()
        if id in OPTOE3_TYPE_LIST:
            return super().set_lpmode(lpmode)

        api = self.get_xcvr_api()
        if api is None:
            return False

        if api.get_power_override_support() is False:
            return False

        return self.set_power_override(True, lpmode)

    def get_position_in_parent(self):
        """Retrieves 1-based relative physical position in parent device."""
        return self._port_num

    def is_replaceable(self):
        """
        Retrieves if replaceable
        Returns:
            A boolean value, True if replaceable
        """
        return True
