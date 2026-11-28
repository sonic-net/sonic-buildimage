#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2019-2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#############################################################################
# Mellanox
#
# Module contains an implementation of SONiC Platform Base API and
# provides the FANs status which are available in the platform
#
#############################################################################

try:
    import os
    import threading
    from sonic_py_common.logger import Logger
    from sonic_py_common import multi_asic
    from . import utils
    from .device_data import DeviceDataManager
    from .module_detection_flow import ModuleDetectionFlow
    from .module_xcvr import ModuleXcvrMixin, SFP_FW_CONTROL, SFP_SW_CONTROL
    from .module_sysfs import ModuleSysfsMixin, SFP_SYSFS_PRESENT
    # Re-exported: chassis.py and the unit tests read the detection flow's
    # states and events as sfp.STATE_* / sfp.EVENT_*.
    from .module_detection_flow import (
        ACTION_FCP_ON_START,
        ACTION_ON_CANCEL_WAIT,
        ACTION_ON_FW_CONTROL,
        ACTION_ON_POWERED,
        ACTION_ON_POWER_LIMIT_ERROR,
        ACTION_ON_RESET,
        ACTION_ON_START,
        ACTION_ON_SW_CONTROL,
        EVENT_FW_CONTROL,
        EVENT_NOT_PRESENT,
        EVENT_POWER_BAD,
        EVENT_POWER_GOOD,
        EVENT_POWER_LIMIT_EXCEED,
        EVENT_POWER_ON,
        EVENT_PRESENT,
        EVENT_RESET,
        EVENT_RESET_DONE,
        EVENT_START,
        EVENT_SW_CONTROL,
        STATE_DOWN,
        STATE_FCP_DOWN,
        STATE_FCP_INIT,
        STATE_FCP_NOT_PRESENT,
        STATE_FCP_PRESENT,
        STATE_FW_CONTROL,
        STATE_INIT,
        STATE_NOT_PRESENT,
        STATE_POWERED_ON,
        STATE_POWER_BAD,
        STATE_POWER_LIMIT_ERROR,
        STATE_RESETTING,
        STATE_SW_CONTROL,
    )
    from .nvidia_sysfs_eeprom import NvidiaSysfsEepromUnbanked, SFP_PAGE0_PATH
    # Re-exported: chassis.py reads these as sfp.SFP_STATUS_*, and the unit
    # tests import the SX_PORT_MODULE_STATUS_* codes from here.
    from .module_sysfs import (
        SFP_STATUS_ERROR,
        SFP_STATUS_INSERTED,
        SFP_STATUS_REMOVED,
        SFP_STATUS_UNKNOWN,
        SX_PORT_MODULE_STATUS_INITIALIZING,
        SX_PORT_MODULE_STATUS_PLUGGED,
        SX_PORT_MODULE_STATUS_PLUGGED_DISABLED,
        SX_PORT_MODULE_STATUS_PLUGGED_WITH_ERROR,
        SX_PORT_MODULE_STATUS_UNPLUGGED,
    )
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase

except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

# identifier value of xSFP module which is in the first byte of the EEPROM
# if the identifier value falls into SFP_TYPE_CODE_LIST the module is treated as a SFP module and parsed according to 8472
# for QSFP_TYPE_CODE_LIST the module is treated as a QSFP module and parsed according to 8436/8636
# Originally the type (SFP/QSFP) of each module is determined according to the SKU dictionary
# where the type of each FP port is defined. The content of EEPROM is parsed according to its type.
# However, sometimes the SFP module can be fit in an adapter and then pluged into a QSFP port.
# In this case the EEPROM content is in format of SFP but parsed as QSFP, causing failure.
# To resolve that issue the type field of the xSFP module is also fetched so that we can know exectly what type the
# module is. Currently only the following types are recognized as SFP/QSFP module.
# Meanwhile, if the a module's identifier value can't be recognized, it will be parsed according to the SKU dictionary.
# This is because in the future it's possible that some new identifier value which is not regonized but backward compatible
# with the current format and by doing so it can be parsed as much as possible.
SFP_TYPE_CODE_LIST = [
    '03' # SFP/SFP+/SFP28
]
QSFP_TYPE_CODE_LIST = [
    '0d', # QSFP+ or later
    '11' # QSFP28 or later
]
QSFP_DD_TYPE_CODE_LIST = [
    '18' # QSFP-DD Double Density 8X Pluggable Transceiver
]

RJ45_TYPE = "RJ45"

#variables for sdk
REGISTER_NUM = 1
DEVICE_ID = 1
SWITCH_ID = 0

PMAOS_ASE = 1
PMAOS_EE = 1
PMAOS_E = 2
PMAOS_RST = 0
PMAOS_ENABLE = 1
PMAOS_DISABLE = 2

PMMP_LPMODE_BIT = 8
MCION_TX_DISABLE_BIT = 1

#on page 0
#i2c address 0x50
MCIA_ADDR_TX_CHANNEL_DISABLE = 86

MCIA_ADDR_POWER_OVERRIDE = 93
#power set bit
MCIA_ADDR_POWER_OVERRIDE_PS_BIT = 1
#power override bit
MCIA_ADDR_POWER_OVERRIDE_POR_BIT = 0

#on page 0
#i2c address 0x51
MCIA_ADDR_TX_DISABLE = 110
MCIA_ADDR_TX_DISABLE_BIT = 6

PORT_TYPE_NVE = 8
PORT_TYPE_CPU = 4
PORT_TYPE_OFFSET = 28
PORT_TYPE_MASK = 0xF0000000
NVE_MASK = PORT_TYPE_MASK & (PORT_TYPE_NVE << PORT_TYPE_OFFSET)
CPU_MASK = PORT_TYPE_MASK & (PORT_TYPE_CPU << PORT_TYPE_OFFSET)

# SFP type constants
SFP_TYPE_CMIS = 'cmis'
SFP_TYPE_CPO = 'cpo'
SFP_TYPE_SFF8472 = 'sff8472'
SFP_TYPE_SFF8636 = 'sff8636'

# SFP stderr
SFP_EEPROM_NOT_AVAILABLE = 'Input/output error'

# Module host management definitions begin
# SFP EEPROM limited bytes
limited_eeprom = {
    SFP_TYPE_CMIS: {
        'write': {
            0: [26, (31, 36), (126, 127)],
            16: [(0, 128)]
        }
    },
    SFP_TYPE_SFF8472: {
        'write': {
            0: [110, (114, 115), 118, 127]
        }
    },
    SFP_TYPE_SFF8636: {
        'write': {
            0: [(86, 88), 93, (98, 99), (100, 106), 127],
            3: [(230, 241), (242, 251)]
        }
    }
}

# Global logger class instance
logger = Logger()


class NvidiaSFPCommon(ModuleSysfsMixin, NvidiaSysfsEepromUnbanked, SfpOptoeBase):
    # Declaration order is load-bearing. The mixins have to precede
    # SfpOptoeBase so that the module-side set_power wins over the xcvr-side
    # one, and so that the sysfs EEPROM transport wins over the optoe
    # flat-file one.
    sfp_index_to_logical_port_dict = {}
    sfp_index_to_logical_lock = threading.Lock()

    def __init__(self, sfp_index, asic_id='asic0'):
        super(NvidiaSFPCommon, self).__init__()
        self.index = sfp_index + 1
        self.sdk_index = sfp_index
        self.asic_id = asic_id
        self.asic_index = multi_asic.get_asic_index_from_namespace(asic_id)

    @property
    def module_access(self):
        """A pluggable port carries its own sysfs coordinates and access."""
        return self

    def get_asic_id(self):
        return self.asic_id


class SFP(ModuleXcvrMixin, NvidiaSFPCommon):
    """Platform-specific SFP class"""
    shared_sdk_handle = None

    def __init__(self, sfp_index, sfp_type=None, slot_id=0, linecard_port_count=0, lc_name=None, asic_id='asic0'):
        super(SFP, self).__init__(sfp_index, asic_id=asic_id)
        self._sfp_type = sfp_type

        if slot_id == 0: # For non-modular chassis
            from .thermal import initialize_sfp_thermal
            self._thermal_list = initialize_sfp_thermal(self)
        else: # For modular chassis
            # (slot_id % MAX_LC_CONUNT - 1) * MAX_PORT_COUNT + (sfp_index + 1) * (MAX_PORT_COUNT / LC_PORT_COUNT)
            max_linecard_count = DeviceDataManager.get_linecard_count()
            max_linecard_port_count = DeviceDataManager.get_linecard_max_port_count()
            self.index = (slot_id % max_linecard_count - 1) * max_linecard_port_count + sfp_index * (max_linecard_port_count / linecard_port_count) + 1
            self.sdk_index = sfp_index

            from .thermal import initialize_linecard_sfp_thermal
            self._thermal_list = initialize_linecard_sfp_thermal(lc_name, slot_id, sfp_index)

        self.slot_id = slot_id
        self._sfp_type_str = None
        ModuleDetectionFlow.init_detection_flow(self, self.sdk_index)
        self.sn = None
        if DeviceDataManager.is_multi_asic_platform():
            self.namespace = asic_id
        else:
            self.namespace = multi_asic.DEFAULT_NAMESPACE

    def __str__(self):
        return f'SFP {self.sdk_index}'

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    # The three accessors StateMachine drives an entity through. Everything
    # else about the detection flow lives in ModuleDetectionFlow.
    def get_state(self):
        """Return the current state.

        Returns:
            str: current state
        """
        return self.state

    def change_state(self, new_state):
        """Change from old state to new state

        Args:
            new_state (str): new state
        """
        self.state = new_state

    def on_action(self, action_name):
        """Called when a state machine action is executing

        Args:
            action_name (str): action name
        """
        ModuleDetectionFlow.action_table[action_name](self)

    def fill_change_event(self, port_dict):
        """Fill change event data based on current state.

        Args:
            port_dict (dict): {<sfp_index>:<sfp_state>}
        """
        if self.state == STATE_NOT_PRESENT or self.state == STATE_FCP_NOT_PRESENT:
            port_dict[self.index] = SFP_STATUS_REMOVED
        elif self.state == STATE_SW_CONTROL or self.state == STATE_FW_CONTROL or self.state == STATE_FCP_PRESENT:
            port_dict[self.index] = SFP_STATUS_INSERTED
        elif self.state == STATE_POWER_BAD or self.state == STATE_POWER_LIMIT_ERROR:
            sfp_state = SFP.SFP_ERROR_BIT_POWER_BUDGET_EXCEEDED | SFP.SFP_STATUS_BIT_INSERTED
            port_dict[self.index] = str(sfp_state)

    def _uses_a2h_alias(self, eeprom_root):
        """SFF8472 parts alias bytes 256-511 onto the A2h file instead of page 01h."""
        return self._get_sfp_type_str(eeprom_root) == SFP_TYPE_SFF8472

    def _get_sfp_type_str(self, eeprom_path):
        """Get SFP type by reading first byte of EEPROM

        Args:
            eeprom_path (str): EEPROM path

        Returns:
            str: SFP type in string
        """
        if self._sfp_type_str is None:
            page = os.path.join(eeprom_path, SFP_PAGE0_PATH)
            try:
                with open(page, mode='rb', buffering=0) as f:
                    id_byte_raw = bytearray(f.read(1))
                    id = id_byte_raw[0]
                    if id == 0x18 or id == 0x19 or id == 0x1e:
                        self._sfp_type_str = SFP_TYPE_CMIS
                    elif id == 0x80:
                        self._sfp_type_str = SFP_TYPE_CPO
                    elif id == 0x11 or id == 0x0D:
                        # in sonic-platform-common, 0x0D is treated as sff8436,
                        # but it shared the same implementation on Nvidia platforms,
                        # so, we treat it as sff8636 here.
                        self._sfp_type_str = SFP_TYPE_SFF8636
                    elif id == 0x03:
                        self._sfp_type_str = SFP_TYPE_SFF8472
                    else:
                        logger.log_error(f'Unsupported sfp type {id}')
            except (OSError, IOError) as e:
                # SFP_EEPROM_NOT_AVAILABLE usually indicates SFP is not present, no need
                # print such error information to log
                if SFP_EEPROM_NOT_AVAILABLE not in str(e):
                    logger.log_error(f'Failed to get SFP type, index={self.sdk_index}, error={e}')
                return None
        return self._sfp_type_str

    def _is_write_protected(self, page, page_offset, num_bytes):
        """Check if the EEPROM read/write operation hit limitation bytes

        Args:
            page (str): EEPROM page path
            page_offset (int): EEPROM page offset
            num_bytes (int): read/write size

        Returns:
            bool: True if the limited bytes is hit
        """
        try:
            if self.is_sw_control():
                return False
        except Exception as e:
            logger.log_notice(f'Module is under initialization, cannot write module EEPROM - {e}')
            return True

        eeprom_path = self._get_eeprom_path()
        limited_data = limited_eeprom.get(self._get_sfp_type_str(eeprom_path))
        if not limited_data:
            return False

        access_type = 'write'
        limited_data = limited_data.get(access_type)
        if not limited_data:
            return False

        limited_ranges = limited_data.get(page)
        if not limited_ranges:
            return False

        access_begin = page_offset
        access_end = page_offset + num_bytes - 1
        for limited_range in limited_ranges:
            if isinstance(limited_range, int):
                if access_begin <= limited_range <= access_end:
                    return True
            else: # tuple
                if not (access_end < limited_range[0] or access_begin > limited_range[1]):
                    return True

        return False

    def get_xcvr_api(self):
        """
        Retrieves the XcvrApi associated with this SFP

        Returns:
            An object derived from XcvrApi that corresponds to the SFP
        """
        if self._xcvr_api is None:
            self.refresh_xcvr_api()
        return self._xcvr_api


class RJ45Port(NvidiaSFPCommon):
    """class derived from SFP, representing RJ45 ports"""

    def __init__(self, sfp_index, asic_id='asic0'):
        super(RJ45Port, self).__init__(sfp_index, asic_id=asic_id)
        self.sfp_type = RJ45_TYPE

    def get_presence(self):
        """
        Retrieves the presence of the device
        For RJ45 ports, it always return True

        Returns:
            bool: True if device is present, False if not
        """
        present = utils.read_int_from_file(self._module_attr_path(SFP_SYSFS_PRESENT))
        return present == 1

    def get_transceiver_info(self):
        """
        Retrieves transceiver info of this port.
        For RJ45, all fields are N/A

        Returns:
            A dict which contains following keys/values :
        ================================================================================
        keys                       |Value Format   |Information
        ---------------------------|---------------|----------------------------
        type                       |1*255VCHAR     |type of SFP
        vendor_rev                 |1*255VCHAR     |vendor revision of SFP
        serial                     |1*255VCHAR     |serial number of the SFP
        manufacturer               |1*255VCHAR     |SFP vendor name
        model                      |1*255VCHAR     |SFP model name
        connector                  |1*255VCHAR     |connector information
        encoding                   |1*255VCHAR     |encoding information
        ext_identifier             |1*255VCHAR     |extend identifier
        ext_rateselect_compliance  |1*255VCHAR     |extended rateSelect compliance
        cable_length               |INT            |cable length in m
        mominal_bit_rate           |INT            |nominal bit rate by 100Mbs
        specification_compliance   |1*255VCHAR     |specification compliance
        vendor_date                |1*255VCHAR     |vendor date
        vendor_oui                 |1*255VCHAR     |vendor OUI
        application_advertisement  |1*255VCHAR     |supported applications advertisement
        ================================================================================
        """
        transceiver_info_keys = ['manufacturer',
                                 'model',
                                 'vendor_rev',
                                 'serial',
                                 'vendor_oui',
                                 'vendor_date',
                                 'connector',
                                 'encoding',
                                 'ext_identifier',
                                 'ext_rateselect_compliance',
                                 'cable_type',
                                 'cable_length',
                                 'specification_compliance',
                                 'nominal_bit_rate',
                                 'application_advertisement']
        transceiver_info_dict = dict.fromkeys(transceiver_info_keys, 'N/A')
        transceiver_info_dict['type'] = self.sfp_type

        return transceiver_info_dict

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP

        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        return False

    def reset(self):
        """
        Reset SFP and return all user module settings to their default state.

        Returns:
            A boolean, True if successful, False if not

        refer plugins/sfpreset.py
        """
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
        return False

    def get_error_description(self):
        """
        Get error description

        Args:
            error_code: Always false on SN2201

        Returns:
            The error description
        """
        return False

    def get_transceiver_bulk_status(self):
        """
        Retrieves transceiver bulk status of this SFP

        Returns:
            A dict which contains following keys/values :
        ========================================================================
        keys                       |Value Format   |Information
        ---------------------------|---------------|----------------------------
        RX LOS                     |BOOLEAN        |RX lost-of-signal status,
                                   |               |True if has RX los, False if not.
        TX FAULT                   |BOOLEAN        |TX fault status,
                                   |               |True if has TX fault, False if not.
        Reset status               |BOOLEAN        |reset status,
                                   |               |True if SFP in reset, False if not.
        LP mode                    |BOOLEAN        |low power mode status,
                                   |               |True in lp mode, False if not.
        TX disable                 |BOOLEAN        |TX disable status,
                                   |               |True TX disabled, False if not.
        TX disabled channel        |HEX            |disabled TX channles in hex,
                                   |               |bits 0 to 3 represent channel 0
                                   |               |to channel 3.
        Temperature                |INT            |module temperature in Celsius
        Voltage                    |INT            |supply voltage in mV
        TX bias                    |INT            |TX Bias Current in mA
        RX power                   |INT            |received optical power in mW
        TX power                   |INT            |TX output power in mW
        ========================================================================
        """
        transceiver_dom_info_dict = {}

        dom_info_dict_keys = ['temperature',    'voltage',
                              'rx1power',       'rx2power',
                              'rx3power',       'rx4power',
                              'rx5power',       'rx6power',
                              'rx7power',       'rx8power',
                              'tx1bias',        'tx2bias',
                              'tx3bias',        'tx4bias',
                              'tx5bias',        'tx6bias',
                              'tx7bias',        'tx8bias',
                              'tx1power',       'tx2power',
                              'tx3power',       'tx4power',
                              'tx5power',       'tx6power',
                              'tx7power',       'tx8power'
                             ]
        transceiver_dom_info_dict = dict.fromkeys(dom_info_dict_keys, 'N/A')

        return transceiver_dom_info_dict


    def get_transceiver_threshold_info(self):
        """
        Retrieves transceiver threshold info of this SFP

        Returns:
            A dict which contains following keys/values :
        ========================================================================
        keys                       |Value Format   |Information
        ---------------------------|---------------|----------------------------
        temphighalarm              |FLOAT          |High Alarm Threshold value of temperature in Celsius.
        templowalarm               |FLOAT          |Low Alarm Threshold value of temperature in Celsius.
        temphighwarning            |FLOAT          |High Warning Threshold value of temperature in Celsius.
        templowwarning             |FLOAT          |Low Warning Threshold value of temperature in Celsius.
        vcchighalarm               |FLOAT          |High Alarm Threshold value of supply voltage in mV.
        vcclowalarm                |FLOAT          |Low Alarm Threshold value of supply voltage in mV.
        vcchighwarning             |FLOAT          |High Warning Threshold value of supply voltage in mV.
        vcclowwarning              |FLOAT          |Low Warning Threshold value of supply voltage in mV.
        rxpowerhighalarm           |FLOAT          |High Alarm Threshold value of received power in dBm.
        rxpowerlowalarm            |FLOAT          |Low Alarm Threshold value of received power in dBm.
        rxpowerhighwarning         |FLOAT          |High Warning Threshold value of received power in dBm.
        rxpowerlowwarning          |FLOAT          |Low Warning Threshold value of received power in dBm.
        txpowerhighalarm           |FLOAT          |High Alarm Threshold value of transmit power in dBm.
        txpowerlowalarm            |FLOAT          |Low Alarm Threshold value of transmit power in dBm.
        txpowerhighwarning         |FLOAT          |High Warning Threshold value of transmit power in dBm.
        txpowerlowwarning          |FLOAT          |Low Warning Threshold value of transmit power in dBm.
        txbiashighalarm            |FLOAT          |High Alarm Threshold value of tx Bias Current in mA.
        txbiaslowalarm             |FLOAT          |Low Alarm Threshold value of tx Bias Current in mA.
        txbiashighwarning          |FLOAT          |High Warning Threshold value of tx Bias Current in mA.
        txbiaslowwarning           |FLOAT          |Low Warning Threshold value of tx Bias Current in mA.
        ========================================================================
        """
        transceiver_dom_threshold_info_dict = {}

        dom_info_dict_keys = ['temphighalarm',    'temphighwarning',
                              'templowalarm',     'templowwarning',
                              'vcchighalarm',     'vcchighwarning',
                              'vcclowalarm',      'vcclowwarning',
                              'rxpowerhighalarm', 'rxpowerhighwarning',
                              'rxpowerlowalarm',  'rxpowerlowwarning',
                              'txpowerhighalarm', 'txpowerhighwarning',
                              'txpowerlowalarm',  'txpowerlowwarning',
                              'txbiashighalarm',  'txbiashighwarning',
                              'txbiaslowalarm',   'txbiaslowwarning'
                             ]
        transceiver_dom_threshold_info_dict = dict.fromkeys(dom_info_dict_keys, 'N/A')

        return transceiver_dom_threshold_info_dict

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP

        Returns:
            A Boolean, True if reset enabled, False if disabled

        for QSFP, originally I would like to make use of Initialization complete flag bit
        which is at Page a0 offset 6 bit 0 to test whether reset is complete.
        However as unit testing was carried out I find this approach may fail because:
            1. we make use of ethtool to read data on I2C bus rather than to read directly
            2. ethtool is unable to access I2C during QSFP module being reset
        In other words, whenever the flag is able to be retrived, the value is always be 1
        As a result, it doesn't make sense to retrieve that flag. Just treat successfully
        retrieving data as "data ready".
        for SFP it seems that there is not flag indicating whether reset succeed. However,
        we can also do it in the way for QSFP.
        """
        return False

    def read_eeprom(self, offset, num_bytes):
        return None

    def reinit(self):
        """
        Nothing to do for RJ45. Just provide it to avoid exception
        :return:
        """
        return

    def get_module_status(self):
        """Get value of sysfs status. It could return:
            SXD_PMPE_MODULE_STATUS_PLUGGED_ENABLED_E = 0x1,
            SXD_PMPE_MODULE_STATUS_UNPLUGGED_E = 0x2,
            SXD_PMPE_MODULE_STATUS_MODULE_PLUGGED_ERROR_E = 0x3,
            SXD_PMPE_MODULE_STATUS_PLUGGED_DISABLED_E = 0x4,
            SXD_PMPE_MODULE_STATUS_UNKNOWN_E = 0x5,

        Returns:
            str: sonic status of the module
        """
        status = super().get_module_status()
        return SFP_STATUS_REMOVED if status == SFP_STATUS_UNKNOWN else status

