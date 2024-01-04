#
# Copyright (c) 2019-2023 NVIDIA CORPORATION & AFFILIATES.
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
    import subprocess
    import os
    import threading
    from sonic_py_common.logger import Logger
    from sonic_py_common.general import check_output_pipe
    from . import utils
    from .device_data import DeviceDataManager
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_platform_base.sonic_xcvr.fields import consts
    from sonic_platform_base.sonic_xcvr.api.public import sff8636, sff8436

except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

try:
    # python_sdk_api does not support python3 for now. Daemons like thermalctld or psud
    # also import this file without actually use the sdk lib. So we catch the ImportError
    # and ignore it here. Meanwhile, we have to trigger xcvrd using python2 now because it
    # uses the sdk lib.
    from python_sdk_api.sxd_api import *
    from python_sdk_api.sx_api import *
except ImportError as e:
    pass

# Define the sdk constants
SX_PORT_MODULE_STATUS_INITIALIZING = 0
SX_PORT_MODULE_STATUS_PLUGGED = 1
SX_PORT_MODULE_STATUS_UNPLUGGED = 2
SX_PORT_MODULE_STATUS_PLUGGED_WITH_ERROR = 3
SX_PORT_MODULE_STATUS_PLUGGED_DISABLED = 4

try:
    if os.environ["PLATFORM_API_UNIT_TESTING"] == "1":
        # Unable to import SDK constants under unit test
        # Define them here
        SX_PORT_ADMIN_STATUS_UP = True
        SX_PORT_ADMIN_STATUS_DOWN = False
except KeyError:
    pass

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

# parameters for SFP presence
SFP_STATUS_INSERTED = '1'

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
POWER_MODE_POLICY_AUTO = 2
POWER_MODE_LOW = 1
# POWER_MODE_HIGH = 2  # not used

# SFP type constants
SFP_TYPE_CMIS = 'cmis'
SFP_TYPE_SFF8472 = 'sff8472'
SFP_TYPE_SFF8636 = 'sff8636'

# SFP stderr
SFP_EEPROM_NOT_AVAILABLE = 'Input/output error'

SFP_DEFAULT_TEMP_WARNNING_THRESHOLD = 70.0
SFP_DEFAULT_TEMP_CRITICAL_THRESHOLD = 80.0
SFP_TEMPERATURE_SCALE = 8.0

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


# SDK initializing stuff, called from chassis
def initialize_sdk_handle():
    rc, sdk_handle = sx_api_open(None)
    if (rc != SX_STATUS_SUCCESS):
        logger.log_warning("Failed to open api handle, please check whether SDK is running.")
        sdk_handle = None

    return sdk_handle


def deinitialize_sdk_handle(sdk_handle):
    if sdk_handle is not None:
        rc = sx_api_close(sdk_handle)
        if (rc != SX_STATUS_SUCCESS):
            logger.log_warning("Failed to close api handle.")

        return rc == SXD_STATUS_SUCCESS
    else:
         logger.log_warning("Sdk handle is none")
         return False

class SdkHandleContext(object):
    def __init__(self):
        self.sdk_handle = None

    def __enter__(self):
        self.sdk_handle = initialize_sdk_handle()
        return self.sdk_handle

    def __exit__(self, exc_type, exc_val, exc_tb):
        deinitialize_sdk_handle(self.sdk_handle)

class NvidiaSFPCommon(SfpOptoeBase):
    sfp_index_to_logical_port_dict = {}
    sfp_index_to_logical_lock = threading.Lock()
    
    def __init__(self, sfp_index):
        super(NvidiaSFPCommon, self).__init__()
        self.index = sfp_index + 1
        self.sdk_index = sfp_index

    @property
    def sdk_handle(self):
        if not SFP.shared_sdk_handle:
            SFP.shared_sdk_handle = initialize_sdk_handle()
            if not SFP.shared_sdk_handle:
                logger.log_error('Failed to open SDK handle')
        return SFP.shared_sdk_handle

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
    
    @classmethod
    def get_sfp_index_to_logical_port(cls, force=False):
        if not cls.sfp_index_to_logical_port_dict or force:
            config_db = utils.DbUtils.get_db_instance('CONFIG_DB')
            port_data = config_db.get_table('PORT')
            for key, data in port_data.items():
                if data['index'] not in cls.sfp_index_to_logical_port_dict:
                    cls.sfp_index_to_logical_port_dict[int(data['index']) - 1] = key
    
    @classmethod
    def get_logical_port_by_sfp_index(cls, sfp_index):
        with cls.sfp_index_to_logical_lock:
            cls.get_sfp_index_to_logical_port()
            logical_port_name = cls.sfp_index_to_logical_port_dict.get(sfp_index)
            if not logical_port_name:
                cls.get_sfp_index_to_logical_port(force=True)
            else:
                config_db = utils.DbUtils.get_db_instance('CONFIG_DB')
                current_index = int(config_db.get('CONFIG_DB', f'PORT|{logical_port_name}', 'index'))
                if current_index != sfp_index:
                    cls.get_sfp_index_to_logical_port(force=True)
                    logical_port_name = cls.sfp_index_to_logical_port_dict.get(sfp_index)
            return logical_port_name
  

class SFP(NvidiaSFPCommon):
    """Platform-specific SFP class"""
    shared_sdk_handle = None
    SFP_MLNX_ERROR_DESCRIPTION_LONGRANGE_NON_MLNX_CABLE = 'Long range for non-Mellanox cable or module'
    SFP_MLNX_ERROR_DESCRIPTION_ENFORCE_PART_NUMBER_LIST = 'Enforce part number list'
    SFP_MLNX_ERROR_DESCRIPTION_PMD_TYPE_NOT_ENABLED = 'PMD type not enabled'
    SFP_MLNX_ERROR_DESCRIPTION_PCIE_POWER_SLOT_EXCEEDED = 'PCIE system power slot exceeded'
    SFP_MLNX_ERROR_DESCRIPTION_RESERVED = 'Reserved'

    SFP_MLNX_ERROR_BIT_LONGRANGE_NON_MLNX_CABLE = 0x00010000
    SFP_MLNX_ERROR_BIT_ENFORCE_PART_NUMBER_LIST = 0x00020000
    SFP_MLNX_ERROR_BIT_PMD_TYPE_NOT_ENABLED = 0x00040000
    SFP_MLNX_ERROR_BIT_PCIE_POWER_SLOT_EXCEEDED = 0x00080000
    SFP_MLNX_ERROR_BIT_RESERVED = 0x80000000

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

    def reinit(self):
        """
        Re-initialize this SFP object when a new SFP inserted
        :return:
        """
        self._sfp_type_str = None
        self.refresh_xcvr_api()

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        try:
            self.is_sw_control()
        except:
            return False
        eeprom_raw = self._read_eeprom(0, 1, log_on_error=False)
        return eeprom_raw is not None

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
        _, page, page_offset = self._get_page_and_page_offset(offset)
        if not page:
            return None

        try:
            with open(page, mode='rb', buffering=0) as f:
                f.seek(page_offset)
                content = f.read(num_bytes)
                if ctypes.get_errno() != 0:
                    raise IOError(f'errno = {os.strerror(ctypes.get_errno())}')
        except (OSError, IOError) as e:
            if log_on_error:
                logger.log_warning(f'Failed to read sfp={self.sdk_index} EEPROM page={page}, page_offset={page_offset}, \
                    size={num_bytes}, offset={offset}, error = {e}')
            return None

        return bytearray(content)

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
                if ret != num_bytes:
                    raise IOError(f'write return code = {ret}')
                if ctypes.get_errno() != 0:
                    raise IOError(f'errno = {os.strerror(ctypes.get_errno())}')
        except (OSError, IOError) as e:
            data = ''.join('{:02x}'.format(x) for x in write_buffer)
            logger.log_error(f'Failed to write EEPROM data sfp={self.sdk_index} EEPROM page={page}, page_offset={page_offset}, size={num_bytes}, \
                offset={offset}, data = {data}, error = {e}')
            return False
        return True

    @classmethod
    def mgmt_phy_mod_pwr_attr_get(cls, power_attr_type, sdk_handle, sdk_index, slot_id):
        sx_mgmt_phy_mod_pwr_attr_p = new_sx_mgmt_phy_mod_pwr_attr_t_p()
        sx_mgmt_phy_mod_pwr_attr = sx_mgmt_phy_mod_pwr_attr_t()
        sx_mgmt_phy_mod_pwr_attr.power_attr_type = power_attr_type
        sx_mgmt_phy_mod_pwr_attr_t_p_assign(sx_mgmt_phy_mod_pwr_attr_p, sx_mgmt_phy_mod_pwr_attr)
        module_id_info = sx_mgmt_module_id_info_t()
        module_id_info.slot_id = slot_id
        module_id_info.module_id = sdk_index
        try:
            rc = sx_mgmt_phy_module_pwr_attr_get(sdk_handle, module_id_info, sx_mgmt_phy_mod_pwr_attr_p)
            assert SX_STATUS_SUCCESS == rc, "sx_mgmt_phy_module_pwr_attr_get failed {}".format(rc)
            sx_mgmt_phy_mod_pwr_attr = sx_mgmt_phy_mod_pwr_attr_t_p_value(sx_mgmt_phy_mod_pwr_attr_p)
            pwr_mode_attr = sx_mgmt_phy_mod_pwr_attr.pwr_mode_attr
            return pwr_mode_attr.admin_pwr_mode_e, pwr_mode_attr.oper_pwr_mode_e
        finally:
            delete_sx_mgmt_phy_mod_pwr_attr_t_p(sx_mgmt_phy_mod_pwr_attr_p)

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
            elif DeviceDataManager.is_independent_mode():
                file_path = SFP_SDK_MODULE_SYSFS_ROOT_TEMPLATE.format(self.sdk_index) + SFP_SYSFS_POWER_MODE
                power_mode = utils.read_int_from_file(file_path)
                return power_mode == POWER_MODE_LOW
        except Exception as e:
            print(e)
            return False

        if utils.is_host():
            # To avoid performance issue,
            # call class level method to avoid initialize the whole sonic platform API
            get_lpmode_code = 'from sonic_platform import sfp;\n' \
                              'with sfp.SdkHandleContext() as sdk_handle:' \
                              'print(sfp.SFP._get_lpmode(sdk_handle, {}, {}))'.format(self.sdk_index, self.slot_id)
            lpm_cmd = ["docker", "exec", "pmon", "python3", "-c", get_lpmode_code]
            try:
                output = subprocess.check_output(lpm_cmd, universal_newlines=True)
                return 'True' in output
            except subprocess.CalledProcessError as e:
                print("Error! Unable to get LPM for {}, rc = {}, err msg: {}".format(self.sdk_index, e.returncode, e.output))
                return False
        else:
            return self._get_lpmode(self.sdk_handle, self.sdk_index, self.slot_id)

    @classmethod
    def _get_lpmode(cls, sdk_handle, sdk_index, slot_id):
        """Class level method to get low power mode.

        Args:
            sdk_handle: SDK handle
            sdk_index (integer): SDK port index
            slot_id (integer): Slot ID

        Returns:
            [boolean]: True if low power mode is on else off
        """
        _, oper_pwr_mode = cls.mgmt_phy_mod_pwr_attr_get(SX_MGMT_PHY_MOD_PWR_ATTR_PWR_MODE_E, sdk_handle, sdk_index, slot_id)
        return oper_pwr_mode == SX_MGMT_PHY_MOD_PWR_MODE_LOW_E

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


    @classmethod
    def is_nve(cls, port):
        return (port & NVE_MASK) != 0


    @classmethod
    def is_cpu(cls, port):
        return (port & CPU_MASK) != 0


    @classmethod
    def _fetch_port_status(cls, sdk_handle, log_port):
        oper_state_p = new_sx_port_oper_state_t_p()
        admin_state_p = new_sx_port_admin_state_t_p()
        module_state_p = new_sx_port_module_state_t_p()
        rc = sx_api_port_state_get(sdk_handle, log_port, oper_state_p, admin_state_p, module_state_p)
        assert rc == SXD_STATUS_SUCCESS, "sx_api_port_state_get failed, rc = %d" % rc

        admin_state = sx_port_admin_state_t_p_value(admin_state_p)
        oper_state = sx_port_oper_state_t_p_value(oper_state_p)

        delete_sx_port_oper_state_t_p(oper_state_p)
        delete_sx_port_admin_state_t_p(admin_state_p)
        delete_sx_port_module_state_t_p(module_state_p)

        return oper_state, admin_state


    @classmethod
    def is_port_admin_status_up(cls, sdk_handle, log_port):
        _, admin_state = cls._fetch_port_status(sdk_handle, log_port);
        return admin_state == SX_PORT_ADMIN_STATUS_UP


    @classmethod
    def set_port_admin_status_by_log_port(cls, sdk_handle, log_port, admin_status):
        rc = sx_api_port_state_set(sdk_handle, log_port, admin_status)
        if SX_STATUS_SUCCESS != rc:
            logger.log_error("sx_api_port_state_set failed, rc = %d" % rc)

        return SX_STATUS_SUCCESS == rc


    @classmethod
    def get_logical_ports(cls, sdk_handle, sdk_index, slot_id):
        # Get all the ports related to the sfp, if port admin status is up, put it to list
        port_cnt_p = new_uint32_t_p()
        uint32_t_p_assign(port_cnt_p, 0)
        rc = sx_api_port_device_get(sdk_handle, DEVICE_ID, SWITCH_ID, None,  port_cnt_p)

        assert rc == SX_STATUS_SUCCESS, "sx_api_port_device_get failed, rc = %d" % rc
        port_cnt = uint32_t_p_value(port_cnt_p)
        port_attributes_list = new_sx_port_attributes_t_arr(port_cnt)

        rc = sx_api_port_device_get(sdk_handle, DEVICE_ID , SWITCH_ID, port_attributes_list,  port_cnt_p)
        assert rc == SX_STATUS_SUCCESS, "sx_api_port_device_get failed, rc = %d" % rc

        port_cnt = uint32_t_p_value(port_cnt_p)
        log_port_list = []
        for i in range(0, port_cnt):
            port_attributes = sx_port_attributes_t_arr_getitem(port_attributes_list, i)
            if not cls.is_nve(int(port_attributes.log_port)) \
               and not cls.is_cpu(int(port_attributes.log_port)) \
               and port_attributes.port_mapping.module_port == sdk_index \
               and port_attributes.port_mapping.slot == slot_id \
               and cls.is_port_admin_status_up(sdk_handle, port_attributes.log_port):
                log_port_list.append(port_attributes.log_port)

        delete_sx_port_attributes_t_arr(port_attributes_list)
        delete_uint32_t_p(port_cnt_p)
        return log_port_list


    @classmethod
    def mgmt_phy_mod_pwr_attr_set(cls, sdk_handle, sdk_index, slot_id, power_attr_type, admin_pwr_mode):
        result = False
        sx_mgmt_phy_mod_pwr_attr = sx_mgmt_phy_mod_pwr_attr_t()
        sx_mgmt_phy_mod_pwr_mode_attr = sx_mgmt_phy_mod_pwr_mode_attr_t()
        sx_mgmt_phy_mod_pwr_attr.power_attr_type = power_attr_type
        sx_mgmt_phy_mod_pwr_mode_attr.admin_pwr_mode_e = admin_pwr_mode
        sx_mgmt_phy_mod_pwr_attr.pwr_mode_attr = sx_mgmt_phy_mod_pwr_mode_attr
        sx_mgmt_phy_mod_pwr_attr_p = new_sx_mgmt_phy_mod_pwr_attr_t_p()
        sx_mgmt_phy_mod_pwr_attr_t_p_assign(sx_mgmt_phy_mod_pwr_attr_p, sx_mgmt_phy_mod_pwr_attr)
        module_id_info = sx_mgmt_module_id_info_t()
        module_id_info.slot_id = slot_id
        module_id_info.module_id = sdk_index
        try:
            rc = sx_mgmt_phy_module_pwr_attr_set(sdk_handle, SX_ACCESS_CMD_SET, module_id_info, sx_mgmt_phy_mod_pwr_attr_p)
            if SX_STATUS_SUCCESS != rc:
                logger.log_error("Error occurred when setting power mode for SFP module {}, slot {}, error code {}".format(sdk_index, slot_id, rc))
                result = False
            else:
                result = True
        finally:
            delete_sx_mgmt_phy_mod_pwr_attr_t_p(sx_mgmt_phy_mod_pwr_attr_p)

        return result


    @classmethod
    def _set_lpmode_raw(cls, sdk_handle, sdk_index, slot_id, ports, attr_type, power_mode):
        result = False
        # Check if the module already works in the same mode
        admin_pwr_mode, oper_pwr_mode = cls.mgmt_phy_mod_pwr_attr_get(attr_type, sdk_handle, sdk_index, slot_id)
        if (power_mode == SX_MGMT_PHY_MOD_PWR_MODE_LOW_E and oper_pwr_mode == SX_MGMT_PHY_MOD_PWR_MODE_LOW_E) \
           or (power_mode == SX_MGMT_PHY_MOD_PWR_MODE_AUTO_E and admin_pwr_mode == SX_MGMT_PHY_MOD_PWR_MODE_AUTO_E):
            return True
        try:
            # Bring the port down
            for port in ports:
                cls.set_port_admin_status_by_log_port(sdk_handle, port, SX_PORT_ADMIN_STATUS_DOWN)
            # Set the desired power mode
            result = cls.mgmt_phy_mod_pwr_attr_set(sdk_handle, sdk_index, slot_id, attr_type, power_mode)
        finally:
            # Bring the port up
            for port in ports:
                cls.set_port_admin_status_by_log_port(sdk_handle, port, SX_PORT_ADMIN_STATUS_UP)

        return result


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
                return api.get_lpmode() == lpmode
            elif DeviceDataManager.is_independent_mode():
                # FW control under CMIS host management mode. 
                # Currently, we don't support set LPM under this mode.
                # Just return False to indicate set Fail
                return False
        except Exception as e:
            print(e)
            return False

        if utils.is_host():
            # To avoid performance issue,
            # call class level method to avoid initialize the whole sonic platform API
            set_lpmode_code = 'from sonic_platform import sfp;\n' \
                              'with sfp.SdkHandleContext() as sdk_handle:' \
                              'print(sfp.SFP._set_lpmode({}, sdk_handle, {}, {}))' \
                              .format('True' if lpmode else 'False', self.sdk_index, self.slot_id)
            lpm_cmd = ["docker", "exec", "pmon", "python3", "-c", set_lpmode_code]

            # Set LPM
            try:
                output = subprocess.check_output(lpm_cmd, universal_newlines=True)
                return 'True' in output
            except subprocess.CalledProcessError as e:
                print("Error! Unable to set LPM for {}, rc = {}, err msg: {}".format(self.sdk_index, e.returncode, e.output))
                return False
        else:
            return self._set_lpmode(lpmode, self.sdk_handle, self.sdk_index, self.slot_id)


    @classmethod
    def _set_lpmode(cls, lpmode, sdk_handle, sdk_index, slot_id):
        log_port_list = cls.get_logical_ports(sdk_handle, sdk_index, slot_id)
        sdk_lpmode = SX_MGMT_PHY_MOD_PWR_MODE_LOW_E if lpmode else SX_MGMT_PHY_MOD_PWR_MODE_AUTO_E
        cls._set_lpmode_raw(sdk_handle,
                            sdk_index,
                            slot_id,
                            log_port_list,
                            SX_MGMT_PHY_MOD_PWR_ATTR_PWR_MODE_E,
                            sdk_lpmode)
        logger.log_info("{} low power mode for module {}, slot {}".format("Enabled" if lpmode else "Disabled", sdk_index, slot_id))
        return True

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
                return 'Not supported'
        except:
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
        return [False] * api.NUM_CHANNELS if api else None

    def get_tx_fault(self):
        """Accessing tx fault is not supproted, return all False

        Returns:
            list: [False] * channels
        """
        api = self.get_xcvr_api()
        return [False] * api.NUM_CHANNELS if api else None

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

        self.reinit()
        temperature = super().get_temperature()
        return temperature if temperature is not None else None

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
        
        support, thresh = self._get_temperature_threshold()
        if support is None or thresh is None:
            # Failed to read from EEPROM
            return None
        if support is False:
            # Do not support
            return 0.0
        return thresh.get(consts.TEMP_HIGH_WARNING_FIELD, SFP_DEFAULT_TEMP_WARNNING_THRESHOLD)

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

        support, thresh = self._get_temperature_threshold()
        if support is None or thresh is None:
            # Failed to read from EEPROM
            return None
        if support is False:
            # Do not support
            return 0.0
        return thresh.get(consts.TEMP_HIGH_ALARM_FIELD, SFP_DEFAULT_TEMP_CRITICAL_THRESHOLD)

    def _get_temperature_threshold(self):
        """Get temperature thresholds data from EEPROM

        Returns:
            tuple: (support, thresh_dict)
        """
        self.reinit()
        api = self.get_xcvr_api()
        if not api:
            return None, None

        thresh_support = api.get_transceiver_thresholds_support()
        if thresh_support:
            if isinstance(api, sff8636.Sff8636Api) or isinstance(api, sff8436.Sff8436Api):
                return thresh_support, api.xcvr_eeprom.read(consts.TEMP_THRESHOLDS_FIELD)
            return thresh_support, api.xcvr_eeprom.read(consts.THRESHOLDS_FIELD)
        else:
            return thresh_support, {}

    def get_xcvr_api(self):
        """
        Retrieves the XcvrApi associated with this SFP

        Returns:
            An object derived from XcvrApi that corresponds to the SFP
        """
        if self._xcvr_api is None:
            self.refresh_xcvr_api()
            if self._xcvr_api is not None:
                self._xcvr_api.get_rx_los = self.get_rx_los
                self._xcvr_api.get_tx_fault = self.get_tx_fault
        return self._xcvr_api

    def is_sw_control(self):
        if not DeviceDataManager.is_independent_mode():
            return False
        
        db = utils.DbUtils.get_db_instance('STATE_DB')
        logical_port = NvidiaSFPCommon.get_logical_port_by_sfp_index(self.sdk_index)
        if not logical_port:
            raise Exception(f'Module {self.sdk_index} is not present or under initialization')
        
        initialized = db.exists('STATE_DB', f'TRANSCEIVER_STATUS|{logical_port}')
        if not initialized:
            raise Exception(f'Module {self.sdk_index} is not present or under initialization')
        
        try:
            return utils.read_int_from_file(f'/sys/module/sx_core/asic0/module{self.sdk_index}/control', 
                                            raise_exception=True, log_func=None) == 1
        except:
            # just in case control file does not exist
            raise Exception(f'Module {self.sdk_index} is under initialization')


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
