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
"""sx_core flavours of the community CPO device abstractions.

The community `OeBase` / `ElsfpBase` carry the API factory and the bank but
leave the EEPROM transport to the platform. These subclasses supply it, so a
CPO port is composed of two devices that read their own EEPROM through the
same per-page sysfs tree the pluggable modules use.
"""

from sonic_py_common.logger import Logger
from sonic_platform_base.sonic_xcvr.api.nvidia.cpo_cdb import get_module_cdb_lock
from sonic_platform_base.sonic_xcvr.cpo.cpo_base import CpoDeviceBase, CpoHardwareInfo
from sonic_platform_base.sonic_xcvr.cpo.elsfp import ElsfpBase
from sonic_platform_base.sonic_xcvr.cpo.oe import OeBase

from . import utils
from .device_data import DeviceDataManager
from .module_sysfs import (
    POWER_MODE_LOW,
    POWER_MODE_POLICY_HIGH,
    POWER_MODE_POLICY_LOW,
    SFP_SYSFS_CONTROL,
    SFP_SYSFS_HWRESET,
    SFP_SYSFS_POWER_MODE,
    SFP_SYSFS_POWER_MODE_POLICY,
    SFP_SYSFS_RESET,
)
from .nvidia_sysfs_eeprom import NvidiaSysfsEepromBanked

logger = Logger()


class _NvidiaSysfsCpoDevice(CpoDeviceBase, NvidiaSysfsEepromBanked):
    """Shared sx_core plumbing for banked CPO devices.

    Adds the module index and the ASIC it belongs to, to CpoDeviceBase, which
    only carries the bank. All three are held here and nowhere else: a CPO port
    reaches them through `module_access`, which points at its OE, so the port and
    the device cannot disagree about which module they address. Only the module
    index and the bank locate the sysfs tree; the ASIC index is identity that
    hw-management's per-ASIC tree needs.
    """

    # The directory of the CDB lock of each module (see get_module_cdb_lock),
    # which pmon also mounts.
    CDB_LOCK_DIR = '/var/lock/nvidia-cpo'
    # Seconds that a firmware operation waits for the CDB of its module: long
    # enough for a command of another user, such as a DOM read, but not for
    # another firmware operation.
    FIRMWARE_CDB_LOCK_TIMEOUT = 2

    def __init__(self, hardware_id: CpoHardwareInfo, sdk_index: int, bank: int = 0, asic_index: int = 0):
        self.sdk_index = sdk_index
        self.asic_index = asic_index
        super().__init__(hardware_id, bank=bank)

    def _firmware_lock(self):
        """
        Hold the CDB lock of the module for a firmware operation.

        The OE and the ELS of a module share one CDB, so this one lock runs
        one firmware operation per module, and keeps the other users of the
        CDB (xcvrd, cpoutil show) out of it: a download fails if other CDB
        commands reach the module between its blocks.

        Raises:
            RuntimeError: Another firmware operation on the module is in
                progress (the lock stayed busy FIRMWARE_CDB_LOCK_TIMEOUT s)
        """
        return get_module_cdb_lock(self).hold(self.FIRMWARE_CDB_LOCK_TIMEOUT)

    def download_firmware(self, file_path):
        with self._firmware_lock():
            yield from super().download_firmware(file_path)

    def run_firmware(self, *args, **kwargs):
        with self._firmware_lock():
            return super().run_firmware(*args, **kwargs)

    def commit_firmware(self):
        with self._firmware_lock():
            return super().commit_firmware()

    def upgrade_firmware(self, file_path):
        with self._firmware_lock():
            yield from super().upgrade_firmware(file_path)

    def get_firmware_info(self):
        with self._firmware_lock():
            return super().get_firmware_info()

    def get_asic_index(self):
        return self.asic_index

    def get_sdk_index(self):
        return self.sdk_index


class NvidiaSysfsOe(OeBase, _NvidiaSysfsCpoDevice):
    """OE whose low-power and reset controls are the shared module's.

    The banks of an OE share one sx_core module. These hooks read and write
    that module's power and reset attributes.
    """

    def _is_sw_control(self):
        if not DeviceDataManager.is_module_host_management_mode():
            return False
        try:
            return utils.read_int_from_file(
                self.module_attr_path(SFP_SYSFS_CONTROL),
                raise_exception=True, log_func=None) == 1
        except Exception:
            raise Exception(
                'control sysfs for OE {} does not exist'.format(self.get_sdk_index()))

    def get_lpmode(self) -> bool:
        """Return whether the OE module is in low power."""
        try:
            if self._is_sw_control():
                api = self.get_api()
                return bool(api.get_lpmode()) if api else False
        except Exception as exc:
            logger.log_error(
                'Failed to get lpmode of OE {}: {}'.format(self.get_sdk_index(), exc))
            return False

        power_mode = utils.read_int_from_file(self.module_attr_path(SFP_SYSFS_POWER_MODE))
        return power_mode == POWER_MODE_LOW

    def set_lpmode(self, lpmode: bool) -> bool:
        """Set the OE module low-power mode. True asserts low power."""
        try:
            if self._is_sw_control():
                api = self.get_api()
                if not api:
                    return False
                if api.get_lpmode() == lpmode:
                    return True
                api.set_lpmode(lpmode)
                return utils.wait_until(
                    lambda api, lpmode: api.get_lpmode() == lpmode,
                    2, 1, api=api, lpmode=lpmode)
        except Exception as exc:
            logger.log_error(
                'Failed to set lpmode of OE {}: {}'.format(self.get_sdk_index(), exc))
            return False

        path = self.module_attr_path(SFP_SYSFS_POWER_MODE_POLICY)
        target = POWER_MODE_POLICY_LOW if lpmode else POWER_MODE_POLICY_HIGH
        if utils.read_int_from_file(path) == target:
            return True
        return utils.write_file(path, str(target))

    def reset(self) -> bool:
        """Pulse the OE module reset. True if the sysfs write succeeded."""
        try:
            if not self._is_sw_control():
                return utils.write_file(self.module_attr_path(SFP_SYSFS_RESET), '1')
            path = self.module_attr_path(SFP_SYSFS_HWRESET)
            return utils.write_file(path, '0') and utils.write_file(path, '1')
        except Exception as exc:
            logger.log_error(
                'Failed to reset OE {}: {}'.format(self.get_sdk_index(), exc))
            return False


class NvidiaSysfsElsfp(ElsfpBase, _NvidiaSysfsCpoDevice):
    pass
