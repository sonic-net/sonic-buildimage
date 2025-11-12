#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2019-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
    import ctypes
    import select
    import subprocess
    import os
    import threading
    import time
    from sonic_py_common.logger import Logger
    from sonic_py_common.general import check_output_pipe
    from . import utils
    from .device_data import DeviceDataManager
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_platform_base.sonic_xcvr.fields import consts
    from sonic_platform_base.sonic_xcvr.api.public import sff8636, sff8436

    from sonic_platform_base.sonic_xcvr.api.public import cmis as cmis_api
    from sonic_platform_base.sonic_xcvr.codes.public import cmis as cmis_codes
    from sonic_platform_base.sonic_xcvr.mem_maps.public import cmis as cmis_mem

except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

# Define the sdk constants
SX_PORT_MODULE_STATUS_INITIALIZING = 0
SX_PORT_MODULE_STATUS_PLUGGED = 1
SX_PORT_MODULE_STATUS_UNPLUGGED = 2
SX_PORT_MODULE_STATUS_PLUGGED_WITH_ERROR = 3
SX_PORT_MODULE_STATUS_PLUGGED_DISABLED = 4

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
CPO_TYPE = "CPO"

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

# parameters for SFP presence
SFP_STATUS_REMOVED = '0'
SFP_STATUS_INSERTED = '1'
SFP_STATUS_ERROR = '2'
SFP_STATUS_UNKNOWN = '-1'

# SFP status from PMAOS register
# 0x1 plug in
# 0x2 plug out
# 0x3 plug in with error
# 0x4 disabled, at this status SFP eeprom is not accessible, 
#     and presence status also will be not present, 
#     so treate it as plug out.
SDK_SFP_STATE_IN  = 0x1
SDK_SFP_STATE_OUT = 0x2
SDK_SFP_STATE_ERR = 0x3
SDK_SFP_STATE_DIS = 0x4
SDK_SFP_STATE_UNKNOWN = 0x5

SDK_STATUS_TO_SONIC_STATUS = {
    SDK_SFP_STATE_IN:  SFP_STATUS_INSERTED,
    SDK_SFP_STATE_OUT: SFP_STATUS_REMOVED,
    SDK_SFP_STATE_ERR: SFP_STATUS_ERROR,
    SDK_SFP_STATE_DIS: SFP_STATUS_REMOVED,
    SDK_SFP_STATE_UNKNOWN: SFP_STATUS_UNKNOWN
}

# SDK error definitions begin

# SFP errors that will block eeprom accessing
SDK_SFP_BLOCKING_ERRORS = [
    0x2, # SFP.SFP_ERROR_BIT_I2C_STUCK,
    0x3, # SFP.SFP_ERROR_BIT_BAD_EEPROM,
    0x5, # SFP.SFP_ERROR_BIT_UNSUPPORTED_CABLE,
    0x6, # SFP.SFP_ERROR_BIT_HIGH_TEMP,
    0x7, # SFP.SFP_ERROR_BIT_BAD_CABLE
]

# SDK error definitions end

# SFP constants
SFP_PAGE_SIZE = 256          # page size of page0h
SFP_UPPER_PAGE_OFFSET = 128  # page size of other pages

# SFP sysfs path constants
SFP_PAGE0_PATH = '0/i2c-0x50/data'
SFP_A2H_PAGE0_PATH = '0/i2c-0x51/data'
SFP_SDK_MODULE_SYSFS_ROOT_TEMPLATE = '/sys/module/sx_core/asic0/module{}/'
SFP_EEPROM_ROOT_TEMPLATE = SFP_SDK_MODULE_SYSFS_ROOT_TEMPLATE + 'eeprom/pages'
SFP_SYSFS_STATUS = 'status'
SFP_SYSFS_STATUS_ERROR = 'statuserror'
SFP_SYSFS_PRESENT = 'present'
SFP_SYSFS_RESET = 'reset'
SFP_SYSFS_HWRESET = 'hw_reset'
SFP_SYSFS_POWER_MODE = 'power_mode'
SFP_SYSFS_POWER_MODE_POLICY = 'power_mode_policy'
POWER_MODE_POLICY_HIGH = 1
POWER_MODE_POLICY_LOW = 3
POWER_MODE_LOW = 1
# POWER_MODE_HIGH = 2  # not used

# SFP type constants
SFP_TYPE_CMIS = 'cmis'
SFP_TYPE_SFF8472 = 'sff8472'
SFP_TYPE_SFF8636 = 'sff8636'

# SFP stderr
SFP_EEPROM_NOT_AVAILABLE = 'Input/output error'

SFP_TEMPERATURE_SCALE = 8.0

# Module host management definitions begin
SFP_SW_CONTROL = 1
SFP_FW_CONTROL = 0

CMIS_MAX_POWER_OFFSET = 201
CMIS_MEDIA_INTERFACE_TECH_OFFSET = 212

SFF_POWER_CLASS_MASK = 0xE3
SFF_POWER_CLASS_MAPPING = {
    0: 1.5,   # 1.5W
    64: 2,    # 2.0W
    128: 2.5, # 2.5W
    192: 3.5, # 3.5W
    193: 4,   # 4.0W
    194: 4.5, # 4.5W
    195: 5    # 5.0W
}
SFF_POWER_CLASS_OFFSET = 129
SFF_POWER_CLASS_8_INDICATOR = 32
SFF_POWER_CLASS_8_OFFSET = 107

CMIS_MCI_EEPROM_OFFSET = 2
CMIS_MCI_MASK = 0b00001100

STATE_DOWN = 'Down'                             # Initial state
STATE_INIT = 'Initializing'                     # Module starts initializing, check module present, also power on the module if need
STATE_RESETTING = 'Resetting'                   # Module is resetting the firmware
STATE_POWERED_ON = 'Power On'                   # Module is powered on, module firmware has been loaded, check module power is in good state
STATE_SW_CONTROL = 'Software Control'           # Module is under software control
STATE_FW_CONTROL = 'Firmware Control'           # Module is under firmware control
STATE_POWER_BAD = 'Power Bad'                   # Module power_good returns 0
STATE_POWER_LIMIT_ERROR = 'Exceed Power Limit'  # Module power exceeds cage power limit
STATE_NOT_PRESENT = 'Not Present'               # Module is not present

EVENT_START = 'Start'
EVENT_NOT_PRESENT = 'Not Present'
EVENT_RESET = 'Reset'
EVENT_POWER_ON = 'Power On'
EVENT_RESET_DONE = 'Reset Done'
EVENT_POWER_BAD = 'Power Bad'
EVENT_SW_CONTROL = 'Software Control'
EVENT_FW_CONTROL = 'Firmware Control'
EVENT_POWER_LIMIT_EXCEED = 'Power Limit Exceed'
EVENT_POWER_GOOD = 'Power Good'
EVENT_PRESENT = 'Present'

ACTION_ON_START = 'On Start'
ACTION_ON_RESET = 'On Reset'
ACTION_ON_POWERED = 'On Powered'
ACTION_ON_SW_CONTROL = 'On Software Control'
ACTION_ON_FW_CONTROL = 'On Firmware Control'
ACTION_ON_POWER_LIMIT_ERROR = 'On Power Limit Error'
ACTION_ON_CANCEL_WAIT = 'On Cancel Wait'

# States/actions for always firmware control ports
STATE_FCP_DOWN = 'Down(Firmware Control)'
STATE_FCP_INIT = 'Initializing(Firmware Control)'
STATE_FCP_NOT_PRESENT = 'Not Present(Firmware Control)'
STATE_FCP_PRESENT = 'Present(Firmware Control)'

ACTION_FCP_ON_START = 'On Start(Firmware Control)'
# Module host management definitions end

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


class NvidiaSFPCommon(SfpOptoeBase):
    sfp_index_to_logical_port_dict = {}
    sfp_index_to_logical_lock = threading.Lock()
    
    SFP_MLNX_ERROR_DESCRIPTION_LONGRANGE_NON_MLNX_CABLE = 'Long range for non-Mellanox cable or module'
    SFP_MLNX_ERROR_DESCRIPTION_ENFORCE_PART_NUMBER_LIST = 'Enforce part number list'
    SFP_MLNX_ERROR_DESCRIPTION_PMD_TYPE_NOT_ENABLED = 'PMD type not enabled'
    SFP_MLNX_ERROR_DESCRIPTION_PCIE_POWER_SLOT_EXCEEDED = 'PCIE system power slot exceeded'
    SFP_MLNX_ERROR_DESCRIPTION_RESERVED = 'Reserved'

    SDK_ERRORS_TO_DESCRIPTION = {
        0x1: SFP_MLNX_ERROR_DESCRIPTION_LONGRANGE_NON_MLNX_CABLE,
        0x4: SFP_MLNX_ERROR_DESCRIPTION_ENFORCE_PART_NUMBER_LIST,
        0x8: SFP_MLNX_ERROR_DESCRIPTION_PMD_TYPE_NOT_ENABLED,
        0xc: SFP_MLNX_ERROR_DESCRIPTION_PCIE_POWER_SLOT_EXCEEDED
    }

    SFP_MLNX_ERROR_BIT_LONGRANGE_NON_MLNX_CABLE = 0x00010000
    SFP_MLNX_ERROR_BIT_ENFORCE_PART_NUMBER_LIST = 0x00020000
    SFP_MLNX_ERROR_BIT_PMD_TYPE_NOT_ENABLED = 0x00040000
    SFP_MLNX_ERROR_BIT_PCIE_POWER_SLOT_EXCEEDED = 0x00080000
    SFP_MLNX_ERROR_BIT_RESERVED = 0x80000000

    SDK_ERRORS_TO_ERROR_BITS = {
        0x0: SfpOptoeBase.SFP_ERROR_BIT_POWER_BUDGET_EXCEEDED,
        0x1: SFP_MLNX_ERROR_BIT_LONGRANGE_NON_MLNX_CABLE,
        0x2: SfpOptoeBase.SFP_ERROR_BIT_I2C_STUCK,
        0x3: SfpOptoeBase.SFP_ERROR_BIT_BAD_EEPROM,
        0x4: SFP_MLNX_ERROR_BIT_ENFORCE_PART_NUMBER_LIST,
        0x5: SfpOptoeBase.SFP_ERROR_BIT_UNSUPPORTED_CABLE,
        0x6: SfpOptoeBase.SFP_ERROR_BIT_HIGH_TEMP,
        0x7: SfpOptoeBase.SFP_ERROR_BIT_BAD_CABLE,
        0x8: SFP_MLNX_ERROR_BIT_PMD_TYPE_NOT_ENABLED,
        0xc: SFP_MLNX_ERROR_BIT_PCIE_POWER_SLOT_EXCEEDED
    }

    def __init__(self, sfp_index):
        super(NvidiaSFPCommon, self).__init__()
        self.index = sfp_index + 1
        self.sdk_index = sfp_index

    @classmethod
    def _get_module_info(self, sdk_index):
        """
        Get oper state and error code of the SFP module

        Returns:
            The oper state and error code fetched from sysfs
        """
        status_file_path = SFP_SDK_MODULE_SYSFS_ROOT_TEMPLATE.format(sdk_index) + SFP_SYSFS_STATUS
        oper_state = utils.read_int_from_file(status_file_path)

        status_error_file_path = SFP_SDK_MODULE_SYSFS_ROOT_TEMPLATE.format(sdk_index) + SFP_SYSFS_STATUS_ERROR
        error_type = utils.read_int_from_file(status_error_file_path)

        return oper_state, error_type

    def get_fd(self, fd_type):
        return open(f'/sys/module/sx_core/asic0/module{self.sdk_index}/{fd_type}')

    def get_fd_for_polling_legacy(self):
        """Get polling fds for when module host management is disabled

        Returns:
            object: file descriptor of present
        """
        return self.get_fd('present')

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
        status = utils.read_int_from_file(f'/sys/module/sx_core/asic0/module{self.sdk_index}/status')
        return SDK_STATUS_TO_SONIC_STATUS[status]

    def get_error_info_from_sdk_error_type(self):
        """Translate SDK error type to SONiC error state and error description. Only calls
        when sysfs "present" returns "2".

        Returns:
            tuple: (error state, error description)
        """
        error_type = utils.read_int_from_file(f'/sys/module/sx_core/asic0/module{self.sdk_index}/statuserror', default=-1)
        sfp_state_bits = NvidiaSFPCommon.SDK_ERRORS_TO_ERROR_BITS.get(error_type)
        if sfp_state_bits is None:
            logger.log_error(f"Unrecognized error {error_type} detected on SFP {self.sdk_index}")
            return SFP_STATUS_ERROR, "Unknown error ({})".format(error_type)

        if error_type in SDK_SFP_BLOCKING_ERRORS:
            # In SFP at error status case, need to overwrite the sfp_state with the exact error code
            sfp_state_bits |= SfpOptoeBase.SFP_ERROR_BIT_BLOCKING

        # An error should be always set along with 'INSERTED'
        sfp_state_bits |= SfpOptoeBase.SFP_STATUS_BIT_INSERTED

        # For vendor specific errors, the description should be returned as well
        error_description = NvidiaSFPCommon.SDK_ERRORS_TO_DESCRIPTION.get(error_type)
        sfp_state = str(sfp_state_bits)
        return sfp_state, error_description
    

class SFP(NvidiaSFPCommon):
    """Platform-specific SFP class"""
    shared_sdk_handle = None
    
    # Class level state machine object, only applicable for module host management
    sm = None
    
    # Class level wait SFP ready task, the task waits for module to load its firmware after resetting,
    # only applicable for module host management
    wait_ready_task = None
    
    # Class level action table which stores the mapping from action name to action function,
    # only applicable for module host management
    action_table = None

    def __init__(self, sfp_index, sfp_type=None, slot_id=0, linecard_port_count=0, lc_name=None):
        super(SFP, self).__init__(sfp_index)
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
        # SFP state, only applicable for module host management
        fw_control_ports = DeviceDataManager.get_always_fw_control_ports()
        if not fw_control_ports or self.sdk_index not in fw_control_ports:
            self.state = STATE_DOWN
        else:
            self.state = STATE_FCP_DOWN
        self.processing_insert_event = False
        self.sn = None
        self.temp_high_threshold = None
        self.temp_critical_threshold = None

    def __str__(self):
        return f'SFP {self.sdk_index}'

    def reinit(self):
        """
        Re-initialize this SFP object when a new SFP inserted
        :return:
        """
        self._sfp_type_str = None
        self._xcvr_api = None

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        presence_sysfs = f'/sys/module/sx_core/asic0/module{self.sdk_index}/hw_present' if self.is_sw_control() else f'/sys/module/sx_core/asic0/module{self.sdk_index}/present'
        if utils.read_int_from_file(presence_sysfs) != 1:
            return False
        eeprom_raw = self._read_eeprom(0, 1, log_on_error=False)
        return eeprom_raw is not None
    
    @classmethod
    def wait_sfp_eeprom_ready(cls, sfp_list, wait_time):
        not_ready_list = sfp_list
        
        while wait_time > 0:
            not_ready_list = [s for s in not_ready_list if s.state == STATE_FW_CONTROL and s._read_eeprom(0, 2,False) is None]
            if not_ready_list:
                time.sleep(0.1)
                wait_time -= 0.1
            else:
                return
        
        for s in not_ready_list:
            logger.log_error(f'SFP {s.sdk_index} eeprom is not ready')

    # read eeprom specfic bytes beginning from offset with size as num_bytes
    def read_eeprom(self, offset, num_bytes):
        """
        Read eeprom specfic bytes beginning from a random offset with size as num_bytes
        Returns:
            bytearray, if raw sequence of bytes are read correctly from the offset of size num_bytes
            None, if the read_eeprom fails
        """
        return self._read_eeprom(offset, num_bytes)

    def _read_eeprom(self, offset, num_bytes, log_on_error=True):
        """Read eeprom specfic bytes beginning from a random offset with size as num_bytes

        Args:
            offset (int): read offset
            num_bytes (int): read size
            log_on_error (bool, optional): whether log error when exception occurs. Defaults to True.

        Returns:
            bytearray: the content of EEPROM
        """
        result = bytearray(0)
        while num_bytes > 0:
            _, page, page_offset = self._get_page_and_page_offset(offset)
            if not page:
                return None

            try:
                with open(page, mode='rb', buffering=0) as f:
                    f.seek(page_offset)
                    content = f.read(num_bytes)
                    if not result:
                        result = content
                    else:
                        result += content
                    read_length = len(content)
                    if read_length == 0:
                        logger.log_error(f'SFP {self.sdk_index}: EEPROM page {page} is empty, no data retrieved')
                        return None
                    num_bytes -= read_length
                    if num_bytes > 0:
                        page_size = f.seek(0, os.SEEK_END)
                        if page_offset + read_length == page_size:
                            offset += read_length
                        else:
                            # Indicate read finished
                            num_bytes = 0
                    if ctypes.get_errno() != 0:
                        raise IOError(f'errno = {os.strerror(ctypes.get_errno())}')
                    logger.log_debug(f'read EEPROM sfp={self.sdk_index}, page={page}, page_offset={page_offset}, '\
                        f'size={read_length}, data={content}')
            except (OSError, IOError) as e:
                if log_on_error:
                    logger.log_warning(f'Failed to read sfp={self.sdk_index} EEPROM page={page}, page_offset={page_offset}, '\
                        f'size={num_bytes}, offset={offset}, error = {e}')
                return None

        return bytearray(result)

    # write eeprom specfic bytes beginning from offset with size as num_bytes
    def write_eeprom(self, offset, num_bytes, write_buffer):
        """
        write eeprom specfic bytes beginning from a random offset with size as num_bytes
        and write_buffer as the required bytes
        Returns:
            Boolean, true if the write succeeded and false if it did not succeed.
        Example:
            mlxreg -d /dev/mst/mt52100_pciconf0 --reg_name MCIA --indexes slot_index=0,module=1,device_address=154,page_number=5,i2c_device_address=0x50,size=1,bank_number=0 --set dword[0]=0x01000000 -y
        """
        if num_bytes != len(write_buffer):
            logger.log_error("Error mismatch between buffer length and number of bytes to be written")
            return False

        while num_bytes > 0:
            page_num, page, page_offset = self._get_page_and_page_offset(offset)
            if not page:
                return False

            try:
                if self._is_write_protected(page_num, page_offset, num_bytes):
                    # write limited eeprom is not supported
                    raise IOError('write limited bytes')
                with open(page, mode='r+b', buffering=0) as f:
                    f.seek(page_offset)
                    ret = f.write(write_buffer[0:num_bytes])
                    written_buffer = write_buffer[0:ret]
                    if ret != num_bytes:
                        page_size = f.seek(0, os.SEEK_END)
                        if page_offset + ret == page_size:
                            # Move to next page
                            write_buffer = write_buffer[ret:num_bytes]
                            offset += ret
                        else:
                            raise IOError(f'write return code = {ret}')
                    num_bytes -= ret
                    if ctypes.get_errno() != 0:
                        raise IOError(f'errno = {os.strerror(ctypes.get_errno())}')
                    logger.log_debug(f'write EEPROM sfp={self.sdk_index}, page={page}, page_offset={page_offset}, '\
                        f'size={ret}, left={num_bytes}, data={written_buffer}')
            except (OSError, IOError) as e:
                data = ''.join('{:02x}'.format(x) for x in write_buffer)
                logger.log_error(f'Failed to write EEPROM data sfp={self.sdk_index} EEPROM page={page}, page_offset={page_offset}, size={num_bytes}, '\
                    f'offset={offset}, data = {data}, error = {e}')
                return False
        return True

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP

        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        try:
            if self.is_sw_control():
                api = self.get_xcvr_api()
                return api.get_lpmode() if api else False
        except Exception as e:
            print(e)
            return False

        file_path = SFP_SDK_MODULE_SYSFS_ROOT_TEMPLATE.format(self.sdk_index) + SFP_SYSFS_POWER_MODE
        power_mode = utils.read_int_from_file(file_path)
        return power_mode == POWER_MODE_LOW

    def reset(self):
        """
        Reset SFP and return all user module settings to their default state.

        Returns:
            A boolean, True if successful, False if not

        refer plugins/sfpreset.py
        """
        try:
            if not self.is_sw_control():
                file_path = SFP_SDK_MODULE_SYSFS_ROOT_TEMPLATE.format(self.sdk_index) + SFP_SYSFS_RESET
                return utils.write_file(file_path, '1')
            else:
                file_path = SFP_SDK_MODULE_SYSFS_ROOT_TEMPLATE.format(self.sdk_index) + SFP_SYSFS_HWRESET
                return utils.write_file(file_path, '0') and utils.write_file(file_path, '1')
        except Exception as e:
            print(f'Failed to reset module - {e}')
            logger.log_error(f'Failed to reset module - {e}')
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
        try:
            if self.is_sw_control():
                api = self.get_xcvr_api()
                if not api:
                    return False
                if api.get_lpmode() == lpmode:
                    return True
                api.set_lpmode(lpmode)
                # check_lpmode is a lambda function which checks if current lpmode already updated to the desired lpmode
                check_lpmode = lambda api, lpmode: api.get_lpmode() == lpmode
                # utils.wait_until function will call check_lpmode function every 1 second for a total timeout of 2 seconds.
                # If at some point get_lpmode=desired_lpmode, it will return true.
                # If after timeout ends, lpmode will not be desired_lpmode, it will return false.
                return utils.wait_until(check_lpmode, 2, 1, api=api, lpmode=lpmode)
        except Exception as e:
            print(e)
            return False

        file_path = SFP_SDK_MODULE_SYSFS_ROOT_TEMPLATE.format(self.sdk_index) + SFP_SYSFS_POWER_MODE_POLICY
        target_admin_mode = POWER_MODE_POLICY_LOW if lpmode else POWER_MODE_POLICY_HIGH
        current_admin_mode = utils.read_int_from_file(file_path)
        if current_admin_mode == target_admin_mode:
            return True
        return utils.write_file(file_path, str(target_admin_mode))

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    @classmethod
    def _get_error_description_dict(cls):
        return {0: cls.SFP_ERROR_DESCRIPTION_POWER_BUDGET_EXCEEDED,
                1: cls.SFP_MLNX_ERROR_DESCRIPTION_LONGRANGE_NON_MLNX_CABLE,
                2: cls.SFP_ERROR_DESCRIPTION_I2C_STUCK,
                3: cls.SFP_ERROR_DESCRIPTION_BAD_EEPROM,
                4: cls.SFP_MLNX_ERROR_DESCRIPTION_ENFORCE_PART_NUMBER_LIST,
                5: cls.SFP_ERROR_DESCRIPTION_UNSUPPORTED_CABLE,
                6: cls.SFP_ERROR_DESCRIPTION_HIGH_TEMP,
                7: cls.SFP_ERROR_DESCRIPTION_BAD_CABLE,
                8: cls.SFP_MLNX_ERROR_DESCRIPTION_PMD_TYPE_NOT_ENABLED,
                12: cls.SFP_MLNX_ERROR_DESCRIPTION_PCIE_POWER_SLOT_EXCEEDED,
                255: cls.SFP_MLNX_ERROR_DESCRIPTION_RESERVED
        }

    def get_error_description(self):
        """
        Get error description

        Args:
            error_code: The error code returned by _get_module_info

        Returns:
            The error description
        """
        try:
            if self.is_sw_control():
                api = self.get_xcvr_api()
                return api.get_error_description() if api else None
        except NotImplementedError:
            return 'Not supported'
        except Exception:
            return self.SFP_STATUS_INITIALIZING

        oper_status, error_code = self._get_module_info(self.sdk_index)
        if oper_status == SX_PORT_MODULE_STATUS_INITIALIZING:
            error_description = self.SFP_STATUS_INITIALIZING
        elif oper_status == SX_PORT_MODULE_STATUS_PLUGGED:
            error_description = self.SFP_STATUS_OK
        elif oper_status == SX_PORT_MODULE_STATUS_UNPLUGGED:
            error_description = self.SFP_STATUS_UNPLUGGED
        elif oper_status == SX_PORT_MODULE_STATUS_PLUGGED_DISABLED:
            error_description = self.SFP_STATUS_DISABLED
        elif oper_status == SX_PORT_MODULE_STATUS_PLUGGED_WITH_ERROR:
            error_description_dict = self._get_error_description_dict()
            if error_code in error_description_dict:
                error_description = error_description_dict[error_code]
            else:
                error_description = "Unknown error ({})".format(error_code)
        else:
            error_description = "Unknow SFP module status ({})".format(oper_status)
        return error_description

    def _get_eeprom_path(self):
        return SFP_EEPROM_ROOT_TEMPLATE.format(self.sdk_index)

    def _get_page_and_page_offset(self, overall_offset):
        """Get EEPROM page and page offset according to overall offset

        Args:
            overall_offset (int): Overall read offset

        Returns:
            tuple: (<page_num>, <page_path>, <page_offset>)
        """
        eeprom_path = self._get_eeprom_path()
        if not os.path.exists(eeprom_path):
            logger.log_error(f'EEPROM file path for sfp {self.sdk_index} does not exist')
            return None, None, None

        if overall_offset < SFP_PAGE_SIZE:
            return 0, os.path.join(eeprom_path, SFP_PAGE0_PATH), overall_offset

        if self._get_sfp_type_str(eeprom_path) == SFP_TYPE_SFF8472:
            page1h_start = SFP_PAGE_SIZE * 2
            if overall_offset < page1h_start:
                return -1, os.path.join(eeprom_path, SFP_A2H_PAGE0_PATH), overall_offset - SFP_PAGE_SIZE
        else:
            page1h_start = SFP_PAGE_SIZE

        page_num = (overall_offset - page1h_start) // SFP_UPPER_PAGE_OFFSET + 1
        page = f'{page_num}/data'
        offset = (overall_offset - page1h_start) % SFP_UPPER_PAGE_OFFSET
        return page_num, os.path.join(eeprom_path, page), offset

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

    def get_rx_los(self):
        """Accessing rx los is not supproted, return all False

        Returns:
            list: [False] * channels
        """
        api = self.get_xcvr_api()
        try:
            if self.is_sw_control():
                return api.get_rx_los() if api else None
        except Exception as e:
            print(e)
        return [False] * api.NUM_CHANNELS if api else None

    def get_tx_fault(self):
        """Accessing tx fault is not supproted, return all False

        Returns:
            list: [False] * channels
        """
        api = self.get_xcvr_api()
        try:
            if self.is_sw_control():
                return api.get_tx_fault() if api else None
        except Exception as e:
            print(e)
        return [False] * api.NUM_CHANNELS if api else None
    
    def reinit_if_sn_changed(self):
        """Reinitialize the SFP if the module ID has changed
        """
        sn = self.get_serial()
        if sn != self.sn:
            self.reinit()
            self.sn = self.get_serial()
            self.temp_high_threshold = None
            self.temp_critical_threshold = None
            return True
        return False
            
    def get_temperature_info(self):
        """Get SFP temperature info in a fast way. This function is faster than calling following functions one by one: get_temperature, get_temperature_warning_threshold, get_temperature_critical_threshold.

        Returns:
            tuple: (temperature, warning_threshold, critical_threshold)
        """
        try:
            sw_control = self.is_sw_control()
            if not sw_control:
                return sw_control, None, None, None

            sn_changed = self.reinit_if_sn_changed()
            # software control, read from EEPROM
            temperature = super().get_temperature()
            if temperature is None:
                # Failed to read temperature, no need read threshold
                return sw_control, None, None, None
            elif temperature == 0.0:
                # Temperature is not supported, no need read threshold
                return sw_control, 0.0, 0.0, 0.0
            else:
                if not sn_changed and self.temp_high_threshold is not None and self.temp_critical_threshold is not None:
                    return sw_control, temperature, self.temp_high_threshold, self.temp_critical_threshold
                else:
                    # Read threshold from EEPROM
                    api = self.get_xcvr_api()
                    thresh_support = api.get_transceiver_thresholds_support()
                    if thresh_support is None:
                        # Failed to read threshold support field, no need read threshold
                        return sw_control, temperature, None, None
                    if thresh_support:
                        # Read threshold from EEPROM
                        self.temp_high_threshold = api.xcvr_eeprom.read(consts.TEMP_HIGH_WARNING_FIELD)
                        self.temp_critical_threshold = api.xcvr_eeprom.read(consts.TEMP_HIGH_ALARM_FIELD)
                        return sw_control, temperature, self.temp_high_threshold, self.temp_critical_threshold
                    else:
                        # No threshold support, use default threshold
                        return sw_control, temperature, 0.0, 0.0
        except:
            # module under initialization, return as temperature not supported
            return False, None, None, None

    def get_temperature(self):
        """Get SFP temperature

        Returns:
            None if there is an error (sysfs does not exist or sysfs return None or module EEPROM not readable)
            0.0 if module temperature is not supported or module is under initialization
            other float value if module temperature is available
        """
        try:
            if not self.is_sw_control():
                temp_file = f'/sys/module/sx_core/asic0/module{self.sdk_index}/temperature/input'
                if not os.path.exists(temp_file):
                    logger.log_error(f'Failed to read from file {temp_file} - not exists')
                    return None
                temperature = utils.read_int_from_file(temp_file,
                                                       log_func=None)
                return temperature / SFP_TEMPERATURE_SCALE if temperature is not None else None
        except:
            return 0.0

        self.reinit_if_sn_changed()
        return super().get_temperature()

    def get_temperature_warning_threshold(self):
        """Get temperature warning threshold

        Returns:
            None if there is an error (module EEPROM not readable)
            0.0 if warning threshold is not supported or module is under initialization
            other float value if warning threshold is available
        """
        try:
            self.is_sw_control()
        except:
            return 0.0
        
        self.temp_high_threshold = self._get_temperature_threshold(consts.TEMP_HIGH_WARNING_FIELD)
        return self.temp_high_threshold

    def get_temperature_critical_threshold(self):
        """Get temperature critical threshold

        Returns:
            None if there is an error (module EEPROM not readable)
            0.0 if critical threshold is not supported or module is under initialization
            other float value if critical threshold is available
        """
        try:
            self.is_sw_control()
        except:
            return 0.0

        self.temp_critical_threshold = self._get_temperature_threshold(consts.TEMP_HIGH_ALARM_FIELD)
        return self.temp_critical_threshold

    def _get_temperature_threshold(self, thresh_field):
        """Get temperature thresholds data from EEPROM
        
        Args:
            thresh_field (str): threshold field name

        Returns:
            float: temperature threshold
        """
        sn_changed = self.reinit_if_sn_changed()
        if not sn_changed:
            if thresh_field == consts.TEMP_HIGH_WARNING_FIELD and self.temp_high_threshold is not None:
                return self.temp_high_threshold
            elif thresh_field == consts.TEMP_HIGH_ALARM_FIELD and self.temp_critical_threshold is not None:
                return self.temp_critical_threshold
        api = self.get_xcvr_api()
        if not api:
            return None

        thresh_support = api.get_transceiver_thresholds_support()
        if thresh_support is None:
            return None
        return api.xcvr_eeprom.read(thresh_field) if thresh_support else 0.0

    def get_xcvr_api(self):
        """
        Retrieves the XcvrApi associated with this SFP

        Returns:
            An object derived from XcvrApi that corresponds to the SFP
        """
        if self._xcvr_api is None:
            self.refresh_xcvr_api()
        return self._xcvr_api

    def is_sw_control(self):
        if not DeviceDataManager.is_module_host_management_mode():
            return False
        try:
            return utils.read_int_from_file(f'/sys/module/sx_core/asic0/module{self.sdk_index}/control', 
                                            raise_exception=True, log_func=None) == 1
        except:
            # just in case control file does not exist
            raise Exception(f'control sysfs for SFP {self.sdk_index} does not exist')
    
    def get_hw_present(self):
        """Get hardware present status, only applicable on host management mode

        Returns:
            bool: True if module is in the cage
        """
        return utils.read_int_from_file(f'/sys/module/sx_core/asic0/module{self.sdk_index}/hw_present') == 1
    
    def get_power_on(self):
        """Get power on status, only applicable on host management mode

        Returns:
            bool: True if the module is powered on
        """
        return utils.read_int_from_file(f'/sys/module/sx_core/asic0/module{self.sdk_index}/power_on') == 1
    
    def set_power(self, on):
        """Control the power of this module, only applicable on host management mode

        Args:
            on (bool): True if on
        """
        value = 1 if on else 0
        utils.write_file(f'/sys/module/sx_core/asic0/module{self.sdk_index}/power_on', value)
    
    def get_reset_state(self):
        """Get reset state of this module, only applicable on host management mode

        Returns:
            bool: True if module is not in reset status
        """
        return utils.read_int_from_file(f'/sys/module/sx_core/asic0/module{self.sdk_index}/hw_reset') == 1
    
    def set_hw_reset(self, value):
        """Set the module reset status

        Args:
            value (int): 1 for reset, 0 for leaving reset
        """
        utils.write_file(f'/sys/module/sx_core/asic0/module{self.sdk_index}/hw_reset', value)
    
    def get_power_good(self):
        """Get power good status of this module, only applicable on host management mode

        Returns:
            bool: True if the power is in good status
        """
        return utils.read_int_from_file(f'/sys/module/sx_core/asic0/module{self.sdk_index}/power_good') == 1
    
    def get_control_type(self):
        """Get control type of this module, only applicable on host management mode

        Returns:
            int: 1 - software control, 0 - firmware control
        """
        return utils.read_int_from_file(f'/sys/module/sx_core/asic0/module{self.sdk_index}/control')
    
    def set_control_type(self, control_type):
        """Set control type for the module

        Args:
            control_type (int): 0 for firmware control, currently only 0 is allowed
        """
        utils.write_file(f'/sys/module/sx_core/asic0/module{self.sdk_index}/control', control_type)
    
    def determine_control_type(self):
        """Determine control type according to module type

        Returns:
            enum: software control or firmware control
        """
        api = self.get_xcvr_api()
        if not api:
            logger.log_error(f'Failed to get api object for SFP {self.sdk_index}, probably module EEPROM is not ready')
            return SFP_FW_CONTROL
        
        if not self.is_supported_for_software_control(api):
            return SFP_FW_CONTROL
        else:
            return SFP_SW_CONTROL
        
    def is_cmis_api(self, xcvr_api):
        """Check if the api type is CMIS

        Args:
            xcvr_api (object): xcvr api object

        Returns:
            bool: True if the api is of type CMIS
        """
        return isinstance(xcvr_api, cmis_api.CmisApi)

    def is_sff_api(self, xcvr_api):
        """Check if the api type is SFF

        Args:
            xcvr_api (object): xcvr api object

        Returns:
            bool: True if the api is of type SFF
        """
        return isinstance(xcvr_api, sff8636.Sff8636Api) or isinstance(xcvr_api, sff8436.Sff8436Api)

    def check_media_interface_technology(self, xcvr_api):
        """Check media interface technology
        0x0F in offset 212, means the module is 'Copper cable, linear active equalizers'.
        Nvidia doesn't support it to be SW control, so we set it to FW control
        Args:
            xcvr_api (object): xcvr api object
        """
        media_interface = self.read_eeprom(CMIS_MEDIA_INTERFACE_TECH_OFFSET, 1)
        return media_interface[0] != 0x0F if media_interface else False

    def is_supported_for_software_control(self, xcvr_api):
        """Check if the api object supports software control

        Args:
            xcvr_api (object): xcvr api object

        Returns:
            bool: True if the api object supports software control
        """
        if xcvr_api.is_flat_memory():
            if self.is_cmis_api(xcvr_api):
                # For Copper active modules, Nvidia doesn't support SW control
                return self.check_media_interface_technology(xcvr_api)
            return self.is_sff_api(xcvr_api)

        return self.is_cmis_api(xcvr_api)

    def check_power_capability(self):
        """Check module max power with cage power limit

        Returns:
            bool: True if max power does not exceed cage power limit
        """
        max_power = self.get_module_max_power()
        if max_power < 0:
            return False
        
        power_limit = self.get_power_limit()
        logger.log_info(f'SFP {self.sdk_index}: max_power={max_power}, power_limit={power_limit}')
        if max_power <= power_limit:
            return True
        else:
            logger.log_error(f'SFP {self.sdk_index} exceed power limit: max_power={max_power}, power_limit={power_limit}')
            return False
            
    def get_power_limit(self):
        """Get power limit of this module

        Returns:
            int: Power limit in unit of 0.25W
        """
        return utils.read_int_from_file(f'/sys/module/sx_core/asic0/module{self.sdk_index}/power_limit')
        
    def get_module_max_power(self):
        """Get module max power from EEPROM

        Returns:
            int: max power in terms of 0.25W. Return POWER_CLASS_INVALID if EEPROM data is incorrect.
        """
        xcvr_api = self.get_xcvr_api()
        if self.is_cmis_api(xcvr_api):
            powercap_raw = self.read_eeprom(CMIS_MAX_POWER_OFFSET, 1)
            return powercap_raw[0]
        elif self.is_sff_api(xcvr_api):
            power_class_raw = self.read_eeprom(SFF_POWER_CLASS_OFFSET, 1)
            power_class_bit = power_class_raw[0] & SFF_POWER_CLASS_MASK
            if power_class_bit in SFF_POWER_CLASS_MAPPING:
                powercap = SFF_POWER_CLASS_MAPPING[power_class_bit]
            elif power_class_bit == SFF_POWER_CLASS_8_INDICATOR:
                # According to standard:
                # Byte 128:
                #    if bit 5 is 1, "Power Class 8 implemented (Max power declared in byte 107)"
                # Byte 107: 
                #    "Maximum power consumption of module. Unsigned integer with LSB = 0.1 W."
                power_class_8_byte = self.read_eeprom(SFF_POWER_CLASS_8_OFFSET, 1)
                powercap = power_class_8_byte[0] * 0.1
            else:
                logger.log_error(f'SFP {self.sdk_index} got invalid value for power class field: {power_class_bit}')
                return -1

            # Multiplying the sysfs value (0.25 Watt units) by 4 aligns it with the EEPROM max power value (1 Watt units), 
            # ensuring both are in the same unit for a meaningful comparison
            return powercap * 4 #
        else:
            # Should never hit, just in case
            logger.log_error(f'SFP {self.sdk_index} with api type {xcvr_api} does not support getting max power')
            return -1
 
    def update_i2c_frequency(self):
        """Update I2C frequency for the module.
        """
        if self.get_frequency_support():
            api = self.get_xcvr_api()
            if self.is_cmis_api(api):
                # for CMIS modules, read the module maximum supported clock of Management Comm Interface (MCI) from module EEPROM.
                # from byte 2 bits 3-2:
                # 00b means module supports up to 400KHz
                # 01b means module supports up to 1MHz
                logger.log_debug(f"Reading mci max frequency for SFP {self.sdk_index}")
                read_mci = self.read_eeprom(CMIS_MCI_EEPROM_OFFSET, 1)
                logger.log_debug(f"Read mci max frequency {read_mci[0]} for SFP {self.sdk_index}")
                frequency = (read_mci[0] & CMIS_MCI_MASK) >> 2
            elif self.is_sff_api(api):
                # for SFF modules, frequency is always 400KHz
                frequency = 0
            else:
                # Should never hit, just in case
                logger.log_error(f'SFP {self.sdk_index} with api type {api} does not support updating frequency but frequency_support sysfs return 1')
                return
            
            logger.log_info(f"Read mci max frequency bits {frequency} for SFP {self.sdk_index}")
            self.set_frequency(frequency)
    
    def get_frequency_support(self):
        """Get frequency support for this module

        Returns:
            bool: True if supported
        """
        return utils.read_int_from_file(f'/sys/module/sx_core/asic0/module{self.sdk_index}/frequency_support') == 1
    
    def set_frequency(self, freqeuncy):
        """Set module frequency.

        Args:
            freqeuncy (int): 0 - up to 400KHz, 1 - up to 1MHz
        """
        utils.write_file(f'/sys/module/sx_core/asic0/module{self.sdk_index}/frequency', freqeuncy)
    
    def disable_tx_for_sff_optics(self):
        """Disable TX for SFF optics
        """
        api = self.get_xcvr_api()
        if self.is_sff_api(api) and api.get_tx_disable_support():
            logger.log_info(f'Disabling tx for SFP {self.sdk_index}')
            api.tx_disable(True)
    
    @classmethod
    def get_state_machine(cls):
        """Get state machine object, create if not exists

        Returns:
            object: state machine object
        """
        if not cls.sm:
            from .state_machine import StateMachine
            sm = StateMachine()
            sm.add_state(STATE_DOWN).add_transition(EVENT_START, STATE_INIT)
            sm.add_state(STATE_INIT).set_entry_action(ACTION_ON_START) \
              .add_transition(EVENT_NOT_PRESENT, STATE_NOT_PRESENT) \
              .add_transition(EVENT_RESET, STATE_RESETTING) \
              .add_transition(EVENT_POWER_ON, STATE_POWERED_ON) \
              .add_transition(EVENT_FW_CONTROL, STATE_FW_CONTROL)  # for warm reboot, cable might be in firmware control at startup
            sm.add_state(STATE_RESETTING).set_entry_action(ACTION_ON_RESET) \
              .add_transition(EVENT_RESET_DONE, STATE_POWERED_ON) \
              .add_transition(EVENT_NOT_PRESENT, STATE_NOT_PRESENT, ACTION_ON_CANCEL_WAIT)
            sm.add_state(STATE_POWERED_ON).set_entry_action(ACTION_ON_POWERED) \
              .add_transition(EVENT_POWER_BAD, STATE_POWER_BAD) \
              .add_transition(EVENT_SW_CONTROL, STATE_SW_CONTROL) \
              .add_transition(EVENT_FW_CONTROL, STATE_FW_CONTROL)
            sm.add_state(STATE_SW_CONTROL).set_entry_action(ACTION_ON_SW_CONTROL) \
              .add_transition(EVENT_NOT_PRESENT, STATE_NOT_PRESENT) \
              .add_transition(EVENT_POWER_LIMIT_EXCEED, STATE_POWER_LIMIT_ERROR) \
              .add_transition(EVENT_POWER_BAD, STATE_POWER_BAD)
            sm.add_state(STATE_FW_CONTROL).set_entry_action(ACTION_ON_FW_CONTROL) \
              .add_transition(EVENT_NOT_PRESENT, STATE_NOT_PRESENT)
            sm.add_state(STATE_POWER_BAD).add_transition(EVENT_POWER_GOOD, STATE_POWERED_ON) \
              .add_transition(EVENT_NOT_PRESENT, STATE_NOT_PRESENT)
            sm.add_state(STATE_NOT_PRESENT).add_transition(EVENT_PRESENT, STATE_INIT)
            sm.add_state(STATE_POWER_LIMIT_ERROR).set_entry_action(ACTION_ON_POWER_LIMIT_ERROR) \
              .add_transition(EVENT_POWER_GOOD, STATE_POWERED_ON) \
              .add_transition(EVENT_NOT_PRESENT, STATE_NOT_PRESENT)
                          
            cls.action_table = {}
            cls.action_table[ACTION_ON_START] = cls.action_on_start
            cls.action_table[ACTION_ON_RESET] = cls.action_on_reset
            cls.action_table[ACTION_ON_POWERED] = cls.action_on_powered
            cls.action_table[ACTION_ON_SW_CONTROL] = cls.action_on_sw_control
            cls.action_table[ACTION_ON_FW_CONTROL] = cls.action_on_fw_control
            cls.action_table[ACTION_ON_CANCEL_WAIT] = cls.action_on_cancel_wait
            cls.action_table[ACTION_ON_POWER_LIMIT_ERROR] = cls.action_on_power_limit_error
            
            # For always firewire control ports
            sm.add_state(STATE_FCP_DOWN).add_transition(EVENT_START, STATE_FCP_INIT)
            sm.add_state(STATE_FCP_INIT).set_entry_action(ACTION_FCP_ON_START) \
              .add_transition(EVENT_NOT_PRESENT, STATE_FCP_NOT_PRESENT) \
              .add_transition(EVENT_PRESENT, STATE_FCP_PRESENT)
            sm.add_state(STATE_FCP_NOT_PRESENT).add_transition(EVENT_PRESENT, STATE_FCP_PRESENT)
            sm.add_state(STATE_FCP_PRESENT).add_transition(EVENT_NOT_PRESENT, STATE_FCP_NOT_PRESENT)
            
            cls.action_table[ACTION_FCP_ON_START] = cls.action_fcp_on_start
            
            cls.sm = sm
            
        return cls.sm
    
    @classmethod
    def action_on_start(cls, sfp):
        if sfp.get_control_type() == SFP_FW_CONTROL:
            logger.log_info(f'SFP {sfp.sdk_index} is already FW control, probably in warm reboot')
            sfp.on_event(EVENT_FW_CONTROL)
            return
        
        if not sfp.get_hw_present():
            logger.log_info(f'SFP {sfp.sdk_index} is not present')
            sfp.on_event(EVENT_NOT_PRESENT)
            return
        
        if not sfp.get_power_on():
            logger.log_info(f'SFP {sfp.sdk_index} is not powered on')
            sfp.set_power(True)
            sfp.set_hw_reset(1)
            sfp.on_event(EVENT_RESET)
        else:
            if not sfp.get_reset_state():
                logger.log_info(f'SFP {sfp.sdk_index} is in reset state')
                sfp.set_hw_reset(1)
                sfp.on_event(EVENT_RESET)
            else:
                if not sfp.processing_insert_event:
                    sfp.on_event(EVENT_POWER_ON)
                else:
                    sfp.processing_insert_event = False
                    logger.log_info(f'SFP {sfp.sdk_index} is processing insert event and needs to wait module ready')
                    sfp.on_event(EVENT_RESET)

    @classmethod
    def action_fcp_on_start(cls, sfp):
        present = utils.read_int_from_file(f'/sys/module/sx_core/asic0/module{sfp.sdk_index}/present')
        if present:
            sfp.on_event(EVENT_PRESENT)
        else:
            sfp.on_event(EVENT_NOT_PRESENT)
            
    @classmethod
    def action_on_reset(cls, sfp):
        logger.log_info(f'SFP {sfp.sdk_index} is scheduled to wait for resetting done')
        cls.get_wait_ready_task().schedule_wait(sfp.sdk_index)
        
    @classmethod
    def action_on_powered(cls, sfp):
        if not sfp.get_power_good():
            logger.log_error(f'SFP {sfp.sdk_index} is not in power good state')
            sfp.on_event(EVENT_POWER_BAD)
            return
        
        control_type = sfp.determine_control_type()
        if control_type == SFP_SW_CONTROL:
            sfp.on_event(EVENT_SW_CONTROL)
        else:
            sfp.on_event(EVENT_FW_CONTROL)
            
    @classmethod
    def action_on_sw_control(cls, sfp):
        if not sfp.check_power_capability():
            sfp.on_event(EVENT_POWER_LIMIT_EXCEED)
            return
        
        sfp.update_i2c_frequency()
        sfp.disable_tx_for_sff_optics()
        logger.log_info(f'SFP {sfp.sdk_index} is set to software control')
        
    @classmethod
    def action_on_fw_control(cls, sfp):
        if sfp.get_control_type() != SFP_FW_CONTROL:
            logger.log_info(f'SFP {sfp.sdk_index} is set to firmware control')
            sfp.set_control_type(SFP_FW_CONTROL)
        
    @classmethod
    def action_on_cancel_wait(cls, sfp):
        cls.get_wait_ready_task().cancel_wait(sfp.sdk_index)
        
    @classmethod
    def action_on_power_limit_error(cls, sfp):
        logger.log_info(f'SFP {sfp.sdk_index} is powered off due to exceeding power limit')
        sfp.set_power(False)
        sfp.set_hw_reset(0)
    
    @classmethod
    def get_wait_ready_task(cls):
        """Get SFP wait ready task. Create if not exists.

        Returns:
            object: an instance of WaitSfpReadyTask
        """
        if not cls.wait_ready_task:
            from .wait_sfp_ready_task import WaitSfpReadyTask
            cls.wait_ready_task = WaitSfpReadyTask()
        return cls.wait_ready_task
    
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
        SFP.action_table[action_name](self)
    
    def on_event(self, event):
        """Called when a state machine event arrives

        Args:
            event (str): State machine event
        """
        SFP.get_state_machine().on_event(self, event)
        
    def in_stable_state(self):
        """Indicate whether this module is in a stable state. 'Stable state' means the module is pending on a polling event
        from SDK.

        Returns:
            bool: True if the module is in a stable state
        """
        return self.state in (STATE_NOT_PRESENT, STATE_SW_CONTROL, STATE_FW_CONTROL, 
                              STATE_POWER_BAD, STATE_POWER_LIMIT_ERROR, STATE_FCP_NOT_PRESENT,
                              STATE_FCP_PRESENT)
        
    def get_fds_for_poling(self):            
        if self.state == STATE_FW_CONTROL or self.state == STATE_FCP_NOT_PRESENT or self.state == STATE_FCP_PRESENT:
            return {
                'present': self.get_fd('present')
            } 
        else:
            return {
                'hw_present': self.get_fd('hw_present'),
                'power_good': self.get_fd('power_good')
            } 
    
    def fill_change_event(self, port_dict):
        """Fill change event data based on current state.

        Args:
            port_dict (dict): {<sfp_index>:<sfp_state>}
        """
        if self.state == STATE_NOT_PRESENT or self.state == STATE_FCP_NOT_PRESENT:
            port_dict[self.sdk_index + 1] = SFP_STATUS_REMOVED
        elif self.state == STATE_SW_CONTROL or self.state == STATE_FW_CONTROL or self.state == STATE_FCP_PRESENT:
            port_dict[self.sdk_index + 1] = SFP_STATUS_INSERTED
        elif self.state == STATE_POWER_BAD or self.state == STATE_POWER_LIMIT_ERROR:
            sfp_state = SFP.SFP_ERROR_BIT_POWER_BUDGET_EXCEEDED | SFP.SFP_STATUS_BIT_INSERTED
            port_dict[self.sdk_index + 1] = str(sfp_state)
            
    def refresh_poll_obj(self, poll_obj, all_registered_fds):
        """Refresh polling object and registered fds. This function is usually called when a cable plugin
        event occurs. For example, user plugs out a software control module and replaces with a firmware
        control cable. In such case, poll_obj was polling "hw_present" and "power_good" for software control,
        and it needs to be changed to poll "present" for new control type which is firmware control.

        Args:
            poll_obj (object): poll object
            all_registered_fds (dict): fds that have been registered to poll object
        """
        # find fds registered by this SFP
        current_registered_fds = {item[2]: (fileno, item[1]) for fileno, item in all_registered_fds.items() if item[0] == self.sdk_index}
        logger.log_debug(f'SFP {self.sdk_index} registered fds are: {current_registered_fds}')
        if self.state == STATE_FW_CONTROL or self.state == STATE_FCP_NOT_PRESENT or self.state == STATE_FCP_PRESENT:
            target_poll_types = ['present']
        else:
            target_poll_types = ['hw_present', 'power_good']
            
        for target_poll_type in target_poll_types:
            if target_poll_type not in current_registered_fds:
                # need add new fd for polling
                logger.log_debug(f'SFP {self.sdk_index} is registering file descriptor: {target_poll_type}')
                fd = self.get_fd(target_poll_type)
                poll_obj.register(fd, select.POLLERR | select.POLLPRI)
                all_registered_fds[fd.fileno()] = (self.sdk_index, fd, target_poll_type)
            else:
                # the fd is already in polling
                current_registered_fds.pop(target_poll_type)

        for _, item in current_registered_fds.items():
            # Deregister poll, close fd
            logger.log_debug(f'SFP {self.sdk_index} is de-registering file descriptor: {item}')
            poll_obj.unregister(item[1])
            all_registered_fds.pop(item[0])
            item[1].close()

    def is_dummy_event(self, fd_type, fd_value):
        """Check whether an event is dummy event

        Args:
            origin_state (str): original state before polling
            fd_type (str): polling sysfs type
            fd_value (int): polling sysfs value

        Returns:
            bool: True if the event is a dummy event
        """
        if fd_type == 'hw_present' or fd_type == 'present':
            if fd_value == int(SFP_STATUS_INSERTED):
                return self.state in (STATE_SW_CONTROL, STATE_FW_CONTROL, STATE_POWER_BAD,
                                      STATE_POWER_LIMIT_ERROR, STATE_FCP_PRESENT)
            elif fd_value == int(SFP_STATUS_REMOVED):
                return self.state in (STATE_NOT_PRESENT, STATE_FCP_NOT_PRESENT)
        elif fd_type == 'power_good':
            if fd_value == 1:
                return self.state in (STATE_SW_CONTROL, STATE_NOT_PRESENT, STATE_RESETTING)
            else:
                return self.state in (STATE_POWER_BAD, STATE_POWER_LIMIT_ERROR, STATE_NOT_PRESENT)
        return False

    @classmethod
    def initialize_sfp_modules(cls, sfp_list):
        """Initialize all modules. Only applicable when module host management is enabled

        Args:
            sfp_list (object): all sfps
        """
        wait_ready_task = cls.get_wait_ready_task()
        wait_ready_task.start()
        
        for s in sfp_list:
            s.on_event(EVENT_START)
            
        if not wait_ready_task.empty():
            # Wait until wait_ready_task is up
            while not wait_ready_task.is_alive():
                pass

            # Resetting SFP requires a reloading of module firmware, it takes up to 3 seconds
            # according to standard
            max_wait_time = 3.5
            begin = time.monotonic()
            while True:
                ready_sfp_set = wait_ready_task.get_ready_set()
                for sfp_index in ready_sfp_set:
                    s = sfp_list[sfp_index]
                    logger.log_debug(f'SFP {sfp_index} is recovered from resetting state')
                    s.on_event(EVENT_RESET_DONE)
                elapse = time.monotonic() - begin
                if elapse < max_wait_time:
                    time.sleep(0.5)
                else:
                    break

        # Verify that all modules are in a stable state
        for index, s in enumerate(sfp_list):
            if not s.in_stable_state():
                logger.log_error(f'SFP {index} is not in stable state after initializing, state={s.state}')
            logger.log_notice(f'SFP {index} is in state {s.state} after module initialization')

        cls.wait_sfp_eeprom_ready(sfp_list, 2)
        
class RJ45Port(NvidiaSFPCommon):
    """class derived from SFP, representing RJ45 ports"""

    def __init__(self, sfp_index):
        super(RJ45Port, self).__init__(sfp_index)
        self.sfp_type = RJ45_TYPE

    def get_presence(self):
        """
        Retrieves the presence of the device
        For RJ45 ports, it always return True

        Returns:
            bool: True if device is present, False if not
        """
        file_path = SFP_SDK_MODULE_SYSFS_ROOT_TEMPLATE.format(self.sdk_index) + SFP_SYSFS_PRESENT
        present = utils.read_int_from_file(file_path)
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


class CpoPort(SFP):
    """class derived from SFP, representing CPO ports"""

    def __init__(self, sfp_index):
        super(CpoPort, self).__init__(sfp_index)
        self._sfp_type_str = None
        self.sfp_type = CPO_TYPE

    def get_transceiver_info(self):
        transceiver_info_dict = super().get_transceiver_info()
        transceiver_info_dict['type'] = self.sfp_type
        return transceiver_info_dict

    def get_xcvr_api(self):
        if self._xcvr_api is None:
            self._xcvr_api = self._xcvr_api_factory._create_api(cmis_codes.CmisCodes, cmis_mem.CmisMemMap, cmis_api.CmisApi)
        return self._xcvr_api

    def get_presence(self):
        file_path = SFP_SDK_MODULE_SYSFS_ROOT_TEMPLATE.format(self.sdk_index) + SFP_SYSFS_PRESENT
        present = utils.read_int_from_file(file_path)
        return present == 1

    def reinit(self):
        """
        Nothing to do for cpo. Just provide it to avoid exception
        :return:
        """
        return


