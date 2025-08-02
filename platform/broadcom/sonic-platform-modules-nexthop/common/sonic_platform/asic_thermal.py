#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
from sonic_platform_pddf_base.pddf_asic_thermal import PddfAsicThermal


class AsicThermal(PddfAsicThermal):
    """PDDF Platform-Specific ASIC Thermal class"""

    def __init__(
        self,
        index,
        pddf_data=None,
    ):
        super().__init__(
            index, pddf_data
        )
        self._min_temperature = None
        self._max_temperature = None

    def get_temperature(self):
        temp = super().get_temperature()
        if self._min_temperature is None or temp < self._min_temperature:
            self._min_temperature = temp
        if self._max_temperature is None or temp > self._max_temperature:
            self._max_temperature = temp
        return temp

    def get_minimum_recorded(self):
        self.get_temperature()
        return self._min_temperature

    def get_maximum_recorded(self):
        self.get_temperature()
        return self._max_temperature
