"""Expose PSU electrical measurements through the chassis sensor API.

Opt-in adapters reuse the existing PSU API; psud remains responsible for PSU
health. SensorBase uses mV/mA, whereas the PSU API returns V/A.
"""

import math

from sonic_platform_base.sensor_base import CurrentSensorBase, VoltageSensorBase


class _PsuSensor:
    _direction = "Output"

    def __init__(self, psu, position):
        self._psu = psu
        self._position = position
        self._minimum = None
        self._maximum = None

    def get_name(self):
        return "{}_{}_{}".format(self._psu.get_name(), self._direction, self._quantity)

    def get_position_in_parent(self):
        return self._position

    def is_replaceable(self):
        return True

    def get_presence(self):
        return self._psu.get_presence()

    def get_status(self):
        return self._psu.get_status()

    def _read(self, method):
        try:
            if not self.get_presence() or not self.get_status():
                return None
            if method is None:
                return None
            value = getattr(self._psu, method)()
            if value is None or isinstance(value, bool):
                return None
            value = float(value)
            if not math.isfinite(value) or value < 0:
                return None
            return value * 1000
        except (AttributeError, NotImplementedError, OSError, ValueError, TypeError):
            return None

    def get_value(self):
        value = self._read(self._value_method)
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
        value = self._read(self._low_method)
        return value if value is not None and value > 0 else None

    def get_high_threshold(self):
        value = self._read(self._high_method)
        return value if value is not None and value > 0 else None

    def get_low_critical_threshold(self):
        return None

    def get_high_critical_threshold(self):
        return None


class PsuVoltageSensor(_PsuSensor, VoltageSensorBase):
    """PSU output voltage and manufacturer voltage limits, in millivolts."""

    _quantity = "Voltage"
    _value_method = "get_voltage"
    _low_method = "get_voltage_low_threshold"
    _high_method = "get_voltage_high_threshold"


class PsuCurrentSensor(_PsuSensor, CurrentSensorBase):
    """PSU output current in milliamps; unsupported limits stay unavailable."""

    _quantity = "Current"
    _value_method = "get_current"
    _low_method = "get_current_low_threshold"
    _high_method = "get_current_high_threshold"


class PsuInputTelemetry:
    """Opt-in input readings; missing data must not become false zero samples.

    Existing PDDF input getters default to zero for missing attributes. These
    platforms expose validated input telemetry only when the PSU is power-good.
    """

    def _get_input_reading(self, attribute, divisor):
        try:
            if not self.get_presence() or not self.get_status():
                return None
            output = self.pddf_obj.get_attr_name_output(
                "PSU{}".format(self.psu_index), attribute)
            value = output.get("status") if output else None
            if value is None or isinstance(value, bool):
                return None
            value = float(value)
            if not math.isfinite(value) or value < 0:
                return None
            return value / divisor
        except (AttributeError, NotImplementedError, OSError, ValueError, TypeError):
            return None

    def get_input_voltage(self):
        """Return input voltage in volts, or None when unavailable."""
        return self._get_input_reading("psu_v_in", 1000)

    def get_input_current(self):
        """Return input current in amperes, or None when unavailable."""
        return self._get_input_reading("psu_i_in", 1000)

    def get_input_power(self):
        """Return input power in watts, or None when unavailable."""
        return self._get_input_reading("psu_p_in", 1000000)


class PsuInputVoltageSensor(_PsuSensor, VoltageSensorBase):
    """PSU input voltage in millivolts; no inferred alarm thresholds."""

    _direction = "Input"
    _quantity = "Voltage"
    _value_method = "get_input_voltage"
    _low_method = None
    _high_method = None


class PsuInputCurrentSensor(_PsuSensor, CurrentSensorBase):
    """PSU input current in milliamps; no inferred alarm thresholds."""

    _direction = "Input"
    _quantity = "Current"
    _value_method = "get_input_current"
    _low_method = None
    _high_method = None
