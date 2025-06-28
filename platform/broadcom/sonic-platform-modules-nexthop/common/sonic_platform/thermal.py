#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import time

try:
    from sonic_platform_pddf_base.pddf_thermal import PddfThermal
    from sonic_platform_base.thermal_base import ThermalBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Thermal(PddfThermal):
    """PDDF Platform-Specific Thermal class"""

    def __init__(
        self,
        index,
        pddf_data=None,
        pddf_plugin_data=None,
        is_psu_thermal=False,
        psu_index=0,
    ):
        PddfThermal.__init__(
            self, index, pddf_data, pddf_plugin_data, is_psu_thermal, psu_index
        )
        self._min_temperature = None
        self._max_temperature = None

    def get_model(self):
        return "N/A"

    def get_serial(self):
        return "N/A"

    def get_status(self):
        return self.get_presence()

    def get_temperature(self):
        temp = PddfThermal.get_temperature(self)
        if not self._min_temperature or temp < self._min_temperature:
            self._min_temperature = temp
        if not self._max_temperature or temp > self._max_temperature:
            self._max_temperature = temp
        return temp

    def get_minimum_recorded(self):
        self.get_temperature()
        return self._min_temperature

    def get_maximum_recorded(self):
        self.get_temperature()
        return self._max_temperature

class SfpThermal(ThermalBase):
    """SFP thermal interface class"""
    THRESHOLDS_CACHE_INTERVAL_SEC = 5

    def __init__(self, sfp):
        ThermalBase.__init__(self)
        self._sfp = sfp
        self._min_temperature = None
        self._max_temperature = None
        self._threshold_info = {}
        self._threshold_info_time = 0

    def get_name(self):
        return f"Transceiver {self._sfp.get_name().capitalize()}"

    def get_presence(self):
        presence = self._sfp.get_presence()
        if not presence:
            self._threshold_info = {}
        return presence

    def get_model(self):
        return "N/A"

    def get_serial(self):
        return "N/A"

    def get_revision(self):
        return "N/A"

    def get_status(self):
        return self.get_presence()

    def get_position_in_parent(self):
        return self._sfp.get_position_in_parent()

    def is_replaceable(self):
        return True

    def get_temperature(self):
        temp = self._sfp.get_temperature()
        if self._min_temperature is None or temp < self._min_temperature:
            self._min_temperature = temp
        if self._max_temperature is None or temp > self._max_temperature:
            self._max_temperature = temp
        return temp

    def maybe_update_threshold_info(self):
        time_elapsed = time.monotonic() - self._threshold_info_time
        if not self._threshold_info or time_elapsed > self.THRESHOLDS_CACHE_INTERVAL_SEC:
            self._threshold_info = self._sfp.get_transceiver_threshold_info()
            self._threshold_info_time = time.monotonic()

    def get_high_threshold(self):
        self.maybe_update_threshold_info()
        return self._threshold_info.get("temphighwarning")

    def get_low_threshold(self):
        self.maybe_update_threshold_info()
        return self._threshold_info.get("templowwarning")

    def get_high_critical_threshold(self):
        self.maybe_update_threshold_info()
        return self._threshold_info.get("temphighalarm")

    def get_low_critical_threshold(self):
        self.maybe_update_threshold_info()
        return self._threshold_info.get("templowalarm")

    def set_high_threshold(self, temperature):
        return False

    def set_low_threshold(self, temperature):
        return False

    def set_high_critical_threshold(self, temperature):
        return False

    def set_low_critical_threshold(self, temperature):
        return False

    def get_minimum_recorded(self):
        self.get_temperature()
        return self._min_temperature

    def get_maximum_recorded(self):
        self.get_temperature()
        return self._max_temperature
