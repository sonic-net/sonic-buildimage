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
"""The module describing itself over its own EEPROM.

This is the opposite direction from `module_sysfs`: there the host drives the
module through the driver, here the module answers questions about the part it
carries. Anything that has to read the EEPROM or ask the xcvr api lives here,
including the members that consult both - presence, for instance, is a sysfs
bit *and* a readable EEPROM, which is why this layer sits above the sysfs one
and calls down into it rather than beside it.

A handful of members at the bottom answer the same questions from the
databases instead of from the part, because xcvrd has usually already read a
value this process would otherwise read again over I2C - the thermal path asks
for a temperature rather than waking the module.

Two members are deliberately absent, because they are the only thing that
differs between a pluggable port and a CPO port:

  read_eeprom   a pluggable port reads its own module, a CPO port reads the OE
  get_xcvr_api  SfpOptoeBase builds one api, CpoBase builds one per device

Both are required of any class that mixes this in, along with `_read_eeprom`.
The mixin must never *define* read_eeprom, not even as an abstract method: it
sits ahead of the transport in the port MRO, so a name of its own there would
shadow the implementation it is asking for.

The aggregate getters xcvrd reads a port through are written out here rather
than inherited, so the CPO branch answers that surface without taking on
`SfpBase.__init__` and the api machinery re-parenting was meant to shed. A
pluggable port would resolve identical bodies from `SfpOptoeBase`.
"""

import os
import sys

from sonic_py_common.logger import Logger
from swsscommon.swsscommon import ConfigDBConnector
from sonic_platform_base.sfp_base import SfpBase
from sonic_platform_base.sonic_xcvr.fields import consts
from sonic_platform_base.sonic_xcvr.api.public import cmis as cmis_api
from sonic_platform_base.sonic_xcvr.api.public import sff8636, sff8436

from . import utils
from .db_table_helper import get_db_table_helper
from .device_data import DeviceDataManager
from .module_host_mgmt_initializer import get_asic_ready_file_path
from .module_sysfs import (
    POWER_MODE_LOW,
    POWER_MODE_POLICY_HIGH,
    POWER_MODE_POLICY_LOW,
    SFP_SYSFS_HWRESET,
    SFP_SYSFS_HW_PRESENT,
    SFP_SYSFS_POWER_MODE,
    SFP_SYSFS_POWER_MODE_POLICY,
    SFP_SYSFS_PRESENT,
    SFP_SYSFS_RESET,
    SX_PORT_MODULE_STATUS_INITIALIZING,
    SX_PORT_MODULE_STATUS_PLUGGED,
    SX_PORT_MODULE_STATUS_PLUGGED_DISABLED,
    SX_PORT_MODULE_STATUS_PLUGGED_WITH_ERROR,
    SX_PORT_MODULE_STATUS_UNPLUGGED,
)

try:
    sys.path.append('/run/hw-management/bin')
    import hw_management_independent_mode_update
except ImportError:
    # Only mock if running under pytest (check if pytest is imported)
    if 'pytest' in sys.modules:
        from unittest import mock
        hw_management_independent_mode_update = mock.MagicMock()
        hw_management_independent_mode_update.vendor_data_set_module = mock.MagicMock()
    else:
        raise

logger = Logger()

SFP_SW_CONTROL = 1
SFP_FW_CONTROL = 0

CMIS_MAX_POWER_OFFSET = 201
CMIS_MEDIA_INTERFACE_TECH_OFFSET = 212

# 0x0F in offset 212 means 'Copper cable, linear active equalizers', which
# NVIDIA does not support under software control.
CMIS_MEDIA_INTERFACE_TECH_COPPER_LINEAR = 0x0F

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

# Converts the EEPROM's 1W units to the sysfs power limit's 0.25W units.
SFF_POWER_CLASS_TO_QUARTER_WATTS = 4

CMIS_MCI_EEPROM_OFFSET = 2
CMIS_MCI_MASK = 0b00001100

POWER_CLASS_INVALID = -1

CFG_PORT_TABLE = 'PORT'
PORT_CONFIG_DONE = 'PORT_TABLE:PortConfigDone'


class ModuleXcvrMixin:
    """Transceiver behaviour shared by every port that carries a real module.

    Mixed into the port classes that have an EEPROM to read - not into
    `NvidiaSFPCommon`, because an RJ45 port has a cage and a driver but no
    module to interrogate.

    A class mixing this in must provide `read_eeprom`, `_read_eeprom` and
    `get_xcvr_api`, which the mixin calls but deliberately never defines.

    The database-backed members additionally need `index` (the 1-based chassis
    position, which is what CONFIG_DB's port index holds) and `namespace`.
    """

    #: index to logical port, built once from CONFIG_DB and shared by every port.
    port_mapping = {}

    def reinit(self):
        """
        Re-initialize this SFP object when a new SFP inserted
        :return:
        """
        self._sfp_type_str = None
        # Drops the cached api, which on a CPO port means both devices' apis.
        self.remove_xcvr_api()

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        try:
            asic_id = self.asic_id
            asic_id_for_file = "asic" + str(int(asic_id.replace("asic", "")) + 1)
            if utils.read_int_from_file(f'/var/run/hw-management/config/{asic_id_for_file}_ready', log_func=None) == 1:
                if DeviceDataManager.is_module_host_management_mode():
                    if not os.path.exists(get_asic_ready_file_path(asic_id)):
                        return False

                if self.is_sw_control():
                    presence_sysfs = self._module_attr_path(SFP_SYSFS_HW_PRESENT)
                    if utils.read_int_from_file(presence_sysfs, log_func=None) == 1:
                        return True
                else:
                    presence_sysfs = self._module_attr_path(SFP_SYSFS_PRESENT)
                    if utils.read_int_from_file(presence_sysfs, log_func=None) != 0:
                        return self._read_eeprom(0, 1, log_on_error=False) is not None
            return False
        except Exception as e:
            logger.log_warning(f'Failed to check presence of SFP {self.get_sdk_index()}: {e}')
            return False

    def check_eeprom_ready_if_present(self):
        """
        Check if the eeprom is ready for a present SFP

        Returns:
            bool: False if the SFP is present and the eeprom is not ready, True otherwise
        """
        presence_file = SFP_SYSFS_HW_PRESENT if self.is_sw_control() else SFP_SYSFS_PRESENT
        if utils.read_int_from_file(self._module_attr_path(presence_file), log_func=None) == 0:
            return True
        return self._read_eeprom(0, 1, log_on_error=False) is not None

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
            logger.log_error(f'Failed to get lpmode of SFP {self.get_sdk_index()}: {e}')
            return False

        power_mode = utils.read_int_from_file(self._module_attr_path(SFP_SYSFS_POWER_MODE))
        return power_mode == POWER_MODE_LOW

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
            logger.log_error(f'Failed to set lpmode of SFP {self.get_sdk_index()}: {e}')
            return False

        file_path = self._module_attr_path(SFP_SYSFS_POWER_MODE_POLICY)
        target_admin_mode = POWER_MODE_POLICY_LOW if lpmode else POWER_MODE_POLICY_HIGH
        current_admin_mode = utils.read_int_from_file(file_path)
        if current_admin_mode == target_admin_mode:
            return True
        return utils.write_file(file_path, str(target_admin_mode))

    def reset(self):
        """
        Reset SFP and return all user module settings to their default state.

        Returns:
            A boolean, True if successful, False if not

        refer plugins/sfpreset.py
        """
        try:
            if not self.is_sw_control():
                return utils.write_file(self._module_attr_path(SFP_SYSFS_RESET), '1')
            else:
                file_path = self._module_attr_path(SFP_SYSFS_HWRESET)
                return utils.write_file(file_path, '0') and utils.write_file(file_path, '1')
        except Exception as e:
            logger.log_error(f'Failed to reset module - {e}')
            return False

    @classmethod
    def _get_error_description_dict(cls):
        # The generic descriptions are named on SfpBase rather than on cls,
        # since a CPO port is not one.
        return {0: SfpBase.SFP_ERROR_DESCRIPTION_POWER_BUDGET_EXCEEDED,
                1: cls.SFP_MLNX_ERROR_DESCRIPTION_LONGRANGE_NON_MLNX_CABLE,
                2: SfpBase.SFP_ERROR_DESCRIPTION_I2C_STUCK,
                3: SfpBase.SFP_ERROR_DESCRIPTION_BAD_EEPROM,
                4: cls.SFP_MLNX_ERROR_DESCRIPTION_ENFORCE_PART_NUMBER_LIST,
                5: SfpBase.SFP_ERROR_DESCRIPTION_UNSUPPORTED_CABLE,
                6: SfpBase.SFP_ERROR_DESCRIPTION_HIGH_TEMP,
                7: SfpBase.SFP_ERROR_DESCRIPTION_BAD_CABLE,
                8: cls.SFP_MLNX_ERROR_DESCRIPTION_PMD_TYPE_NOT_ENABLED,
                12: cls.SFP_MLNX_ERROR_DESCRIPTION_PCIE_POWER_SLOT_EXCEEDED,
                15: cls.SFP_MLNX_ERROR_DESCRIPTION_BOOT_ERROR,
                16: cls.SFP_MLNX_ERROR_DESCRIPTION_RECOVERY_ERROR,
                17: cls.SFP_MLNX_ERROR_DESCRIPTION_SUBMODULE_FAILURE,
                19: cls.SFP_MLNX_ERROR_DESCRIPTION_ELS_CRITICAL_INDICATION,
                255: cls.SFP_MLNX_ERROR_DESCRIPTION_RESERVED
        }

    def get_error_description(self):
        """
        Get error description

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
            return SfpBase.SFP_STATUS_INITIALIZING

        oper_status, error_code = self._get_module_info()
        if oper_status == SX_PORT_MODULE_STATUS_INITIALIZING:
            error_description = SfpBase.SFP_STATUS_INITIALIZING
        elif oper_status == SX_PORT_MODULE_STATUS_PLUGGED:
            error_description = SfpBase.SFP_STATUS_OK
        elif oper_status == SX_PORT_MODULE_STATUS_UNPLUGGED:
            error_description = SfpBase.SFP_STATUS_UNPLUGGED
        elif oper_status == SX_PORT_MODULE_STATUS_PLUGGED_DISABLED:
            error_description = SfpBase.SFP_STATUS_DISABLED
        elif oper_status == SX_PORT_MODULE_STATUS_PLUGGED_WITH_ERROR:
            error_description_dict = self._get_error_description_dict()
            if error_code in error_description_dict:
                error_description = error_description_dict[error_code]
            else:
                error_description = "Unknown error ({})".format(error_code)
        else:
            error_description = "Unknown state ({})".format(oper_status)

        return error_description

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
            logger.log_error(f'Failed to get rx los of SFP {self.get_sdk_index()}: {e}')
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
            logger.log_error(f'Failed to get tx fault of SFP {self.get_sdk_index()}: {e}')
        return [False] * api.NUM_CHANNELS if api else None

    def _get_serial(self):
        """
        Get serial number from EEPROM. sfp_base.get_serial() might read from
        memory cache, which is not always up to date. This function is used by reinit_if_sn_changed() to detect if a SFP is replaced.
        """
        api = self.get_xcvr_api()
        if not api:
            return None

        sn = api.xcvr_eeprom.read(consts.VENDOR_SERIAL_NO_FIELD)
        if sn is None:
            return None
        return sn.rstrip()

    def reinit_if_sn_changed(self):
        """Reinitialize the SFP if the module ID has changed
        """
        sn = self._get_serial()
        if sn != self.sn:
            self.reinit()
            self.sn = self._get_serial()
            return True
        return False

    def get_vendor_info(self):
        """Get SFP vendor info (manufacturer and part number).
        Reads fields via xcvr_eeprom to avoid manual offset logic.
        Uses cache to avoid redundant reads.
        Returns:
            tuple: (manufacturer, part_number) or (None, None) if read fails
        """
        try:
            display_idx = self.get_sdk_index() + 1
            if self.manufacturer is not None and self.part_number is not None:
                return self.manufacturer, self.part_number

            api = self.get_xcvr_api()
            if not api or api.xcvr_eeprom is None:
                return None, None

            try:
                manufacturer = api.xcvr_eeprom.read(consts.VENDOR_NAME_FIELD)
                part_number = api.xcvr_eeprom.read(consts.VENDOR_PART_NO_FIELD)
                logger.log_info(f"SFP {display_idx} vendor info read: manufacturer='{manufacturer}', part_number='{part_number}'")
            except Exception as e:
                logger.log_error(f"SFP {display_idx} vendor info read failed: {e}")
                manufacturer = None
                part_number = None

            if manufacturer and part_number:
                self.manufacturer = manufacturer
                self.part_number = part_number
                return manufacturer, part_number

            return None, None
        except Exception:
            return None, None

    def get_temperature_info(self):
        """Get SFP temperature info in a fast way. This function is faster than calling following functions one by one: get_temperature, get_temperature_warning_threshold, get_temperature_critical_threshold.

        Returns:
            tuple: (temperature, warning_threshold, critical_threshold)
        """
        try:
            self.reinit_if_sn_changed()
            if self.retry_read_vendor > 0:
                try:
                    manufacturer, part_number = self.get_vendor_info()
                    if manufacturer and part_number:
                        vendor_info = {'manufacturer': manufacturer, 'part_number': part_number}
                        hw_management_independent_mode_update.vendor_data_set_module(
                            0,  # ASIC index always 0 for now
                            self.get_sdk_index() + 1,
                            vendor_info
                        )
                        logger.log_notice(f'Module {self.get_sdk_index() + 1} vendor info updated - '
                                          f'manufacturer: {manufacturer} part_number: {part_number}')
                        self.retry_read_vendor = 0
                    else:
                        self.retry_read_vendor -= 1
                        if self.retry_read_vendor == 0:
                            logger.log_notice(f"SFP {self.get_sdk_index() + 1}: vendor info unavailable after retries")
                except Exception as e:
                    logger.log_warning(f'Failed to publish vendor info for SFP {self.get_sdk_index() + 1} - {e}')
                    self.retry_read_vendor -= 1
                    if self.retry_read_vendor == 0:
                        logger.log_notice(f"SFP {self.get_sdk_index() + 1}: vendor info unavailable after retries")

            sw_control = self.is_sw_control()
            if not sw_control:
                return sw_control, None, None, None

            self.reinit_if_sn_changed()
            # software control, read from EEPROM
            temperature = self.get_temperature_from_eeprom()
            if temperature is None:
                # Failed to read temperature, no need read threshold
                return sw_control, None, None, None
            elif temperature == 0.0:
                # Temperature is not supported, no need read threshold
                return sw_control, 0.0, 0.0, 0.0
            else:
                self._update_temperature_threshold(sw_control)
                return sw_control, temperature, self.temp_high_threshold, self.temp_critical_threshold
        except Exception:
            # module under initialization, return as temperature not supported
            return False, None, None, None

    def _update_temperature_threshold(self, sw_control):
        """Update temperature threshold

        Args:
            sw_control (bool): True if software control, False if firmware control
        """
        if self.retry_read_threshold <= 0:
            return
        self.temp_high_threshold = None
        self.temp_critical_threshold = None
        if sw_control:
            api = self.get_xcvr_api()
            if api:
                thresh_support = api.get_transceiver_thresholds_support()
                if thresh_support:
                    self.temp_high_threshold = api.xcvr_eeprom.read(consts.TEMP_HIGH_WARNING_FIELD)
                    self.temp_critical_threshold = api.xcvr_eeprom.read(consts.TEMP_HIGH_ALARM_FIELD)
        else:
            self.temp_high_threshold, self.temp_critical_threshold = self.get_temperature_thresholds_from_sysfs()

        if not self.temp_high_threshold or not self.temp_critical_threshold:
            self.retry_read_threshold -= 1
        else:
            self.retry_read_threshold = 0

    def get_temperature_from_eeprom(self):
        """Read the temperature the module reports over its own EEPROM."""
        api = self.get_xcvr_api()
        if api is None:
            return None

        temperature = api.get_module_temperature()
        # The api answers 'N/A' for a module that does not report a temperature.
        return 0.0 if temperature == 'N/A' else temperature

    def get_temperature(self):
        """Get SFP temperature

        A module under firmware control cannot be asked, so the driver's
        reading stands in.

        Returns:
            None if there is an error (sysfs does not exist or sysfs return None or module EEPROM not readable)
            0.0 if module temperature is not supported or module is under initialization
            other float value if module temperature is available
        """
        try:
            if not self.is_sw_control():
                return self.get_temperature_from_sysfs()
        except Exception as e:
            # Per this method's contract every path that reaches here is an error
            # (e.g. is_sw_control() raising 'control sysfs does not exist'), so return
            # None. Returning 0.0 would masquerade a read failure as a genuine 0 degC
            # reading and hide it from thermalctld.
            logger.log_error(f'Failed to get SFP temperature - {e}')
            return None

        self.reinit_if_sn_changed()
        return self.get_temperature_from_eeprom()

    def determine_control_type(self):
        """Determine control type according to module type

        Returns:
            enum: software control or firmware control
        """
        api = self.get_xcvr_api()
        if not api:
            logger.log_error(f'Failed to get api object for SFP {self.get_sdk_index()}, probably module EEPROM is not ready')
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

        Args:
            xcvr_api (object): xcvr api object
        """
        media_interface = self.read_eeprom(CMIS_MEDIA_INTERFACE_TECH_OFFSET, 1)
        return media_interface[0] != CMIS_MEDIA_INTERFACE_TECH_COPPER_LINEAR if media_interface else False

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
        sdk_index = self.get_sdk_index()
        max_power = self.get_module_max_power()
        if max_power < 0:
            return False

        power_limit = self.get_power_limit()
        logger.log_info(f'SFP {sdk_index}: max_power={max_power}, power_limit={power_limit}')
        if max_power <= power_limit:
            return True
        else:
            logger.log_error(f'SFP {sdk_index} exceed power limit: max_power={max_power}, power_limit={power_limit}')
            return False

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
                logger.log_error(f'SFP {self.get_sdk_index()} got invalid value for power class field: {power_class_bit}')
                return POWER_CLASS_INVALID

            return powercap * SFF_POWER_CLASS_TO_QUARTER_WATTS
        else:
            # Should never hit, just in case
            logger.log_error(f'SFP {self.get_sdk_index()} with api type {xcvr_api} does not support getting max power')
            return POWER_CLASS_INVALID

    def update_i2c_frequency(self):
        """Update I2C frequency for the module.
        """
        if self.get_frequency_support():
            sdk_index = self.get_sdk_index()
            api = self.get_xcvr_api()
            if self.is_cmis_api(api):
                # for CMIS modules, read the module maximum supported clock of Management Comm Interface (MCI) from module EEPROM.
                # from byte 2 bits 3-2:
                # 00b means module supports up to 400KHz
                # 01b means module supports up to 1MHz
                logger.log_debug(f"Reading mci max frequency for SFP {sdk_index}")
                read_mci = self.read_eeprom(CMIS_MCI_EEPROM_OFFSET, 1)
                logger.log_debug(f"Read mci max frequency {read_mci[0]} for SFP {sdk_index}")
                frequency = (read_mci[0] & CMIS_MCI_MASK) >> 2
            elif self.is_sff_api(api):
                # for SFF modules, frequency is always 400KHz
                frequency = 0
            else:
                # Should never hit, just in case
                logger.log_error(f'SFP {sdk_index} with api type {api} does not support updating frequency but frequency_support sysfs return 1')
                return

            logger.log_info(f"Read mci max frequency bits {frequency} for SFP {sdk_index}")
            self.set_frequency(frequency)

    def disable_tx_for_sff_optics(self):
        """Disable TX for SFF optics
        """
        api = self.get_xcvr_api()
        if self.is_sff_api(api) and api.get_tx_disable_support():
            logger.log_info(f'Disabling tx for SFP {self.get_sdk_index()}')
            api.tx_disable(True)

    # The aggregate getters xcvrd reads a port through. A pluggable port would
    # inherit these from SfpOptoeBase; they are written out here so the CPO
    # branch answers the same surface without inheriting that class.

    def get_transceiver_info(self):
        api = self.get_xcvr_api()
        return api.get_transceiver_info() if api is not None else None

    def get_transceiver_info_firmware_versions(self):
        api = self.get_xcvr_api()
        return api.get_transceiver_info_firmware_versions() if api is not None else None

    def get_transceiver_status(self):
        api = self.get_xcvr_api()
        return api.get_transceiver_status() if api is not None else None

    def get_transceiver_status_flags(self):
        api = self.get_xcvr_api()
        return api.get_transceiver_status_flags() if api is not None else None

    def get_transceiver_dom_real_value(self):
        api = self.get_xcvr_api()
        return api.get_transceiver_dom_real_value() if api is not None else None

    def get_transceiver_dom_flags(self):
        api = self.get_xcvr_api()
        return api.get_transceiver_dom_flags() if api is not None else None

    def get_transceiver_threshold_info(self):
        api = self.get_xcvr_api()
        return api.get_transceiver_threshold_info() if api is not None else None

    def get_transceiver_vdm_real_value_basic(self):
        api = self.get_xcvr_api()
        return api.get_transceiver_vdm_real_value_basic() if api is not None else None

    def get_transceiver_vdm_real_value_statistic(self):
        api = self.get_xcvr_api()
        return api.get_transceiver_vdm_real_value_statistic() if api is not None else None

    def get_transceiver_vdm_flags(self):
        api = self.get_xcvr_api()
        return api.get_transceiver_vdm_flags() if api is not None else None

    def get_transceiver_vdm_thresholds(self):
        api = self.get_xcvr_api()
        return api.get_transceiver_vdm_thresholds() if api is not None else None

    def get_transceiver_pm(self):
        api = self.get_xcvr_api()
        return api.get_transceiver_pm() if api is not None else None

    def is_transceiver_vdm_supported(self):
        api = self.get_xcvr_api()
        return api.is_transceiver_vdm_supported() if api is not None else None

    def is_vdm_statistic_supported(self):
        api = self.get_xcvr_api()
        return api.is_vdm_statistic_supported() if api is not None else None

    def get_vdm_freeze_status(self):
        """Whether the module has finished freezing its VDM statistics.

        An optic that cannot freeze is reported as not frozen rather than as an
        error, because the caller polls this to decide when a sample is ready.
        """
        api = self.get_xcvr_api()
        try:
            return api.get_vdm_freeze_status() if api is not None else False
        except (NotImplementedError, AttributeError):
            return False

    def get_vdm_unfreeze_status(self):
        """Whether the module has finished unfreezing its VDM statistics."""
        api = self.get_xcvr_api()
        try:
            return api.get_vdm_unfreeze_status() if api is not None else False
        except (NotImplementedError, AttributeError):
            return False

    # sfpshow and sfputil ask a port for the extra fields it wants printed. They
    # are answered here rather than inherited because a CPO port is a DeviceBase,
    # not an SfpBase, so it has no upstream default to fall back on. This platform
    # adds no fields of its own.

    def get_platform_specific_transceiver_info_format_map(self):
        return {}

    def get_platform_specific_transceiver_status_format_map(self):
        return {}

    def get_platform_specific_dom_format_map(self):
        return {}, {}

    # The rest of the api-forwarding surface. Nothing calls these on a CPO port
    # today, but a port that answers only today's call sites fails in pmon the
    # first time upstream adds one, so the set is kept complete.

    def get_tx_power(self):
        api = self.get_xcvr_api()
        return api.get_tx_power() if api is not None else None

    def get_tx_disable(self):
        api = self.get_xcvr_api()
        return api.get_tx_disable() if api is not None else None

    def get_tx_disable_channel(self):
        api = self.get_xcvr_api()
        return api.get_tx_disable_channel() if api is not None else None

    def tx_disable(self, tx_disable):
        api = self.get_xcvr_api()
        return api.tx_disable(tx_disable) if api is not None else None

    def tx_disable_channel(self, channel, disable):
        api = self.get_xcvr_api()
        return api.tx_disable_channel(channel, disable) if api is not None else None

    def get_rx_disable(self):
        api = self.get_xcvr_api()
        return api.get_rx_disable() if api is not None else None

    def get_rx_disable_channel(self):
        api = self.get_xcvr_api()
        return api.get_rx_disable_channel() if api is not None else None

    def rx_disable(self, rx_disable):
        api = self.get_xcvr_api()
        return api.rx_disable(rx_disable) if api is not None else None

    def rx_disable_channel(self, channel, disable):
        api = self.get_xcvr_api()
        return api.rx_disable_channel(channel, disable) if api is not None else None

    def get_power_override(self):
        api = self.get_xcvr_api()
        return api.get_power_override() if api is not None else None

    def set_power_override(self, power_override, power_set):
        api = self.get_xcvr_api()
        return api.set_power_override(power_override, power_set) if api is not None else None

    def get_transceiver_loopback(self):
        api = self.get_xcvr_api()
        return api.get_transceiver_loopback() if api is not None else None

    def is_coherent_module(self):
        api = self.get_xcvr_api()
        return api.is_coherent_module() if api is not None else None

    # What no NVIDIA port can answer. sfputil guards some of these with `except
    # NotImplementedError` alone, so refusing is safer than being absent.

    def get_lpmode_via_pin(self):
        """Low power mode is driven over the EEPROM, not a hardware pin."""
        raise NotImplementedError

    def set_lpmode_via_pin(self, lpmode):
        """Low power mode is driven over the EEPROM, not a hardware pin."""
        raise NotImplementedError

    def get_eeprom_path(self):
        """There is no optoe flat file; the EEPROM is reached through sx_core."""
        raise NotImplementedError

    def set_optoe_write_max(self, write_max):
        """Follows from `get_eeprom_path`: no optoe driver, no write_max knob."""
        raise NotImplementedError

    def get_reset_status(self):
        """The reset line can be driven but its state cannot be read back."""
        raise NotImplementedError

    # The database-backed members. They answer the same questions as the ones
    # above without waking the module, keyed by the port's logical name.

    @classmethod
    def get_port_config_done(cls, namespace):
        app_db = get_db_table_helper().get_appl_db(namespace)
        return app_db.exists(PORT_CONFIG_DONE)

    @classmethod
    def build_port_mapping(cls, namespace):
        from natsort import natsorted
        db = ConfigDBConnector(use_unix_socket_path=True, namespace=namespace)
        db.db_connect(db.CONFIG_DB)
        port_table = db.get_table(CFG_PORT_TABLE)
        for logical_port, value in natsorted(port_table.items()):
            index = int(value.get('index'))
            if index not in cls.port_mapping:
                cls.port_mapping[index] = logical_port

    def get_logical_port(self):
        logical_port = self.port_mapping.get(self.index)
        if not logical_port:
            port_config_done = self.get_port_config_done(self.namespace)
            if not port_config_done:
                return None
            self.build_port_mapping(self.namespace)
            logical_port = self.port_mapping.get(self.index)
        return logical_port

    def get_temperature_from_db(self):
        """Get temperature from DB

        Returns:
            float: return 0 if module does not support temperature or not present, return -1 if read failed,
                   return other float value if module supports temperature
        """
        present, value = self._get_data_from_db(get_db_table_helper().get_module_temperature_table,
                                                'temperature')
        if not present:
            return 0

        if value == 'None':
            return -1

        return float(value)

    def get_warning_threshold_from_db(self):
        present, value = self._get_data_from_db(get_db_table_helper().get_module_threshold_table,
                                                'temphighwarning')
        # xcvrd returns N/A if threshold is not supported
        # xcvrd cannot tell read failure or not supported,
        # so we return 0 in both cases
        if not present or value == 'N/A':
            return 0

        return float(value)

    def get_critical_threshold_from_db(self):
        present, value = self._get_data_from_db(get_db_table_helper().get_module_threshold_table,
                                                'temphighalarm')
        # xcvrd returns N/A if threshold is not supported
        # xcvrd cannot tell read failure or not supported,
        # so we return 0 in both cases
        if not present or value == 'N/A':
            return 0

        return float(value)

    def get_vendor_name_from_db(self):
        present, value = self._get_data_from_db(get_db_table_helper().get_module_info_table,
                                                'manufacturer')
        if not present:
            return ''
        return value.strip()

    def get_part_number_from_db(self):
        present, value = self._get_data_from_db(get_db_table_helper().get_module_info_table,
                                                'model')
        if not present:
            return ''
        return value.strip()

    def _get_data_from_db(self, table_cb, key):
        logical_port = self.get_logical_port()
        if not logical_port:
            return False, None
        return table_cb().hget(logical_port, key)
