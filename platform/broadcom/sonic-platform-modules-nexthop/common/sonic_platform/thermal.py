#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import time

from nexthop import fpga_lib
try:
    from sonic_platform_base.thermal_base import ThermalBase
    from sonic_platform_pddf_base.pddf_thermal import PddfThermal
    from sonic_platform_base.thermal_base import ThermalBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


# Nexthop FPGA thermal sensor value to Celsius conversion
TYPE_TO_CELSIUS_LAMBDA_DICT = {
  'nh1': lambda field_val:  0.158851 * (3000 - field_val),
}


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
        return self._min_temperature

    def get_maximum_recorded(self):
        return self._max_temperature


class NexthopFpgaAsicThermal(ThermalBase):
    """ASIC temperature sensor read from the FPGA register"""

    def __init__(
        self,
        index,
        pddf_data
    ):
        super().__init__()

        pddf_obj_name = f'NEXTHOP-FPGA-ASIC-TEMP-SENSOR{index}'
        pddf_obj_data = pddf_data.data[pddf_obj_name]

        dev_info = pddf_obj_data['dev_info']
        device_parent_name = dev_info['device_parent']
        self._fpga_pci_addr = pddf_data.data[device_parent_name]['dev_info']['device_bdf']

        dev_attr = pddf_obj_data['dev_attr']
        self._display_name = dev_attr['display_name']
        self._fpga_min_rev = int(dev_attr.get('fpga_min_rev', 0), 16)
        self._value_type = dev_attr['value_type']
        self._reg_addr = int(dev_attr['reg_addr'], 16)
        self._field_range_end, self._field_range_start = map(int, dev_attr['field_range'].split(':'))
        self._temp1_high_threshold = dev_attr['temp1_high_threshold']
        self._temp1_high_crit_threshold = dev_attr['temp1_high_crit_threshold']

        self._supported = self._check_sensor_supported()
        self._min_temperature = None
        self._max_temperature = None

    def _check_sensor_supported(self):
        try:
            fpga_ver = fpga_lib.read_32(self._fpga_pci_addr, 0x0)
        except PermissionError:
            # PermissionError is expected if running as non-root
            return False
        return fpga_ver >= self._fpga_min_rev

    def get_num_thermals(self):
        return 1

    def get_name(self):
        return self._display_name

    def get_temp_label(self):
        return None

    def get_temperature(self):
        if not self._supported:
            return None
        reg_val = fpga_lib.read_32(self._fpga_pci_addr, self._reg_addr)
        field_val = reg_val >> self._field_range_start
        field_val = field_val & ((1 << (self._field_range_end - self._field_range_start + 1)) - 1)
        get_celsius_fn = TYPE_TO_CELSIUS_LAMBDA_DICT.get(self._value_type)
        if get_celsius_fn:
            val = get_celsius_fn(field_val)
        else:
            raise NotImplementedError(f'Unsupported NexthopFpgaAsicThermal value type: {self._value_type}')
        val = round(val, 3)
        if self._min_temperature is None or val < self._min_temperature:
            self._min_temperature = val
        if self._max_temperature is None or val > self._max_temperature:
            self._max_temperature = val
        return val

    def get_low_threshold(self):
        return None

    def get_low_critical_threshold(self):
        return None

    def get_high_threshold(self):
        return self._temp1_high_threshold

    def get_high_critical_threshold(self):
        return self._temp1_high_crit_threshold

    def get_minimum_recorded(self):
        return self._min_temperature

    def get_maximum_recorded(self):
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
        if not self.get_presence():
            return None
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
        return self._min_temperature

    def get_maximum_recorded(self):
        return self._max_temperature
