#############################################################################
# Edgecore
#
# Sfp contains an implementation of SONiC Platform Base API and
# provides the sfp device status which are available in the platform
#
#############################################################################

import os
import time

try:
    from sonic_py_common.logger import Logger
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_platform_base.sonic_sfp.sfputilhelper import SfpUtilHelper
    from .helper import APIHelper
    from sonic_py_common import device_info
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

NONE_SFP_TYPE = "NONE-SFP"
SFP_TYPE = "SFP"
QSFP_TYPE = "QSFP"

CPLD_I2C_PATH = "/sys/bus/i2c/devices/3-0060/"

logger = Logger()
class Sfp(SfpOptoeBase):
    """Platform-specific Sfp class"""

    # Port number
    PORT_START = 49
    PORT_END = 54

    # Path to sysfs
    PLATFORM_ROOT_PATH = "/usr/share/sonic/device"
    PMON_HWSKU_PATH = "/usr/share/sonic/hwsku"
    HOST_CHK_CMD = "which systemctl > /dev/null 2>&1"

    _port_to_i2c_mapping = {
           49: 18,
           50: 19,
           51: 20,
           52: 21,
           53: 22,
           54: 23,
    }

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

    def __init__(self, sfp_index=0):
        SfpOptoeBase.__init__(self)
        self._api_helper=APIHelper()
        # Init index
        self.port_num = sfp_index + 1
        self.index = self.port_num
        if self.port_num < self.PORT_START:
            self.sfp_type = NONE_SFP_TYPE
        elif self.port_num < 53:
            self.sfp_type = SFP_TYPE
        else:
            self.sfp_type = QSFP_TYPE

        # Init eeprom path
        eeprom_path = '/sys/bus/i2c/devices/{0}-0050/eeprom'
        self.port_to_eeprom_mapping = {}
        for x in range(self.PORT_START, self.PORT_END + 1):
            self.port_to_eeprom_mapping[x] = eeprom_path.format(self._port_to_i2c_mapping[x])

    def get_eeprom_path(self):
        return self.port_to_eeprom_mapping[self.port_num]

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP
        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        if self.port_num < 53: # non-QSFP ports don't support it.
            return False

        reset_path="{}{}{}".format(CPLD_I2C_PATH , "module_reset_" , str(self.port_num))
        val = self._api_helper.read_txt_file(reset_path)

        if val is not None:
            return int(val, 10) == 1
        else:
            return False

    def get_rx_los(self):
        """
        Retrieves the RX LOS (lost-of-signal) status of SFP
        Returns:
            A Boolean, True if SFP has RX LOS, False if not.
            Note : RX LOS status is latched until a call to get_rx_los or a reset.
        """
        rx_los = [False]

        if self.port_num < 49: #Copper port, no sysfs
            return [False]

        if self.port_num < 53:
            rx_path = "{}{}{}".format(CPLD_I2C_PATH, '/module_rx_los_', self.port_num)
            rx_los=self._api_helper.read_txt_file(rx_path)
            if rx_los is not None:
                if rx_los == '1':
                    return [True]
                else:
                    return [False]
            else:
                return [False]
        else:
            api = self.get_xcvr_api()
            if api is not None:
                rx_los = api.get_rx_los()
                if isinstance(rx_los, list) and "N/A" in rx_los:
                    return [False for _ in rx_los]
                return rx_los
            return None

    def get_tx_fault(self):
        """
        Retrieves the TX fault status of SFP
        Returns:
            A Boolean, True if SFP has TX fault, False if not
            Note : TX fault status is lached until a call to get_tx_fault or a reset.
        """
        tx_fault = [False]
        if self.port_num < 49: #Copper port, no sysfs
            return [False]

        if self.port_num < 53:
            tx_path = "{}{}{}".format(CPLD_I2C_PATH, '/module_tx_fault_', self.port_num)
            tx_fault=self._api_helper.read_txt_file(tx_path)
            if tx_fault is not None:
                if tx_fault == '1':
                    return [True]
                else:
                    return [False]
            else:
                return [False]
        else:
            api = self.get_xcvr_api()
            if api is not None:
                tx_fault = api.get_tx_fault()
                if isinstance(tx_fault, list) and "N/A" in tx_fault:
                    return [False for _ in tx_fault]
                return tx_fault
            return None

    def get_tx_disable(self):
        """
        Retrieves the tx_disable status of this SFP
        Returns:
            A Boolean, True if tx_disable is enabled, False if disabled
        """
        if self.port_num < 49: #Copper port, no sysfs
            return [False]

        if self.port_num < 53:
            tx_disable = False

            tx_path = "{}{}{}".format(CPLD_I2C_PATH, '/module_tx_disable_', self.port_num)
            tx_disable=self._api_helper.read_txt_file(tx_path)
            if tx_disable is not None:
                if tx_disable == '1':
                    return [True]
                else:
                    return [False]
            else:
                return [False]

        else:
            api = self.get_xcvr_api()
            return api.get_tx_disable() if api is not None else None

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP
        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        if self.port_num < 53:
            # SFP doesn't support this feature
            return False
        else:
            power_set=self.get_power_set()
            power_override = self.get_power_override()
            return power_set and power_override

    def get_power_set(self):
        if self.port_num < 53:
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
        if self.port_num < 53: # non-QSFP ports don't support it.
            return False

        reset_path = "{}{}{}".format(CPLD_I2C_PATH, 'module_reset_', self.port_num)
        ret = self._api_helper.write_txt_file(reset_path, 1)
        if ret is not True:
            return ret

        time.sleep(0.01)
        ret = self._api_helper.write_txt_file(reset_path, 0)
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
        if self.port_num < 49: #Copper port, no sysfs
            return False

        if self.port_num < 53:
            tx_path = "{}{}{}".format(CPLD_I2C_PATH, '/module_tx_disable_', self.port_num)
            ret = self._api_helper.write_txt_file(tx_path,  1 if tx_disable else 0)
            if ret is not None:
                time.sleep(0.01)
                return ret
            else:
                return False

        else:
            if not self.get_presence():
                return False
            api = self.get_xcvr_api()
            return api.tx_disable(tx_disable) if api is not None else None

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode (low power mode) of SFP
        Args:
            lpmode: A Boolean, True to enable lpmode, False to disable it
            Note  : lpmode can be overridden by set_power_override
        Returns:
            A boolean, True if lpmode is set successfully, False if not
        """
        if self.port_num < 53:
            return False # SFP doesn't support this feature
        else:
            '''
            use power override to control lpmode
            '''
            if not self.get_presence():
                return False
            api = self.get_xcvr_api()
            if api is None:
                return False
            if api.get_lpmode_support() == False:
                logger.log_notice("The transceiver of port {} doesn't support to set low power mode.". format(self.port_num))
                return True
            if lpmode is True:
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
        if self.port_num < 53:
            return False # SFP doesn't support this feature
        else:
            if not self.get_presence():
                return False
            api = self.get_xcvr_api()
            return api.set_power_override(power_override, power_set) if api is not None else None

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        sfputil_helper = SfpUtilHelper()
        port_config_file_path = device_info.get_path_to_port_config_file()
        sfputil_helper.read_porttab_mappings(port_config_file_path)
        name = sfputil_helper.logical[self.port_num - 1] or "Unknown"
        return name

    def get_presence(self):
        """
        Retrieves the presence of the device
        Returns:
            bool: True if device is present, False if not
        """
        if self.port_num < 49: #Copper port, no sysfs
            return False

        present_path = "{}{}{}".format(CPLD_I2C_PATH, '/module_present_', self.port_num)
        val=self._api_helper.read_txt_file(present_path)
        if val is not None:
            return int(val, 10)==1
        else:
            return False

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        transceiver_dom_info_dict = self.get_transceiver_info()
        return transceiver_dom_info_dict.get("model", "N/A")

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        transceiver_dom_info_dict = self.get_transceiver_info()
        return transceiver_dom_info_dict.get("serial", "N/A")

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_presence() and not self.get_reset_status()

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return self.port_num

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def __validate_eeprom_sfp(self):
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

    def __validate_eeprom_qsfp(self):
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

    def validate_eeprom(self):
        id_byte_raw = self.read_eeprom(0, 1)
        if id_byte_raw is None:
            return None

        id = id_byte_raw[0]
        if id in self.QSFP_TYPE_CODE_LIST:
            return self.__validate_eeprom_qsfp()
        elif id in self.SFP_TYPE_CODE_LIST:
            return self.__validate_eeprom_sfp()
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

    def __get_error_description(self):
        if not self.get_presence():
            return self.SFP_STATUS_UNPLUGGED

        err_stat = self.SFP_STATUS_BIT_INSERTED

        status = self.validate_eeprom()
        if status is not True:
            err_stat = (err_stat | self.SFP_ERROR_BIT_BAD_EEPROM)

        status = self.validate_temperature()
        if status is not True:
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
            return None
        else:
            api = self.get_xcvr_api()
            if api is not None:
                try:
                    return api.get_error_description()
                except NotImplementedError:
                    return self.__get_error_description()
            else:
                return self.__get_error_description()
