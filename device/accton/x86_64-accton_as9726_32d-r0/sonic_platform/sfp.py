#############################################################################
# Accton
#
# Sfp contains an implementation of SONiC Platform Base API and
# provides the sfp device status which are available in the platform
#
#############################################################################

import sys
import time
import struct
import natsort

from ctypes import create_string_buffer

try:
    from sonic_py_common.logger import Logger
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_platform_base.sonic_sfp.sfputilhelper import SfpUtilHelper
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

#Edge-core definitions
CPLD2_I2C_PATH = "/sys/bus/i2c/devices/10-0061/"
CPLD3_I2C_PATH = "/sys/bus/i2c/devices/10-0062/"

XCVR_TYPE_OFFSET = 0
XCVR_TYPE_WIDTH = 1

QSFP_CONTROL_WIDTH = 8
QSFP_CONTROL_OFFSET = 86

QSFP_POWEROVERRIDE_OFFSET = 93

NULL_VAL = 'N/A'


SFP_I2C_START = 17
I2C_EEPROM_PATH = '/sys/bus/i2c/devices/{0}-0050/eeprom'
OPTOE_DEV_CLASS_PATH = '/sys/bus/i2c/devices/{0}-0050/dev_class'

logger = Logger()
class Sfp(SfpOptoeBase):
    """Platform-specific Sfp class"""
    HOST_CHK_CMD = "which systemctl > /dev/null 2>&1"
    PLATFORM = "x86_64-accton_as9726_32d-r0"
    HWSKU = "Accton-AS9726-32D"
    PORT_START = 1
    PORT_END = 34
    QSFP_PORT_START = 1
    QSFP_PORT_END = 32

    # Path to sysfs
    PLATFORM_ROOT_PATH = "/usr/share/sonic/device"
    PMON_HWSKU_PATH = "/usr/share/sonic/hwsku"

    CPLD2_PORT_START = 1
    CPLD2_PORT_END = 16
    CPLD3_PORT_START = 17
    CPLD3_PORT_END = 32
    PRS_PATH = "/sys/devices/platform/dx010_cpld/qsfp_modprs"

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
        0x18, # QSFP-DD Double Density 8X Pluggable Transceiver
        0x1E  # QSFP+ or later with CMIS
    ]
    OSFP_TYPE_CODE_LIST = [
        0x19  # OSFP
    ]

    SFP_TYPE = "SFP"
    QSFP_TYPE = "QSFP"
    OSFP_TYPE = "OSFP"
    QSFP_DD_TYPE = "QSFP_DD"

    UPDATE_DONE = "Done"
    EEPROM_DATA_NOT_READY = "eeprom not ready"
    UNKNOWN_SFP_TYPE_ID = "unknow sfp ID"

    # Path to sysfs
    PLATFORM_ROOT_PATH = "/usr/share/sonic/device"
    PMON_HWSKU_PATH = "/usr/share/sonic/hwsku"

    def __init__(self, sfp_index=0, sfp_name=None):
        SfpOptoeBase.__init__(self)

        self.port_num = sfp_index + 1
        self.index = self.port_num
        self._api_helper = APIHelper()
        self._name = sfp_name

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

    def _convert_string_to_num(self, value_str):
        if "-inf" in value_str:
            return 'N/A'
        elif "Unknown" in value_str:
            return 'N/A'
        elif 'dBm' in value_str:
            t_str = value_str.rstrip('dBm')
            return float(t_str)
        elif 'mA' in value_str:
            t_str = value_str.rstrip('mA')
            return float(t_str)
        elif 'C' in value_str:
            t_str = value_str.rstrip('C')
            return float(t_str)
        elif 'Volts' in value_str:
            t_str = value_str.rstrip('Volts')
            return float(t_str)
        else:
            return 'N/A'

    def get_eeprom_path(self):
        port_to_i2c_mapping = SFP_I2C_START + (self.index - 1)
        port_eeprom_path = I2C_EEPROM_PATH.format(port_to_i2c_mapping)
        return port_eeprom_path

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

        devclass_path = OPTOE_DEV_CLASS_PATH.format(SFP_I2C_START + (self.index - 1))
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
        if self.port_num <= 16:
            reset_path = "{}{}{}".format(CPLD2_I2C_PATH, '/module_reset_', self.port_num)
        else:
            reset_path = "{}{}{}".format(CPLD3_I2C_PATH, '/module_reset_', self.port_num)

        val=self._api_helper.read_txt_file(reset_path)
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
        if self.port_num > 32:
            # SFP doesn't support this feature
            return False

        if not self.get_presence():
            return False

        if self.sfp_type == self.QSFP_DD_TYPE:
            api = self.get_xcvr_api()
            return api.get_lpmode()
        else:
            if self.port_num <= 16:
                lpmode_path = "{}{}{}".format(CPLD2_I2C_PATH, '/module_lpmode_', self.port_num)
            else:
                lpmode_path = "{}{}{}".format(CPLD3_I2C_PATH, '/module_lpmode_', self.port_num)

            val=self._api_helper.read_txt_file(lpmode_path)
            if val is not None:
                return int(val, 10)==0
            else:
                return False

    def reset(self):
        """
        Reset SFP and return all user module settings to their default srate.
        Returns:
            A boolean, True if successful, False if not
        """
        # Check for invalid port_num

        if self.port_num > 32:
            return False # SFP doesn't support this feature
        else:
            if not self.get_presence():
                return False

            if self.port_num <= self.CPLD2_PORT_END:
                reset_path = "{}{}{}".format(CPLD2_I2C_PATH, 'module_reset_', self.port_num)
            else:
                reset_path = "{}{}{}".format(CPLD3_I2C_PATH, 'module_reset_', self.port_num)

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
            sysfsfile_eeprom = None
            try:
                tx_disable_value = 0xf if tx_disable else 0x0
                # Write to eeprom
                sysfsfile_eeprom = open(self.get_eeprom_path(), "r+b")
                sysfsfile_eeprom.seek(QSFP_CONTROL_OFFSET)
                sysfsfile_eeprom.write(struct.pack('B', tx_disable_value))
            except IOError:
                return False
            finally:
                if sysfsfile_eeprom is not None:
                    sysfsfile_eeprom.close()
                    time.sleep(0.01)
            return True
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
        if self.port_num > 32:
            return False # SFP doesn't support this feature
        else:
            if not self.get_presence():
                return False

            if self.sfp_type == self.QSFP_DD_TYPE:
                api = self.get_xcvr_api()
                # ToDO: The return code for CMIS set_lpmode have some issue
                # workaround: always return True
                api.set_lpmode(lpmode)
                return True
            else:
                if self.port_num <= self.CPLD2_PORT_END:
                    lpmode_path = "{}{}{}".format(CPLD2_I2C_PATH, 'module_lpmode_', self.port_num)
                else:
                    lpmode_path = "{}{}{}".format(CPLD3_I2C_PATH, 'module_lpmode_', self.port_num)

                if lpmode is True:
                    ret = self.__write_txt_file(lpmode_path, 0) #enable lpmode
                else:
                    ret = self.__write_txt_file(lpmode_path, 1) #disable lpmode

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
        if self.port_num > 32:
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
                with open(self.get_eeprom_path(), "r+b") as fd:
                    fd.seek(QSFP_POWEROVERRIDE_OFFSET)
                    fd.write(buffer[0])
                    time.sleep(0.01)
            except Exception as e:
                logger.log_error("Error: unable to open file: %s" % str(e))
                return False
            return True

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
        logical_port_list = natsort.natsorted(sfputil_helper.logical)
        name = logical_port_list[self.index - 1] or "Unknown"
        return name

    def get_presence(self):
        """
        Retrieves the presence of the device
        Returns:
            bool: True if device is present, False if not
        """
        if self.port_num <= 16:
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
        Returns:
            Temp return 0
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

            return err_des
