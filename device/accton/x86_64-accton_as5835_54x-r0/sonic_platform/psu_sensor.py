"""Expose PSU output measurements through the chassis sensor API.

The native AS5835 driver already exports mV/mA. Read its sysfs attributes
directly because the legacy PSU API returns zero for unreadable files. PSU
presence and power-good checks still use the existing PSU API.
"""

import math

from sonic_platform_base.sensor_base import CurrentSensorBase, VoltageSensorBase


class _PsuSensor:
    def __init__(self, psu, position):
        self._psu = psu
        self._position = position
        self._minimum = None
        self._maximum = None

    def get_name(self):
        return "{}_Output_{}".format(self._psu.get_name(), self._quantity)

    def get_position_in_parent(self):
        return self._position

    def is_replaceable(self):
        return True

    def get_presence(self):
        return self._psu.get_presence()

    def get_status(self):
        return self._psu.get_status()

    def _read(self, attribute):
        try:
            if not self.get_presence() or not self.get_status():
                return None
            if attribute is None:
                return None
            value = self._psu._api_helper.read_txt_file(
                self._psu.hwmon_path + attribute)
            if value is None or isinstance(value, bool):
                return None
            value = float(value)
            if not math.isfinite(value) or value < 0:
                return None
            return value
        except (AttributeError, NotImplementedError, OSError, ValueError, TypeError):
            return None

    def get_value(self):
        value = self._read(self._value_attribute)
        if value is None:
            # Do not retain extrema across PSU absence or unreadable samples.
            self._minimum = self._maximum = None
        else:
            self._minimum = value if self._minimum is None else min(value, self._minimum)
            self._maximum = value if self._maximum is None else max(value, self._maximum)
        return value

    def get_minimum_recorded(self):
        self.get_value()
        return self._minimum

    def get_maximum_recorded(self):
        self.get_value()
        return self._maximum

    def get_low_threshold(self):
        value = self._read(self._low_attribute)
        return value if value is not None and value > 0 else None

    def get_high_threshold(self):
        value = self._read(self._high_attribute)
        return value if value is not None and value > 0 else None

    def get_low_critical_threshold(self):
        return None

    def get_high_critical_threshold(self):
        return None


class PsuVoltageSensor(_PsuSensor, VoltageSensorBase):
    """PSU output voltage and manufacturer voltage limits, in millivolts."""

    _quantity = "Voltage"
    _value_attribute = "psu_v_out"
    _low_attribute = "psu_mfr_vout_min"
    _high_attribute = "psu_mfr_vout_max"


class PsuCurrentSensor(_PsuSensor, CurrentSensorBase):
    """PSU output current in milliamps; unsupported limits stay unavailable."""

    _quantity = "Current"
    _value_attribute = "psu_i_out"
    _low_attribute = None
    _high_attribute = None
