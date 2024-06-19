#!/usr/bin/env python

# {C} Copyright 2023 AMD Systems Inc. All rights reserved
#############################################################################
# Pensando
#
# Sfp contains an implementation of SONiC Platform Base API and
# provides the sfp status which are available in the platform
#
#############################################################################

import time
import struct

try:
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_platform_base.sonic_sfp.sfputilhelper import SfpUtilHelper
    from sonic_platform.helper import APIHelper
    from sonic_py_common import syslogger
    import syslog
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

"""
DPU port details
"""

NUM_QSFP = 2

SFP_TYPE = "SFP"
QSFP_TYPE = "QSFP"
QSFP_DD_TYPE = "QSFP_DD"

NULL_VAL = 'N/A'

QSFP_PORT_START = 1
QSFP_PORT_END = 2

EEPROM_ADDRESS = "0x50"
QSFP_STAT_CTRL_CPLD_ADDR = "0x2"

SYSLOG_IDENTIFIER = "sonic_platform.sfp"
logger_instance = syslogger.SysLogger(SYSLOG_IDENTIFIER)

def log_error(msg, also_print_to_console=False):
    logger_instance.log_error(msg, also_print_to_console)

class NullSfp(SfpOptoeBase):
    def __init__(self):
        SfpOptoeBase.__init__(self)

    def get_presence(self):
        return False

class Sfp(SfpOptoeBase):
    """Platform-specific Sfp class"""

    def __init__(self, sfp_index=0, sfp_name=None):
        SfpOptoeBase.__init__(self)
        self._index = sfp_index
        self._port_num = self._index + 1
        self._name = f"Ethernet{self._index}" if sfp_name is None else sfp_name
        self._api_helper = APIHelper()

    def get_eeprom_path(self):
        i2c_num = self._port_num
        eeprom_file = f"/usr/share/sonic/platform/qsfp-eeprom-{i2c_num}.bin"
        if self._api_helper.is_host():
            platform = self._api_helper.get_platform()
            eeprom_file = f"/usr/share/sonic/device/{platform}/qsfp-eeprom-{i2c_num}.bin"
        return eeprom_file

    def get_reset_status(self):
        """
        Retrieves the reset status of QSFP
        Returns:
            A Boolean, True if reset enabled, False if disabled
            For SFP it is False
        """
        data = self._get_stat_ctrl_values()
        val = data.get("reset",0)
        return bool(val)

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this QSFP
        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
            For SFP, it is False
        """
        data = self._get_stat_ctrl_values()
        val = data.get("lpmode",0)
        return bool(val)

    ##############################################################
    ###################### Device methods ########################
    ##############################################################

    def _get_stat_ctrl_values(self):
        cmd = "cpldapp -r {}".format(QSFP_STAT_CTRL_CPLD_ADDR)
        try:
            output = self._api_helper.runCMD(cmd)
            reg_val = int(output, 16)
            data = {}
            index = self._index
            data["reset"] = ((reg_val >> index) & 0x1)
            data["lpmode"] = ((reg_val >> (index+2)) & 0x1)
            data["present"] = ((reg_val >> (index+4)) & 0x1)
            return data
        except:
            data = {}
            return data
        return {}

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        return self._name

    def get_transceiver_pm(self):
        return None

    def get_presence(self):
        """
        Retrieves the presence of the PSU
        Returns:
            bool: True if PSU is present, False if not
        """
        data = self._get_stat_ctrl_values()
        val = data.get("present",0)
        return bool(val)

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
        Returns:
            Temp return 0
        """
        return 0

    def is_replaceable(self):
        """
        Retrieves if replaceable
        Returns:
            A boolean value, True if replaceable
        """
        return True
