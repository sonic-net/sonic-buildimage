#############################################################################
#
# Sfp contains an implementation of SONiC Platform Base API and
# provides the sfp device status which are available in the platform
#
#############################################################################

import os
import sys
import time
import struct
import subprocess
import shlex
from ctypes import create_string_buffer

try:
    from sonic_py_common import logger
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_platform_base.sonic_sfp.sfputilhelper import SfpUtilHelper
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SYSLOG_IDENTIFIER = "sfp"
logger = logger.Logger(SYSLOG_IDENTIFIER)
logger.set_min_log_priority_debug()
logger.log_debug("Load {} module".format(__name__))

QSFP_POWEROVERRIDE_OFFSET = 93
SFP_STATUS_INSERTED = '1'
SFP_STATUS_REMOVED = '0'

class Sfp(SfpOptoeBase):
    """Platform-specific Sfp class"""

    HOST_CHK_CMD = "docker > /dev/null 2>&1"

    EEPROM_PATH = '/sys/bus/i2c/devices/{0}-0050/eeprom'
    CPLD_I2C_PATH = "/sys/bus/i2c/devices/"
    PLATFORM_ROOT_PATH = "/usr/share/sonic/device"
    PMON_JSON_PATH = "/usr/share/sonic/platform"
    PMON_HWSKU_PATH = '/usr/share/sonic/hwsku'

    # Port number
    PORT_START = 1
    PORT_END = 16

    _cpld_mapping = {
        0:  "13-0061",
    }

    _port_to_i2c_mapping = {
        1:  18,
        2:  19,
        3:  20,
        4:  21,
        5:  22,
        6:  23,
        7:  24,
        8:  25,
        9:  26,
        10: 27,
        11: 28,
        12: 29,
        13: 30,
        14: 31,
        15: 32,
        16: 33,
    }

    _sfp_port = range(PORT_START, PORT_END + 1)


    def __init__(self, sfp_index=0):

        SfpOptoeBase.__init__(self)
        self._index = sfp_index
        self._port_num = self._index + 1
        self._api_helper = APIHelper()
        self.data = {'present': False}

        self.eeprom_path = self.EEPROM_PATH.format(self._port_to_i2c_mapping[self._port_num])
        self.reset_path = "{}{}{}{}".format(self.CPLD_I2C_PATH, self._cpld_mapping[0], '/module_reset_', self._port_num)
        self.present_path = "{}{}{}{}".format(self.CPLD_I2C_PATH, self._cpld_mapping[0], '/module_present_', self._port_num)
        self.lpmode_path = "{}{}{}{}".format(self.CPLD_I2C_PATH, self._cpld_mapping[0], '/module_lpmode_', self._port_num)


    def __write_txt_file(self, file_path, value):
        try:
            reg_file = open(file_path, "w")
        except IOError as e:
            logger.log_error("Error: unable to open file: {}".format(str(e)))
            return False

        reg_file.write(str(value))
        reg_file.close()

        return True

    def __is_host(self):
        cmd = shlex.split(self.HOST_CHK_CMD)
        return subprocess.call(cmd) == 0


    def __get_path_to_port_config_file(self):
        platform_path = "/".join([self.PLATFORM_ROOT_PATH, self._api_helper.platform])
        platform_json_path = platform_path if self.__is_host() else self.PMON_JSON_PATH
        config_file_path = "/".join([platform_json_path, "platform.json"])
        return config_file_path


    def get_eeprom_path(self):
        return self.eeprom_path


    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        sfputil_helper = SfpUtilHelper()
        sfputil_helper.read_porttab_mappings(self.__get_path_to_port_config_file())
        name = sfputil_helper.logical[self._index] or "Unknown"
        return name


    def get_presence(self):
        """
        Retrieves the presence of the device
        Returns:
            bool: True if device is present, False if not
        """
        val = self._api_helper.read_txt_file(self.present_path)

        if val is not None:
            return int(val, 10)==1
        else:
            return False


    def get_transceiver_change_event(self, timeout=2000):
        """
        Checks if transceiver has been inserted/removed
        Returns:
            SFP_STATUS_INSERTED, SFP_STATUS_REMOVED or None
        """
        present = self.get_presence()
        if present != self.data['present']:
            if present == True:
                sfp_event = SFP_STATUS_INSERTED
                logger.log_debug("get_transceiver_change_event: port {}: SFP_STATUS_INSERTED".format(self._port_num))
            else:
                sfp_event = SFP_STATUS_REMOVED
                logger.log_debug("get_transceiver_change_event: port {}: SFP_STATUS_REMOVED".format(self._port_num))
        else:
            sfp_event = None

        self.data['present'] = present
        return sfp_event


    def get_reset_status(self):
        """
        Retrieves the reset status of SFP
        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        val=self._api_helper.read_txt_file(self.reset_path)
        if val is None:
            return 0

        return int(val, 10)==1


    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP
        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        logger.log_debug("get_lpmode() is not yet implemented")
        return False

        if self._port_num > self.PORT_END:
            # SFP doesn't support this feature
            return False
        else:
            try:
                eeprom = None
                if not self.get_presence():
                    return False
                # Write to eeprom
                eeprom = open(self.eeprom_path, "rb")
                eeprom.seek(QSFP_POWEROVERRIDE_OFFSET)
                lpmode = ord(eeprom.read(1))

                if ((lpmode & 0x3) == 0x3):
                    return True  # Low Power Mode if "Power override" bit is 1 and "Power set" bit is 1
                else:
                    # High Power Mode if one of the following conditions is matched:
                    # 1. "Power override" bit is 0
                    # 2. "Power override" bit is 1 and "Power set" bit is 0
                    return False
            except IOError as e:
                logger.log_error("Error: unable to open file: {}".format(str(e)))
                return False
            finally:
                if eeprom is not None:
                    eeprom.close()
                    time.sleep(0.01)


    def reset(self):
        """
        Reset SFP and return all user module settings to their default srate.
        Returns:
            A boolean, True if successful, False if not
        """
        # Check for invalid port_num

        if self._port_num > self.PORT_END:
            return False # SFP doesn't support this feature
        elif not self.get_presence():
            return False

        ret = self.__write_txt_file(self.reset_path, 1)  #sysfs 1: enable reset
        if ret is not True:
            return ret

        time.sleep(0.2)
        ret = self.__write_txt_file(self.reset_path, 0) #sysfs 0: disable reset
        time.sleep(0.2)

        return ret


    def set_lpmode(self, lpmode):
        """
        Sets the lpmode (low power mode) of SFP
        Args:
            lpmode: A Boolean, True to enable lpmode, False to disable it
            Note  : lpmode can be overridden by set_power_override
        Returns:
            A boolean, True if lpmode is set successfully, False if not
        """
        if self._port_num > self.PORT_END:
            return False # SFP doesn't support this feature
        else:
            if not self.get_presence():
                return False

            if lpmode is True:
                self.set_power_override(True, True)
            else:
                self.set_power_override(True, False)

            return True

    def set_power_override(self, power_override, power_set):
        """
        Sets SFP power level using power_override and power_set
        Args:
            power_override :
                    A Boolean, True to override set_lpmode and use power_set
                    to control SFP power, False to disable SFP power control
                    through power_override/power_set and use set_lpmode
                    to control SFP power.
            power_set :
                    Only valid when power_override is True.
                    A Boolean, True to set SFP to low power mode, False to set
                    SFP to high power mode.
        Returns:
            A boolean, True if power-override and power_set are set successfully,
            False if not
        """
        if self._port_num > self.PORT_END:
            return False # SFP doesn't support this feature
        else:
            if not self.get_presence():
                return False
            try:
                power_override_bit = (1 << 0) if power_override else 0
                power_set_bit      = (1 << 1) if power_set else (1 << 3)

                buffer = create_string_buffer(1)
                if sys.version_info[0] >= 3:
                    buffer[0] = (power_override_bit | power_set_bit)
                else:
                    buffer[0] = chr(power_override_bit | power_set_bit)
                # Write to eeprom

                with open(self.eeprom_path, "r+b") as fd:
                    fd.seek(QSFP_POWEROVERRIDE_OFFSET)
                    fd.write(buffer[0])
                    time.sleep(0.01)
            except Exception as e:
                logger.log_error("Error: unable to open file: {}".format(str(e)))
                return False
            return True


    def get_transceiver_info(self):
        transceiver_info_dict = SfpOptoeBase.get_transceiver_info(self)
        if transceiver_info_dict is not None:
            transceiver_info_dict['hardware_rev'] = 'N/A'

        return transceiver_info_dict


    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_presence() and not self.get_reset_status()

