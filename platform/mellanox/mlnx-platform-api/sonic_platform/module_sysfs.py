#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
"""The host driving a module through the sx_core driver.

Everything here is reachable through sysfs from the (sdk_index, bank) that
`module_access` points at: presence, control mode, power, reset,
I2C clock, status and the polling file descriptors. Nothing here reads the
module's EEPROM, touches the xcvr api, or knows about the detection flow's
`state` - the module describing itself is the opposite direction and belongs
to the xcvr layer.
"""

import os
from abc import ABC, abstractmethod

from sonic_py_common.logger import Logger
from sonic_platform_base.sfp_base import SfpBase

from . import utils
from .device_data import DeviceDataManager

logger = Logger()

SFP_SYSFS_STATUS = 'status'
SFP_SYSFS_STATUS_ERROR = 'statuserror'
SFP_SYSFS_PRESENT = 'present'
SFP_SYSFS_HW_PRESENT = 'hw_present'
SFP_SYSFS_RESET = 'reset'
SFP_SYSFS_HWRESET = 'hw_reset'
SFP_SYSFS_POWER_MODE = 'power_mode'
SFP_SYSFS_POWER_MODE_POLICY = 'power_mode_policy'
SFP_SYSFS_POWER_ON = 'power_on'
SFP_SYSFS_POWER_GOOD = 'power_good'
SFP_SYSFS_POWER_LIMIT = 'power_limit'
SFP_SYSFS_CONTROL = 'control'
SFP_SYSFS_FREQUENCY = 'frequency'
SFP_SYSFS_FREQUENCY_SUPPORT = 'frequency_support'
SFP_SYSFS_TEMPERATURE_INPUT = 'temperature/input'
SFP_SYSFS_TEMPERATURE_THRESHOLD_HI = 'temperature/threshold_hi'
SFP_SYSFS_TEMPERATURE_THRESHOLD_CRITICAL_HI = 'temperature/threshold_critical_hi'

SFP_TEMPERATURE_SCALE = 8.0

POWER_MODE_POLICY_HIGH = 1
POWER_MODE_POLICY_LOW = 3
POWER_MODE_LOW = 1
# POWER_MODE_HIGH = 2  # not used

# parameters for SFP presence
SFP_STATUS_REMOVED = '0'
SFP_STATUS_INSERTED = '1'
SFP_STATUS_ERROR = '2'
SFP_STATUS_UNKNOWN = '-1'

# The same 'status' leaf as SDK_SFP_STATE_* below, decoded the way
# get_error_description wants it: with a name for the 0 the PMAOS mapping has
# no entry for.
SX_PORT_MODULE_STATUS_INITIALIZING = 0
SX_PORT_MODULE_STATUS_PLUGGED = 1
SX_PORT_MODULE_STATUS_UNPLUGGED = 2
SX_PORT_MODULE_STATUS_PLUGGED_WITH_ERROR = 3
SX_PORT_MODULE_STATUS_PLUGGED_DISABLED = 4

# SFP status from PMAOS register
# 0x1 plug in
# 0x2 plug out
# 0x3 plug in with error
# 0x4 disabled, at this status SFP eeprom is not accessible,
#     and presence status also will be not present,
#     so treate it as plug out.
SDK_SFP_STATE_IN = 0x1
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

# SFP errors that will block eeprom accessing
SDK_SFP_BLOCKING_ERRORS = [
    0x2,  # SFP.SFP_ERROR_BIT_I2C_STUCK,
    0x3,  # SFP.SFP_ERROR_BIT_BAD_EEPROM,
    0x5,  # SFP.SFP_ERROR_BIT_UNSUPPORTED_CABLE,
    0x6,  # SFP.SFP_ERROR_BIT_HIGH_TEMP,
    0x7,  # SFP.SFP_ERROR_BIT_BAD_CABLE
]


class ModuleSysfsMixin(ABC):
    """sx_core module control, shared by every port class.

    The mixin deliberately has no `__init__`: adding one to a chain that
    already swallows arguments is how the bank stopped reaching the api on the
    CPO branch. Instead it declares `module_access`, the object that owns this
    module's sysfs coordinates and performs the access. A pluggable port is its
    own access object; a CPO port delegates to its OE. Because the property is
    abstract, a branch that forgets to answer it fails at construction rather
    than at the first sysfs read.

    Declare this mixin before the community bases, or `SfpOptoeBase.set_power`
    wins over the module-side method of the same name.
    """

    SFP_MLNX_ERROR_DESCRIPTION_LONGRANGE_NON_MLNX_CABLE = 'Long range for non-Mellanox cable or module'
    SFP_MLNX_ERROR_DESCRIPTION_ENFORCE_PART_NUMBER_LIST = 'Enforce part number list'
    SFP_MLNX_ERROR_DESCRIPTION_PMD_TYPE_NOT_ENABLED = 'PMD type not enabled'
    SFP_MLNX_ERROR_DESCRIPTION_PCIE_POWER_SLOT_EXCEEDED = 'PCIE system power slot exceeded'
    SFP_MLNX_ERROR_DESCRIPTION_BOOT_ERROR = 'Module boot failed'
    SFP_MLNX_ERROR_DESCRIPTION_RECOVERY_ERROR = 'Module entered firmware recovery mode'
    SFP_MLNX_ERROR_DESCRIPTION_SUBMODULE_FAILURE = 'Internal submodule failure detected'
    SFP_MLNX_ERROR_DESCRIPTION_ELS_CRITICAL_INDICATION = 'Critical ELS fault detected'
    SFP_MLNX_ERROR_DESCRIPTION_RESERVED = 'Reserved'

    SDK_ERRORS_TO_DESCRIPTION = {
        0x1: SFP_MLNX_ERROR_DESCRIPTION_LONGRANGE_NON_MLNX_CABLE,
        0x4: SFP_MLNX_ERROR_DESCRIPTION_ENFORCE_PART_NUMBER_LIST,
        0x8: SFP_MLNX_ERROR_DESCRIPTION_PMD_TYPE_NOT_ENABLED,
        0xc: SFP_MLNX_ERROR_DESCRIPTION_PCIE_POWER_SLOT_EXCEEDED,
        0xf: SFP_MLNX_ERROR_DESCRIPTION_BOOT_ERROR,
        0x10: SFP_MLNX_ERROR_DESCRIPTION_RECOVERY_ERROR,
        0x11: SFP_MLNX_ERROR_DESCRIPTION_SUBMODULE_FAILURE,
        0x13: SFP_MLNX_ERROR_DESCRIPTION_ELS_CRITICAL_INDICATION,
    }

    SFP_MLNX_ERROR_BIT_LONGRANGE_NON_MLNX_CABLE = 0x00010000
    SFP_MLNX_ERROR_BIT_ENFORCE_PART_NUMBER_LIST = 0x00020000
    SFP_MLNX_ERROR_BIT_PMD_TYPE_NOT_ENABLED = 0x00040000
    SFP_MLNX_ERROR_BIT_PCIE_POWER_SLOT_EXCEEDED = 0x00080000
    SFP_MLNX_ERROR_BIT_BOOT_ERROR = 0x00100000
    SFP_MLNX_ERROR_BIT_RECOVERY_ERROR = 0x00200000
    SFP_MLNX_ERROR_BIT_SUBMODULE_FAILURE = 0x00400000
    SFP_MLNX_ERROR_BIT_ELS_CRITICAL_INDICATION = 0x00800000
    SFP_MLNX_ERROR_BIT_RESERVED = 0x80000000

    SDK_ERRORS_TO_ERROR_BITS = {
        0x0: SfpBase.SFP_ERROR_BIT_POWER_BUDGET_EXCEEDED,
        0x1: SFP_MLNX_ERROR_BIT_LONGRANGE_NON_MLNX_CABLE,
        0x2: SfpBase.SFP_ERROR_BIT_I2C_STUCK,
        0x3: SfpBase.SFP_ERROR_BIT_BAD_EEPROM,
        0x4: SFP_MLNX_ERROR_BIT_ENFORCE_PART_NUMBER_LIST,
        0x5: SfpBase.SFP_ERROR_BIT_UNSUPPORTED_CABLE,
        0x6: SfpBase.SFP_ERROR_BIT_HIGH_TEMP,
        0x7: SfpBase.SFP_ERROR_BIT_BAD_CABLE,
        0x8: SFP_MLNX_ERROR_BIT_PMD_TYPE_NOT_ENABLED,
        0xc: SFP_MLNX_ERROR_BIT_PCIE_POWER_SLOT_EXCEEDED,
        0xf: SFP_MLNX_ERROR_BIT_BOOT_ERROR,
        0x10: SFP_MLNX_ERROR_BIT_RECOVERY_ERROR,
        0x11: SFP_MLNX_ERROR_BIT_SUBMODULE_FAILURE,
        0x13: SFP_MLNX_ERROR_BIT_ELS_CRITICAL_INDICATION,
    }

    @property
    @abstractmethod
    def module_access(self):
        """Object owning this module's sysfs coordinates and access."""
        raise NotImplementedError

    def get_sdk_index(self):
        return self.module_access.sdk_index

    def get_asic_index(self):
        """Which ASIC this module belongs to.

        Not a component of any sx_core path - those all sit under 'asic0'. This
        answers hw-management, whose tree really is split per ASIC.
        """
        return self.module_access.asic_index

    def _module_attr_path(self, name):
        return self.module_access.module_attr_path(name)

    def _get_module_info(self):
        """
        Get oper state and error code of the SFP module

        Returns:
            The oper state and error code fetched from sysfs
        """
        oper_state = utils.read_int_from_file(self._module_attr_path(SFP_SYSFS_STATUS))
        error_type = utils.read_int_from_file(self._module_attr_path(SFP_SYSFS_STATUS_ERROR))
        return oper_state, error_type

    def get_fd(self, fd_type):
        path = self._module_attr_path(fd_type)
        try:
            return open(path)
        except FileNotFoundError:
            logger.log_warning(f'Trying to access {path} file which does not exist')
            return None

    def get_fd_for_polling_legacy(self):
        """Get polling fds for when module host management is disabled

        Returns:
            object: file descriptor of present
        """
        return self.get_fd(SFP_SYSFS_PRESENT)

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
        status = utils.read_int_from_file(self._module_attr_path(SFP_SYSFS_STATUS))
        return SDK_STATUS_TO_SONIC_STATUS[status]

    def get_error_info_from_sdk_error_type(self):
        """Translate SDK error type to SONiC error state and error description. Only calls
        when sysfs "present" returns "2".

        Returns:
            tuple: (error state, error description)
        """
        error_type = utils.read_int_from_file(self._module_attr_path(SFP_SYSFS_STATUS_ERROR), default=-1)
        sfp_state_bits = ModuleSysfsMixin.SDK_ERRORS_TO_ERROR_BITS.get(error_type)
        if sfp_state_bits is None:
            logger.log_error(f"Unrecognized error {error_type} detected on SFP {self.get_sdk_index()}")
            return SFP_STATUS_ERROR, "Unknown error ({})".format(error_type)

        if error_type in SDK_SFP_BLOCKING_ERRORS:
            # In SFP at error status case, need to overwrite the sfp_state with the exact error code
            sfp_state_bits |= SfpBase.SFP_ERROR_BIT_BLOCKING

        # An error should be always set along with 'INSERTED'
        sfp_state_bits |= SfpBase.SFP_STATUS_BIT_INSERTED

        # For vendor specific errors, the description should be returned as well
        error_description = ModuleSysfsMixin.SDK_ERRORS_TO_DESCRIPTION.get(error_type)
        sfp_state = str(sfp_state_bits)
        return sfp_state, error_description

    def is_sw_control(self):
        if not DeviceDataManager.is_module_host_management_mode():
            return False
        try:
            return utils.read_int_from_file(self._module_attr_path(SFP_SYSFS_CONTROL),
                                            raise_exception=True, log_func=None) == 1
        except Exception:
            # just in case control file does not exist
            raise Exception(f'control sysfs for SFP {self.get_sdk_index()} does not exist')

    def get_control_type(self):
        """Get control type of this module, only applicable on host management mode

        Returns:
            int: 1 - software control, 0 - firmware control
        """
        return utils.read_int_from_file(self._module_attr_path(SFP_SYSFS_CONTROL))

    def set_control_type(self, control_type):
        """Set control type for the module

        Args:
            control_type (int): 0 for firmware control, currently only 0 is allowed
        """
        utils.write_file(self._module_attr_path(SFP_SYSFS_CONTROL), control_type)

    def get_hw_present(self):
        """Get hardware present status, only applicable on host management mode

        Returns:
            bool: True if module is in the cage
        """
        return utils.read_int_from_file(self._module_attr_path(SFP_SYSFS_HW_PRESENT)) == 1

    def get_power_on(self):
        """Get power on status, only applicable on host management mode

        Returns:
            bool: True if the module is powered on
        """
        return utils.read_int_from_file(self._module_attr_path(SFP_SYSFS_POWER_ON)) == 1

    def set_power(self, on):
        """Control the power of this module, only applicable on host management mode

        Args:
            on (bool): True if on
        """
        value = 1 if on else 0
        utils.write_file(self._module_attr_path(SFP_SYSFS_POWER_ON), value)

    def get_reset_state(self):
        """Get reset state of this module, only applicable on host management mode

        Returns:
            bool: True if module is not in reset status
        """
        return utils.read_int_from_file(self._module_attr_path(SFP_SYSFS_HWRESET)) == 1

    def set_hw_reset(self, value):
        """Set the module reset status

        Args:
            value (int): 1 for reset, 0 for leaving reset
        """
        utils.write_file(self._module_attr_path(SFP_SYSFS_HWRESET), value)

    def get_power_good(self):
        """Get power good status of this module, only applicable on host management mode

        Returns:
            bool: True if the power is in good status
        """
        return utils.read_int_from_file(self._module_attr_path(SFP_SYSFS_POWER_GOOD)) == 1

    def get_power_limit(self):
        """Get power limit of this module

        Returns:
            int: Power limit in unit of 0.25W
        """
        return utils.read_int_from_file(self._module_attr_path(SFP_SYSFS_POWER_LIMIT))

    def get_frequency_support(self):
        """Get frequency support for this module

        Returns:
            bool: True if supported
        """
        return utils.read_int_from_file(self._module_attr_path(SFP_SYSFS_FREQUENCY_SUPPORT)) == 1

    def set_frequency(self, freqeuncy):
        """Set module frequency.

        Args:
            freqeuncy (int): 0 - up to 400KHz, 1 - up to 1MHz
        """
        utils.write_file(self._module_attr_path(SFP_SYSFS_FREQUENCY), freqeuncy)

    def get_temperature_from_sysfs(self):
        """Read the temperature the driver reports for this module, in Celsius.

        A module under software control also reports a temperature over its own
        EEPROM; choosing between the two is the xcvr layer's business.

        Returns:
            float: temperature, or None if the driver does not report one
        """
        temp_file = self._module_attr_path(SFP_SYSFS_TEMPERATURE_INPUT)
        if not os.path.exists(temp_file):
            logger.log_error(f'Failed to read from file {temp_file} - not exists')
            return None
        temperature = utils.read_int_from_file(temp_file, log_func=None)
        return temperature / SFP_TEMPERATURE_SCALE if temperature is not None else None

    def get_temperature_thresholds_from_sysfs(self):
        """Read the high and critical temperature thresholds the driver reports.

        Returns:
            tuple: (high threshold, critical threshold) in Celsius
        """
        high = utils.read_int_from_file(
            self._module_attr_path(SFP_SYSFS_TEMPERATURE_THRESHOLD_HI), log_func=None)
        critical = utils.read_int_from_file(
            self._module_attr_path(SFP_SYSFS_TEMPERATURE_THRESHOLD_CRITICAL_HI), log_func=None)
        return high / SFP_TEMPERATURE_SCALE, critical / SFP_TEMPERATURE_SCALE
