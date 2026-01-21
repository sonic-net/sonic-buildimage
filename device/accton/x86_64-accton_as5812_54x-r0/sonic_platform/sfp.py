#############################################################################
# Edgecore
#
# Sfp contains an implementation of SONiC Platform Base API and
# provides the sfp device status which are available in the platform
#
#############################################################################

import os
import sys
import time
import struct

from ctypes import create_string_buffer

try:
    import natsort
    from sonic_py_common.logger import Logger
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_platform_base.sonic_sfp.sfputilhelper import SfpUtilHelper
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

#Edge-core definitions

CPLD2_I2C_PATH = "/sys/bus/i2c/devices/0-0061/"
CPLD3_I2C_PATH = "/sys/bus/i2c/devices/0-0062/"

XCVR_TYPE_OFFSET = 0
XCVR_TYPE_WIDTH = 1

QSFP_CONTROL_WIDTH = 8
QSFP_CONTROL_OFFSET = 86


QSFP_POWEROVERRIDE_OFFSET = 93



I2C_EEPROM_PATH = '/sys/bus/i2c/devices/{0}-0050/eeprom'
OPTOE_DEV_CLASS_PATH = '/sys/bus/i2c/devices/{0}-0050/dev_class'

logger = Logger()
class Sfp(SfpOptoeBase):
    """Platform-specific Sfp class"""
    PLATFORM_ROOT_PATH = "/usr/share/sonic/device"
    PMON_HWSKU_PATH = "/usr/share/sonic/hwsku"
    HOST_CHK_CMD = "which systemctl > /dev/null 2>&1"
    PLATFORM = "x86_64-accton_as5812_54x-r0"
    HWSKU = "Accton-AS5812-54X"

    PORT_START = 1
    PORT_END = 54
    QSFP_PORT_START = 49
    QSFP_PORT_END = 54

    UPDATE_DONE = "Done"
    EEPROM_DATA_NOT_READY = "eeprom not ready"
    UNKNOWN_SFP_TYPE_ID = "unknow sfp ID"



    SFP_TYPE_CODE_LIST = [
        0x03,  # SFP/SFP+/SFP28
        0x0b   # DWDM-SFP/SFP+
    ]
    QSFP_TYPE_CODE_LIST = [
        0x0c, # QSFP
        0x0d, # QSFP+ or later
        0x11, # QSFP28 or later
        0xe1  # QSFP28 EDFA
    ]
    QSFP_DD_TYPE_CODE_LIST = [
        0x18,  # QSFP-DD Double Density 8X Pluggable Transceiver
        0x1E  # QSFP+ or later with CMIS
    ]
    OSFP_TYPE_CODE_LIST = [
        0x19  # OSFP
    ]



    SFP_TYPE = "SFP"
    QSFP_TYPE = "QSFP"
    OSFP_TYPE = "OSFP"
    QSFP_DD_TYPE = "QSFP_DD"

    NULL_VAL = 'N/A'

    _port_to_i2c_mapping = {
        1: 2,
        2: 3,
        3: 4,
        4: 5,
        5: 6,
        6: 7,
        7: 8,
        8: 9,
        9: 10,
        10:11 ,
        11:12,
        12:13,
        13:14,
        14:15,
        15:16,
        16:17,
        17:18,
        18:19,
        19:20,
        20:21,
        21:22,
        22:23,
        23:24,
        24:25,
        25:26,
        26:27,
        27:28,
        28:29,
        29:30,
        30:31,
        31:32,
        32:33,
        33:34,
        34:35,
        35:36,
        36:37,
        37:38,
        38:39,
        39:40,
        40:41,
        41:42,
        42:43,
        43:44,
        44:45,
        45:46,
        46:47,
        47:48,
        48:49,
        49:50,
        50:52,
        51:54,
        52:51,
        53:53,
        54:55,

    }


    CPLD2_PORT_START = 1
    CPLD2_PORT_END = 24
    CPLD3_PORT_START = 25
    CPLD3_PORT_END = 54
    PRS_PATH = "/sys/devices/platform/dx010_cpld/qsfp_modprs"

    def __init__(self, sfp_index=0, sfp_name=None):
        SfpOptoeBase.__init__(self)
        self._api_helper=APIHelper()
        self.port_num = sfp_index + 1
        self.index = self.port_num
        self._name = sfp_name


        if self.port_num < self.QSFP_PORT_START:
            self.sfp_type = self.SFP_TYPE
        else:
            self.sfp_type = self.QSFP_TYPE
        self.update_sfp_type()
        self.refresh_optoe_dev_class()

    def __write_txt_file(self, file_path, value):
        try:
            reg_file = open(file_path, "w")
        except IOError as e:
            logger.log_error("Error: unable to open file: %s" % str(e))
            return False

        reg_file.write(str(value))
        reg_file.close()

        return True

    def __get_path_to_port_config_file(self):
        platform_path = "/".join([self.PLATFORM_ROOT_PATH, self.PLATFORM])
        hwsku_path = "/".join([platform_path, self.HWSKU]
                              ) if self._api_helper.is_host() else self.PMON_HWSKU_PATH
        return "/".join([hwsku_path, "port_config.ini"])

    def get_eeprom_path(self):
        port_eeprom_path = I2C_EEPROM_PATH.format(self._port_to_i2c_mapping[self.port_num])
        return port_eeprom_path

    def read_eeprom(self, offset, num_bytes):
        port_eeprom_path = self.get_eeprom_path()
        #Try to solved IOError
        tries = 15
        for i in range(tries):
            try:
                eeprom = open(port_eeprom_path, mode='rb', buffering=0)
            except (IOError):
                if i < tries - 1:
                    time.sleep(0.02)
                    continue
                else:
                    return None
                break
        try:
            eeprom.seek(offset)
            eeprom_raw = bytearray(eeprom.read(num_bytes))
            eeprom.close()
            return eeprom_raw
        except (OSError, IOError):
            return None

    def refresh_optoe_dev_class(self):
        if self.get_presence():
            for retry in range(5):
                ret = self.update_sfp_type()
                if ret == self.EEPROM_DATA_NOT_READY:
                    time.sleep(1)
                else:
                    break
            if ret != self.UPDATE_DONE:
                logger.log_error("Error: port {}: update sfp type fail due to {}".format(self.port_num, ret))
                return False

        devclass_path = OPTOE_DEV_CLASS_PATH.format(self._port_to_i2c_mapping[self.port_num])
        devclass = self._api_helper.read_txt_file(devclass_path)
        if devclass is None:
            return False

        if self.sfp_type == self.QSFP_TYPE:
            if devclass == '1':
                return True
            return self._api_helper.write_txt_file(devclass_path, 1)
        elif self.sfp_type == self.SFP_TYPE:
            if devclass == '2':
                return True
            return self._api_helper.write_txt_file(devclass_path, 2)
        elif self.sfp_type == self.QSFP_DD_TYPE:
            if devclass == '3':
                return True
            return self._api_helper.write_txt_file(devclass_path, 3)
        else:
            return False

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP
        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        if not self.get_presence():
            return False
        if self.QSFP_PORT_START <= self.port_num <= self.QSFP_PORT_END:
            reset_path = "{}{}{}".format(CPLD3_I2C_PATH, '/module_reset_', self.port_num)
        else:
            return False

        val = self._api_helper.read_txt_file(reset_path)
        if val is not None:
            return int(val, 10)==1
        else:
            return False


    def get_tx_disable_channel(self):
        """
        Retrieves the TX disabled channels in this SFP
        Returns:
            A hex of 4 bits (bit 0 to bit 3 as channel 0 to channel 3) to represent
            TX channels which have been disabled in this SFP.
            As an example, a returned value of 0x5 indicates that channel 0
            and channel 2 have been disabled.
        """
        tx_disable_list = self.get_tx_disable()
        if tx_disable_list is None:
            return 0
        tx_disabled = 0
        for i in range(len(tx_disable_list)):
            if tx_disable_list[i]:
                tx_disabled |= 1 << i
        return tx_disabled

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP
        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        if self.port_num > self.PORT_END:
            # SFP doesn't support this feature
            return False

        if not self.get_presence():
            return False

        if self.sfp_type == self.SFP_TYPE:
            # SFP doesn't support this feature
            return False
        else:
            power_set=self.get_power_set()
            power_override = self.get_power_override()
            return power_set and power_override

    def get_power_set(self):

        if self.sfp_type == self.SFP_TYPE:
            # SFP doesn't support this feature
            return False
        else:
            api = self.get_xcvr_api()
            return api.get_power_set() if api is not None else None


    def reset(self):
        """
        Reset SFP and return all user module settings to their default srate.
        Returns:
            A boolean, True if successful, False if not
        """
        # Check for invalid port_num
        if not self.get_presence():
            return False

        if self.QSFP_PORT_START <= self.port_num <= self.QSFP_PORT_END:
            reset_path = "{}{}{}".format(CPLD3_I2C_PATH, 'module_reset_', self.port_num)
        else:
            return False

        ret = self.__write_txt_file(reset_path, 1) #sysfs 1: enable reset
        if ret is not True:
            return ret

        time.sleep(0.2)
        ret = self.__write_txt_file(reset_path, 0) #sysfs 0: disable reset
        time.sleep(0.2)

        return ret

    def tx_disable(self, tx_disable):
        """
        Disable SFP TX for all channels
        Args:
            tx_disable : A Boolean, True to enable tx_disable mode, False to disable
                         tx_disable mode.
        Returns:
            A boolean, True if tx_disable is set successfully, False if not
        """
        if self.sfp_type == self.QSFP_TYPE:
            api = self.get_xcvr_api()
            return api.tx_disable(tx_disable) if api is not None else None
        elif self.sfp_type == self.SFP_TYPE:
            if self.port_num <= self.CPLD2_PORT_END :
                disable_path = "{}{}{}".format(CPLD2_I2C_PATH, 'module_tx_disable_', self.port_num)
            else :
                disable_path = "{}{}{}".format(CPLD3_I2C_PATH, 'module_tx_disable_', self.port_num)

            tx_disable_value = 1 if tx_disable else 0
            ret = self._api_helper.write_txt_file(disable_path, tx_disable_value) #sysfs 1: disable tx
            if ret is not True:
                return ret

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
        if self.sfp_type == self.SFP_TYPE:
            return False # SFP doesn't support this feature
        elif self.port_num > self.PORT_END:
            return False # SFP doesn't support this feature
        else:
            if not self.get_presence():
                return False

            api = self.get_xcvr_api()
            if api is None:
                return False
            if api.get_lpmode_support() == False:
                logger.log_notice("The transceiver of port {} doesn't support to set low power mode.". format(self.port_num)) 
                return True
            if lpmode:
                ret = api.set_power_override(True, True)
            else:
                ret = api.set_power_override(True, False)
            return ret


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
        if self.sfp_type == self.SFP_TYPE:
            return False # SFP doesn't support this feature
        else:
            if not self.get_presence():
                return False
            api = self.get_xcvr_api()
            return api.set_power_override(power_override, power_set) if api is not None else None


    ##############################################################
    ###################### Device methods ########################
    ##############################################################

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        sfputil_helper = SfpUtilHelper()
        sfputil_helper.read_porttab_mappings(
            self.__get_path_to_port_config_file())
        logical_port_list = sfputil_helper.logical
        logical_port_list = natsort.natsorted(logical_port_list)
        name = logical_port_list[self.index - 1] or "Unknown"
        return name

    def get_presence(self):
        """
        Retrieves the presence of the device
        Returns:
            bool: True if device is present, False if not
        """
        if self.port_num <= self.CPLD2_PORT_END:
            present_path = "{}{}{}".format(CPLD2_I2C_PATH, '/module_present_', self.port_num)
        else:
            present_path = "{}{}{}".format(CPLD3_I2C_PATH, '/module_present_', self.port_num)

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
    
    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of
        entPhysicalContainedIn is'0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device
            or -1 if cannot determine the position
        """
        return self.port_num

    def is_replaceable(self):
        """
        Retrieves if replaceable
        Returns:
            A boolean value, True if replaceable
        """
        return True

    def update_sfp_type(self):
        """
        Updates the sfp type

        """
        ret = self.UPDATE_DONE
        eeprom_raw = []
        eeprom_raw = self.read_eeprom(0, 1)
        if eeprom_raw and hasattr(self,'sfp_type'):
            if eeprom_raw[0] in self.SFP_TYPE_CODE_LIST:
                self.sfp_type = self.SFP_TYPE
            elif eeprom_raw[0] in self.QSFP_TYPE_CODE_LIST:
                self.sfp_type = self.QSFP_TYPE
            elif eeprom_raw[0] in self.QSFP_DD_TYPE_CODE_LIST:
                self.sfp_type = self.QSFP_DD_TYPE
            elif eeprom_raw[0] in self.OSFP_TYPE_CODE_LIST:
                self.sfp_type = self.OSFP_TYPE
            else:
                ret = self.UNKNOWN_SFP_TYPE_ID
        else:
            ret = self.EEPROM_DATA_NOT_READY

        return ret

    def validate_eeprom_sfp(self):
        checksum_test = 0
        eeprom_raw = self.read_eeprom(0, 96)
        if eeprom_raw is None:
            return None

        for i in range(0, 63):
            checksum_test = (checksum_test + eeprom_raw[i]) & 0xFF
        else:
            if checksum_test != eeprom_raw[63]:
                return False

        checksum_test = 0
        for i in range(64, 95):
            checksum_test = (checksum_test + eeprom_raw[i]) & 0xFF
        else:
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
        else:
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
        else:
            if checksum_test != eeprom_raw[63]:
                return False

        checksum_test = 0
        for i in range(64, 95):
            checksum_test = (checksum_test + eeprom_raw[i]) & 0xFF
        else:
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
        else:
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
        else:
            if checksum_test != eeprom_raw[125]:
                return False

        checksum_test = 0
        eeprom_raw = self.read_eeprom(384, 128)
        if eeprom_raw is None:
            return None

        for i in range(0, 127):
            checksum_test = (checksum_test + eeprom_raw[i]) & 0xFF
        else:
            if checksum_test != eeprom_raw[127]:
                return False

        # CMIS_5.0 starts to support the checksum of page 04h
        cmis_rev = float(api.get_cmis_rev())
        if cmis_rev >= 5.0:
            checksum_test = 0
            eeprom_raw = self.read_eeprom(640, 128)
            if eeprom_raw is None:
                return None

            for i in range(0, 127):
                checksum_test = (checksum_test + eeprom_raw[i]) & 0xFF
            else:
                if checksum_test != eeprom_raw[127]:
                    return False

        return True

    def validate_eeprom(self):
        id_byte_raw = self.read_eeprom(0, 1)
        if id_byte_raw is None:
            return None

        id = id_byte_raw[0]
        if id in self.QSFP_TYPE_CODE_LIST:
            return self.validate_eeprom_qsfp()
        elif id in self.SFP_TYPE_CODE_LIST:
            return self.validate_eeprom_sfp()
        elif id in self.QSFP_DD_TYPE_CODE_LIST:
            return self.validate_eeprom_cmis()
        else:
            return False

    def validate_temperature(self):
        temperature = self.get_temperature()
        threshold_dict = self.get_transceiver_threshold_info()
        if temperature is None:
            return None

        if threshold_dict is None:
            return None

        if isinstance(temperature, float) is not True:
            return True

        if isinstance(threshold_dict['temphighalarm'], float) is not True:
            return True

        return threshold_dict['temphighalarm'] > temperature

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

        err_stat = self.SFP_STATUS_BIT_INSERTED

        status = self.validate_eeprom()
        if status is not True:
            err_stat = (err_stat | self.SFP_ERROR_BIT_BAD_EEPROM)

        status = self.validate_temperature()
        if status is False:
            err_stat = (err_stat | self.SFP_ERROR_BIT_HIGH_TEMP)

        if err_stat is self.SFP_STATUS_BIT_INSERTED:
            return self.SFP_STATUS_OK
        else:
            err_desc = ''
            cnt = 0
            for key in self.SFP_ERROR_BIT_TO_DESCRIPTION_DICT:
                if (err_stat & key) != 0:
                    if cnt > 0:
                        err_desc = err_desc + "|"
                        cnt = cnt + 1
                    err_desc = err_desc + self.SFP_ERROR_BIT_TO_DESCRIPTION_DICT[key]

            return err_desc
