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
"""The port class for co-packaged optics.

A CPO port is not a pluggable one, and this module is a sibling of `sfp.py`
rather than a part of it: what the two technologies share is stated as the
mixins both import, so neither has to know the other exists.
"""

from sonic_py_common import multi_asic

from sonic_platform_base.sfp_base import SfpBase
from sonic_platform_base.sonic_xcvr.cpo.cpo_base import CpoBase, CpoHardwareInfo, OeId

from .cpo_device import NvidiaSysfsElsfp, NvidiaSysfsOe
from .device_data import DeviceDataManager
from .module_detection_flow import (
    ModuleDetectionFlow,
    STATE_FW_CONTROL,
    STATE_NOT_PRESENT,
    STATE_POWER_BAD,
    STATE_POWER_LIMIT_ERROR,
    STATE_SW_CONTROL,
)
from .module_sysfs import ModuleSysfsMixin, SFP_STATUS_INSERTED, SFP_STATUS_REMOVED
from .module_xcvr import ModuleXcvrMixin

CPO_TYPE = "CPO"


class CpoPort(ModuleXcvrMixin, ModuleSysfsMixin, CpoBase):
    """One logical port of a co-packaged optics module.

    A CPO port is not a pluggable one: there is no cage and nothing to insert,
    and its media lives in two devices - the optical engine and the external
    laser source - rather than in one module. So it takes the shared behaviour
    as mixins and its media from `CpoBase`, instead of inheriting `SFP`.

    Four logical ports share one OE, one per bank. Identity therefore lives on
    the OE and not on the port: `module_access` points there, and the three
    accessors below are views onto it rather than copies.
    """

    NUMBER_OF_BANKS = 4

    def __init__(self, sfp_index, bank_id, oe_id, els_id, asic_id='asic0'):
        hardware_id = CpoHardwareInfo(oe_id=OeId.NVIDIA_SPC6_CPO, elsfp_id=None)
        asic_index = multi_asic.get_asic_index_from_namespace(asic_id)
        # Instantiating the devices is what wires their api factories, so
        # there is no api code here: get_xcvr_api() builds lazily off these.
        oe = NvidiaSysfsOe(hardware_id, bank=bank_id, sdk_index=oe_id, asic_index=asic_index)
        elsfp = NvidiaSysfsElsfp(hardware_id, bank=bank_id, sdk_index=oe_id, asic_index=asic_index)
        CpoBase.__init__(self, hardware_id, oe, elsfp)

        self.index = sfp_index + 1
        self.asic_id = asic_id
        self.els_id = els_id
        self.sfp_type = CPO_TYPE
        self._sfp_type_str = None
        self.sn = None
        from .thermal import initialize_sfp_thermal
        self._thermal_list = initialize_sfp_thermal(self)
        if DeviceDataManager.is_multi_asic_platform():
            self.namespace = asic_id
        else:
            self.namespace = multi_asic.DEFAULT_NAMESPACE
        ModuleDetectionFlow.init_detection_flow(self, oe_id)

    @property
    def module_access(self):
        """The OE owns this port's sysfs coordinates; all four banks share it."""
        return self.oe

    @property
    def sdk_index(self):
        return self.oe.sdk_index

    @property
    def oe_id(self):
        return self.oe.sdk_index

    @property
    def bank_id(self):
        return self.oe.bank

    def read_eeprom(self, offset, num_bytes):
        return self.oe.read_eeprom(offset, num_bytes)

    def _read_eeprom(self, offset, num_bytes, log_on_error=True):
        return self.oe._read_eeprom(offset, num_bytes, log_on_error)

    def write_eeprom(self, offset, num_bytes, write_buffer):
        return self.oe.write_eeprom(offset, num_bytes, write_buffer)

    def get_asic_id(self):
        return self.asic_id

    def __str__(self):
        return f'CPO port {self.index} (OE {self.oe.sdk_index} bank {self.oe.bank})'

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
        """One physical OE fans out to NUMBER_OF_BANKS contiguous logical
        ports starting at self.index (= bank 0 of this vModule).
        """
        if self.state == STATE_NOT_PRESENT:
            value = SFP_STATUS_REMOVED
        elif self.state == STATE_SW_CONTROL or self.state == STATE_FW_CONTROL:
            value = SFP_STATUS_INSERTED
        elif self.state == STATE_POWER_BAD or self.state == STATE_POWER_LIMIT_ERROR:
            value = str(SfpBase.SFP_ERROR_BIT_POWER_BUDGET_EXCEEDED
                        | SfpBase.SFP_STATUS_BIT_INSERTED)
        else:
            return
        for i in range(self.NUMBER_OF_BANKS):
            port_dict[self.index + i] = value
