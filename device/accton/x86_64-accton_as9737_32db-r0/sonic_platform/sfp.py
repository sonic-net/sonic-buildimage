#############################################################################
# Edgecore
#
# Sfp contains an implementation of SONiC Platform Base API and
# provides the sfp device status which are available in the platform
#
#############################################################################

try:
    import time
    import natsort
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

CPLD2_PATH = "/sys/bus/i2c/devices/2-0061/"
CPLD3_PATH = "/sys/bus/i2c/devices/3-0062/"
EEPROM_PATH = '/sys/bus/i2c/devices/{}-00{}/eeprom'

class Sfp(SfpOptoeBase):
    """Platform-specific Sfp class"""

    # Port number
    PORT_START = 1
    PORT_END = 33

    LAST_QSFP_PORT_NUM = 32
    CPLD3_FIRST_PORT_NUM = 17

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

    _port_to_i2c_mapping = {
         1:9,   2:10,  3:11,  4:12,
         5:13,  6:14,  7:15,  8:16,
         9:17, 10:18, 11:19, 12:20,
        13:21, 14:22, 15:23, 16:24,
        17:25, 18:26, 19:27, 20:28,
        21:29, 22:30, 23:31, 24:32,
        25:33, 26:34, 27:35, 28:36,
        29:37, 30:38, 31:39, 32:40,
        33:41
    }

    def __init__(self, sfp_index=0, sfp_name=None):
        SfpOptoeBase.__init__(self)
        self._api_helper=APIHelper()

        # Init index
        self.port_num = sfp_index + 1
        self.index = self.port_num
        self.name = sfp_name

        # Init eeprom path
        self.port_to_eeprom_mapping = {}
        for x in range(self.PORT_START, self.PORT_END + 1):
            self.port_to_eeprom_mapping[x] = EEPROM_PATH.format(
                self._port_to_i2c_mapping[x], "50")

        # xvcrd will use 'sfp_type' for configuring the media type.
        if self.port_num > self.LAST_QSFP_PORT_NUM:
            self.sfp_type = self.SFP_TYPE
        else:
            self.sfp_type = self.QSFP_TYPE

        self.update_sfp_type()

    def get_eeprom_path(self):
        # print(self.port_to_eeprom_mapping[self.port_num])
        return self.port_to_eeprom_mapping[self.port_num]

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP
        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        if self.port_num > self.LAST_QSFP_PORT_NUM:
            # SFP doesn't support this feature
            return False

        if self.port_num < self.CPLD3_FIRST_PORT_NUM:
            reset_path = "{}{}{}".format(CPLD2_PATH, 'module_reset_', self.port_num)
        else:
            reset_path = "{}{}{}".format(CPLD3_PATH, 'module_reset_', self.port_num)

        val = self._api_helper.read_txt_file(reset_path)
        if val is not None:
            return int(val, 10) == 1

        return False

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP
        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        if self.port_num > self.LAST_QSFP_PORT_NUM:
            # SFP doesn't support this feature
            return False

        if self.sfp_type in [self.QSFP_DD_TYPE]:
            api = self.get_xcvr_api()
            return api.get_lpmode()
        else:
            if self.port_num < self.CPLD3_FIRST_PORT_NUM:
                lpmode_path = "{}{}{}".format(CPLD2_PATH, 'module_lpmode_', self.port_num)
            else:
                lpmode_path = "{}{}{}".format(CPLD3_PATH, 'module_lpmode_', self.port_num)

            val=self._api_helper.read_txt_file(lpmode_path)
            if val is not None:
                return int(val, 10)==1

        return False

    def reset(self):
        """
        Reset SFP and return all user module settings to their default srate.
        Returns:
            A boolean, True if successful, False if not
        """
        if not self.get_presence():
            return False

        # Check for invalid port_num
        if self.port_num > self.LAST_QSFP_PORT_NUM:
            return False # SFP doesn't support this feature

        if self.port_num < self.CPLD3_FIRST_PORT_NUM:
            reset_path = "{}{}{}".format(CPLD2_PATH, 'module_reset_', self.port_num)
        else:
            reset_path = "{}{}{}".format(CPLD3_PATH, 'module_reset_', self.port_num)

        ret = self._api_helper.write_txt_file(reset_path, 1)
        if ret is not True:
            return ret

        time.sleep(0.2)
        ret = self._api_helper.write_txt_file(reset_path, 0)
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
        if not self.get_presence():
            return False

        if self.port_num > self.LAST_QSFP_PORT_NUM:
            return False # SFP doesn't support this feature

        if self.sfp_type in [self.QSFP_DD_TYPE]:
            api = self.get_xcvr_api()
            ret = api.set_lpmode(lpmode)
        else:
            if self.port_num < self.CPLD3_FIRST_PORT_NUM:
                lpmode_path = "{}{}{}".format(CPLD2_PATH, 'module_lpmode_', self.port_num)
            else:
                lpmode_path = "{}{}{}".format(CPLD3_PATH, 'module_lpmode_', self.port_num)

            if lpmode is True:
                ret = self._api_helper.write_txt_file(lpmode_path, 1) #enable lpmode
            else:
                ret = self._api_helper.write_txt_file(lpmode_path, 0) #disable lpmode

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
        if not self.get_presence():
            return False

        if self.port_num > self.LAST_QSFP_PORT_NUM:
            txdisable_path = "{}{}{}".format(CPLD3_PATH, 'module_tx_disable_', self.port_num)

            if tx_disable is True:
                ret = self._api_helper.write_txt_file(txdisable_path, 1) #enable tx_disable
            else:
                ret = self._api_helper.write_txt_file(txdisable_path, 0) #disable tx_disable
        else:
            api = self.get_xcvr_api()
            if api is None:
                return False

            ret = api.tx_disable(tx_disable)

        return ret

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """

        return self.name

    def get_presence(self):
        """
        Retrieves the presence of the device
        Returns:
            bool: True if device is present, False if not
        """
        if self.port_num < self.CPLD3_FIRST_PORT_NUM:
            present_path = "{}{}{}".format(CPLD2_PATH, 'module_present_', self.port_num)
        else:
            present_path = "{}{}{}".format(CPLD3_PATH, 'module_present_', self.port_num)

        val = self._api_helper.read_txt_file(present_path)
        if val is not None:
            return int(val, 10)==1

        return False

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_presence()

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

    def update_sfp_type(self):
        """
        Updates the sfp type

        """
        if not self.get_presence():
            return self.EEPROM_DATA_NOT_READY

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
            return False

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
            return False

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
            return False

        id = id_byte_raw[0]
        if id in self.QSFP_TYPE_CODE_LIST:
            return self.validate_eeprom_qsfp()
        elif id in self.SFP_TYPE_CODE_LIST:
            return self.validate_eeprom_sfp()
        elif id in self.QSFP_DD_TYPE_CODE_LIST:
            return self.validate_eeprom_cmis()
        elif id in self.OSFP_TYPE_CODE_LIST:
            return self.validate_eeprom_cmis()
        else:
            return False

    def validate_temperature(self):
        temperature = self.get_temperature()
        if temperature is None:
            return False

        threshold_dict = self.get_transceiver_threshold_info()
        if threshold_dict is None:
            return False

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
        if not self.get_presence():
            return self.SFP_STATUS_UNPLUGGED

        try:
            state =  super().get_error_description()
            if state is None:
                return self.SFP_STATUS_OK
            return state
        except NotImplementedError:
            return self.__get_error_description()
