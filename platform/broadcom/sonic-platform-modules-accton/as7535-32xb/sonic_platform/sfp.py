#############################################################################
# Edgecore
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
CPLD1_I2C_PATH = "/sys/bus/i2c/devices/12-0061/"

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
    PLATFORM = "x86_64-accton_as7535_32xb-r0"
    HWSKU = "Accton-AS7535-32XB"

    # Path to sysfs
    PLATFORM_ROOT_PATH = "/usr/share/sonic/device"
    PMON_HWSKU_PATH = "/usr/share/sonic/hwsku"

    _port_to_i2c_mapping = {
        1: [25],
        2: [26],
        3: [27],
        4: [28],
        5: [29],
        6: [30],
        7: [31],
        8: [32],
        9: [33],
        10: [34],
        11: [35],
        12: [36],
        13: [37],
        14: [38],
        15: [39],
        16: [40],
        17: [41],
        18: [42],
        19: [43],
        20: [44],
        21: [45],
        22: [46],
        23: [47],
        24: [48],
        25: [49],
        26: [50],
        27: [23],
        28: [21],
        29: [24],
        30: [22],
    }

    def __init__(self, sfp_index=0, sfp_name=None):
        self._index = sfp_index
        self.port_num = self._index + 1
        self._api_helper = APIHelper()
        self._name = sfp_name

        self.eeprom_path = I2C_EEPROM_PATH.format(self._port_to_i2c_mapping[self.port_num][0])
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

        if 27 <= self.port_num <= 30:
            reset_path = "{}{}{}".format(CPLD1_I2C_PATH, '/module_reset_', self.port_num)
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

        if not self.get_presence():
            return False

        if 27 <= self.port_num <= 30:
            reset_path = "{}{}{}".format(CPLD1_I2C_PATH, '/module_reset_', self.port_num)
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
        if 1 <= self.port_num <= 30:
            present_path = "{}{}{}".format(CPLD1_I2C_PATH, '/module_present_', self.port_num)
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

    def validate_eeprom_qsfp(self):
        checksum_test = 0
        eeprom_raw = self.read_eeprom(128, 96)
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

        return True

    def validate_eeprom_cmis(self):
        checksum_test = 0
        eeprom_raw = self.read_eeprom(128, 95)
        if eeprom_raw is None:
            return None

        for i in range(0, 94):
            checksum_test = (checksum_test + eeprom_raw[i]) & 0xFF
        if checksum_test != eeprom_raw[94]:
            return False

        api = self.get_xcvr_api()
        if api is None:
            return False

        if api.is_flat_memory():
            return True

        checksum_test = 0
        eeprom_raw = self.read_eeprom(258, 126)
        if eeprom_raw is None:
            return None

        for i in range(0, 125):
            checksum_test = (checksum_test + eeprom_raw[i]) & 0xFF
        if checksum_test != eeprom_raw[125]:
            return False

        checksum_test = 0
        eeprom_raw = self.read_eeprom(384, 128)
        if eeprom_raw is None:
            return None

        for i in range(0, 127):
            checksum_test = (checksum_test + eeprom_raw[i]) & 0xFF
        if checksum_test != eeprom_raw[127]:
            return False

        checksum_test = 0
        eeprom_raw = self.read_eeprom(640, 128)
        if eeprom_raw is None:
            return None

        for i in range(0, 127):
            checksum_test = (checksum_test + eeprom_raw[i]) & 0xFF
        if checksum_test != eeprom_raw[127]:
            return False

        return True

    def validate_eeprom(self):
        id_byte_raw = self.read_eeprom(0, 1)
        if id_byte_raw is None:
            return None

        id = id_byte_raw[0]
        if id in OPTOE1_TYPE_LIST:
            return self.validate_eeprom_qsfp()
        elif id in OPTOE2_TYPE_LIST:
            return self.validate_eeprom_sfp()
        elif id in OPTOE3_TYPE_LIST:
            return self.validate_eeprom_cmis()
        else:
            return False

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
