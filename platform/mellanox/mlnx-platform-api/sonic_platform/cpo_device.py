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

from sonic_platform_base.sonic_xcvr.cpo.cpo_base import CpoDeviceBase, CpoHardwareInfo
from sonic_platform_base.sonic_xcvr.cpo.elsfp import ElsfpBase
from sonic_platform_base.sonic_xcvr.cpo.oe import OeBase

from .nvidia_sysfs_eeprom import NvidiaSysfsEepromBanked


class _NvidiaSysfsCpoDevice(CpoDeviceBase, NvidiaSysfsEepromBanked):
    """Shared sx_core plumbing for banked CPO devices.

    Adds the module index and the ASIC it belongs to, to CpoDeviceBase, which
    only carries the bank. All three are held here and nowhere else: a CPO port
    reaches them through `module_access`, which points at its OE, so the port and
    the device cannot disagree about which module they address. Only the module
    index and the bank locate the sysfs tree; the ASIC index is identity that
    hw-management's per-ASIC tree needs.
    """

    def __init__(self, hardware_id: CpoHardwareInfo, sdk_index: int, bank: int = 0, asic_index: int = 0):
        self.sdk_index = sdk_index
        self.asic_index = asic_index
        super().__init__(hardware_id, bank=bank)

    def get_asic_index(self):
        return self.asic_index

    def get_sdk_index(self):
        return self.sdk_index


class NvidiaSysfsOe(OeBase, _NvidiaSysfsCpoDevice):
    pass


class NvidiaSysfsElsfp(ElsfpBase, _NvidiaSysfsCpoDevice):
    pass
