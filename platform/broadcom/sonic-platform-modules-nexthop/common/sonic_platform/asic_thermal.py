#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
from sonic_platform_pddf_base.pddf_asic_thermal import PddfAsicThermal
from .thermal import PidThermalMixin, MinMaxTempMixin


class AsicThermal(PddfAsicThermal, MinMaxTempMixin, PidThermalMixin):
    """PDDF Platform-Specific ASIC Thermal class"""

    def __init__(
        self,
        index,
        position_offset,
        pddf_data=None,
    ):
        super().__init__(index, position_offset, pddf_data)
        MinMaxTempMixin.__init__(self)

        # Get PID configuration from PDDF data
        thermal_obj_name = self.get_thermal_obj_name()
        thermal_obj = pddf_data.data[thermal_obj_name]
        dev_info = thermal_obj['dev_info']

        PidThermalMixin.__init__(self, dev_info)

    def get_temperature(self):
        temp = super().get_temperature()
        self._update_min_max_temp(temp)
        return temp

    def get_minimum_recorded(self):
        # Make sure temp is recorded at least once.
        self.get_temperature()
        return MinMaxTempMixin.get_minimum_recorded(self)

    def get_maximum_recorded(self):
        # Make sure temp is recorded at least once.
        self.get_temperature()
        return MinMaxTempMixin.get_maximum_recorded(self)
