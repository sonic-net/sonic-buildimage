#############################################################################
# Accton
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
    from sonic_py_common.logger import Logger
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_platform_base.sonic_sfp.sfputilhelper import SfpUtilHelper
    from sonic_platform_base.sonic_sfp.sff8436 import sff8436Dom
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

#Edge-core definitions
FPGA_PCIE_PATH = "/sys/devices/platform/as9736_64d_fpga/"

XCVR_TYPE_OFFSET = 0
XCVR_TYPE_WIDTH = 1

QSFP_CONTROL_WIDTH = 8
QSFP_CONTROL_OFFSET = 86
QSFP_POWEROVERRIDE_OFFSET = 93

NULL_VAL = 'N/A'


SFP_I2C_START = 17
PCIE_UDB_EEPROM_PATH = '/sys/devices/platform/pcie_udb_fpga_device.{0}/eeprom'
PCIE_LDB_EEPROM_PATH = '/sys/devices/platform/pcie_ldb_fpga_device.{0}/eeprom'
PCIE_UDB_OPTOE_DEV_CLASS_PATH = '/sys/devices/platform/pcie_udb_fpga_device.{0}/dev_class'
PCIE_LDB_OPTOE_DEV_CLASS_PATH = '/sys/devices/platform/pcie_ldb_fpga_device.{0}/dev_class'

PCIE_UDB_BIND_PATH = "/sys/bus/platform/drivers/pcie_udb_fpga_device/{0}"
PCIE_LDB_BIND_PATH = "/sys/bus/platform/drivers/pcie_ldb_fpga_device/{0}"

logger = Logger()

class Sfp(SfpOptoeBase):
    """Platform-specific Sfp class"""
    HOST_CHK_CMD = "which systemctl > /dev/null 2>&1"
    PLATFORM = "x86_64-accton_as9736_64d-r0"
    HWSKU = "Accton-AS9736-64D"
    PORT_START = 1
    PORT_END = 66
    QSFP_PORT_START = 1
    QSFP_PORT_END = 64

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

    UDB_QSFP_PORT_START = 1
    UDB_QSFP_PORT_END = 32
    LDB_QSFP_PORT_START = 33
    LDB_QSFP_PORT_END = 64
    LDB_SFP_PORT_START = 65
    LDB_SFP_PORT_END = 66

    def __init__(self, sfp_index=0, sfp_name=None):
        SfpOptoeBase.__init__(self)

        self.index = sfp_index
        self.port_num = self.index + 1
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

    def __is_host(self):
        return os.system(self.HOST_CHK_CMD) == 0

    def __get_path_to_port_config_file(self):
        platform_path = "/".join([self.PLATFORM_ROOT_PATH, self.PLATFORM])
        hwsku_path = "/".join([platform_path, self.HWSKU]
                              ) if self.__is_host() else self.PMON_HWSKU_PATH
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
        if self.index < 32:
            port_eeprom_path = PCIE_UDB_EEPROM_PATH.format(self.index)
        else:
            port_eeprom_path = PCIE_LDB_EEPROM_PATH.format(self.index - 32)

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


    def write_eeprom(self, offset, num_bytes, write_buffer):
        try:
            with open(self.get_eeprom_path(), mode='r+b', buffering=0) as f:
                for i in range(num_bytes):
                    f.seek(offset+i)
                    f.write(write_buffer[i:i+1])
        except (OSError, IOError):
            return False
        return True


    def refresh_optoe_dev_class(self):
        if self.index < 32:
            port = "pcie_udb_fpga_device.{}".format(self.index)
            port_dev_unbind = PCIE_UDB_BIND_PATH.format("unbind")
            port_dev_bind = PCIE_UDB_BIND_PATH.format("bind")
        else:
            port = "pcie_ldb_fpga_device.{}".format(self.index-32)
            port_dev_unbind = PCIE_LDB_BIND_PATH.format("unbind")
            port_dev_bind = PCIE_LDB_BIND_PATH.format("bind")

        self._api_helper.write_txt_file(port_dev_unbind, port)
        self._api_helper.write_txt_file(port_dev_bind, port)

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP
        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        reset_path = "{}{}{}".format(FPGA_PCIE_PATH, '/module_reset_', self.port_num)

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
        if self.port_num > 64:
            # SFP doesn't support this feature
            return False

        if self.sfp_type == self.QSFP_DD_TYPE:
            api = self.get_xcvr_api()
            return api.get_lpmode()
        else:
            lpmode_path = "{}{}{}".format(FPGA_PCIE_PATH, '/module_lp_mode_', self.port_num)

            val=self._api_helper.read_txt_file(lpmode_path)
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

        if self.port_num > 64:
            return False # SFP doesn't support this feature
        else:
            if not self.get_presence():
                return False
            reset_path = "{}{}{}".format(FPGA_PCIE_PATH, 'module_reset_', self.port_num)

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
                sysfsfile_eeprom = open(self._eeprom_path, "r+b")
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
        if self.port_num > 64:
            return False # SFP doesn't support this feature
        else:
            if not self.get_presence():
                return False

            if self.sfp_type == self.QSFP_DD_TYPE:
                api = self.get_xcvr_api()
                api.set_lpmode(lpmode)
                return True
            else:
                lpmode_path = "{}{}{}".format(FPGA_PCIE_PATH, 'module_lp_mode_', self.port_num)

                if lpmode is True:
                    ret = self.__write_txt_file(lpmode_path, 1) #enable lpmode
                else:
                    ret = self.__write_txt_file(lpmode_path, 0) #disable lpmode

                return ret

    def _convert_raw_to_byte(self, raw, num_bytes):
        """
        Convert raw to the sytle that can parsing by sfpd_obj function
        Returns:
            A Array, hex value convert with no prefix of 0x
        """
        eeprom_raw = []

        for i in range(0, num_bytes):
            eeprom_raw.append("0x00")

        try:
            if isinstance(raw, str):
                for n in range(0, num_bytes):
                    eeprom_raw[n] = hex(ord(raw[n]))[2:].zfill(2)
            else:
                for n in range(0, num_bytes):
                    eeprom_raw[n] = hex(raw[n])[2:].zfill(2)

        except Exception:
            return None

        return eeprom_raw

    def get_power_override(self):
        """
        Retrieves the power-override status of this SFP
        Returns:
            A Boolean, True if power-override is enabled, False if disabled
        """
        power_override = False
        sfpd_obj = sff8436Dom()
        if sfpd_obj is None:
            return False

        dom_control_raw = self._convert_raw_to_byte(
                self.read_eeprom(QSFP_CONTROL_OFFSET, QSFP_CONTROL_WIDTH), QSFP_CONTROL_WIDTH) if self.get_presence() else None

        if dom_control_raw is not None:
            dom_control_data = sfpd_obj.parse_control_bytes(dom_control_raw, 0)
            power_override = (
                'On' == dom_control_data['data']['PowerOverride']['value'])

        return power_override

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

            port_eeprom_path = self.get_eeprom_path()

            fd = open(port_eeprom_path, mode="rb+", buffering=0)
            fd.seek(QSFP_POWEROVERRIDE_OFFSET)
            fd.write(buffer[0])
            time.sleep(0.01)
            fd.close()

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
        name = sfputil_helper.logical[self.index] or "Unknown"
        return name

    def get_presence(self):
        """
        Retrieves the presence of the device
        Returns:
            bool: True if device is present, False if not
        """
        present_path = "{}{}{}".format(FPGA_PCIE_PATH, '/module_present_', self.port_num)

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

        # EEPROM byte_1: Get '40h' indicates version 4.0, '52h' indicates version 5.2.
        # CMIS_5.0 starts to support the checksum of page 04h
        cmis_rev_byte_raw = self.read_eeprom(1, 1)
        if cmis_rev_byte_raw[0] >= 0x50:
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

