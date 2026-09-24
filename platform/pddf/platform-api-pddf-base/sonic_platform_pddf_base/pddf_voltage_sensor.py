
try:
    from sonic_platform_base.sensor_base import VoltageSensorBase, SensorBase
    from subprocess import getstatusoutput
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class PddfVoltageSensor(VoltageSensorBase):
    """PDDF generic Voltage Sensor class.

    Supports sysfs-backed sensor read methods based on sensor_type in dev_attr:
      - hwmon (default): reads hwmon sysfs via standard PDDF path
      - iio: reads IIO sysfs (iio:deviceX/in_voltageN_raw)

    Scaling (multi_factor / decimate_factor) is applied to all reads.
    Thresholds are read from JSON dev_attr if present, falling back to sysfs.
    """
    pddf_obj = {}
    plugin_data = {}

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        if not pddf_data or not pddf_plugin_data:
            raise ValueError('PDDF JSON data error')

        self.pddf_obj = pddf_data
        self.plugin_data = pddf_plugin_data
        self.sensor_index = index + 1
        
        self.sensor_obj_name = f"VOLTAGE{self.sensor_index}"
        if self.sensor_obj_name not in self.pddf_obj.data:
            raise ValueError(f"Voltage sensor {self.sensor_obj_name} not found in PDDF data")

        self.sensor_obj = self.pddf_obj.data[self.sensor_obj_name]

        # dev_attr may be at top level or under i2c
        self._top_dev_attr = self.sensor_obj.get('dev_attr', {})
        self._i2c_dev_attr = self.pddf_obj.get_sensor_i2c_dev_attr(self.sensor_obj)

        self.display_name = (self._top_dev_attr.get('display_name') or
                             self._i2c_dev_attr.get('display_name', self.sensor_obj_name))

        # Sensor type: 'iio' or '' (hwmon default)
        self._sensor_type = self._i2c_dev_attr.get('sensor_type', '')

        # Scaling factors (default 1:1 if missing or invalid in JSON)
        try:
            self._multi_factor = float(
                self._top_dev_attr.get('multi_factor',
                    self._i2c_dev_attr.get('multi_factor', '1')))
        except (ValueError, TypeError):
            self._multi_factor = 1.0
        try:
            self._decimate_factor = float(
                self._top_dev_attr.get('decimate_factor',
                    self._i2c_dev_attr.get('decimate_factor', '1')))
        except (ValueError, TypeError):
            self._decimate_factor = 1.0

    def get_name(self):
        return self.display_name

    # ------------------------------------------------------------------ #
    #  Value read — dispatches by sensor_type
    # ------------------------------------------------------------------ #
    def get_value(self):
        """Read voltage value in millivolts.  Returns float or None."""
        output = self.pddf_obj.get_attr_name_output(self.sensor_obj_name, "volt1_input")
        if not output:
            return None

        try:
            return self._scale(float(output['status']))
        except (KeyError, TypeError, ValueError):
            return None

    # ------------------------------------------------------------------ #
    #  Scaling
    # ------------------------------------------------------------------ #
    def _scale(self, raw_value):
        """Apply multi/decimate scaling.  Returns millivolts (whole number)."""
        if raw_value is None or self._decimate_factor == 0:
            return None
        # Truncate to integer millivolts, return as float for API consistency
        return float(int(raw_value * self._multi_factor / self._decimate_factor))

    # ------------------------------------------------------------------ #
    #  Thresholds — sysfs first (chip hw), JSON dev_attr fallback
    # ------------------------------------------------------------------ #
    def _get_threshold_from_sysfs(self, attr_name):
        """Read threshold from hwmon sysfs via PDDF attr_list mapping.

        Works when the chip supports the PMBus warn/fault registers
        (e.g. inN_max, inN_min, inN_crit, inN_lcrit) and the JSON
        attr_list maps them via drv_attr_name.
        """
        output = self.pddf_obj.get_attr_name_output(self.sensor_obj_name, attr_name)
        if not output:
            return None
        try:
            return float(output['status'])
        except (KeyError, TypeError, ValueError):
            return None

    def _get_threshold_from_json(self, key):
        """Fallback: look up static threshold from dev_attr in JSON.

        Used for chips that don't support warn limit registers
        (e.g. TPS53659/TPS53622 lack VOUT_OV_WARN / VOUT_UV_WARN).
        """
        val = self._top_dev_attr.get(key)
        if val is None:
            val = self._i2c_dev_attr.get(key)
        if val is None:
            return None
        try:
            return float(val)
        except (ValueError, TypeError):
            return None

    def get_low_threshold(self):
        # Try sysfs first (chip hw warn limit)
        val = self._get_threshold_from_sysfs("volt1_low_threshold")
        if val is not None:
            return val
        # Fallback: static JSON threshold
        return self._get_threshold_from_json('low_threshold')

    def get_high_threshold(self):
        val = self._get_threshold_from_sysfs("volt1_high_threshold")
        if val is not None:
            return val
        return self._get_threshold_from_json('high_threshold')

    def get_high_critical_threshold(self):
        val = self._get_threshold_from_sysfs("volt1_crit_high_threshold")
        if val is not None:
            return val
        return self._get_threshold_from_json('crit_high_threshold')

    def get_low_critical_threshold(self):
        val = self._get_threshold_from_sysfs("volt1_crit_low_threshold")
        if val is not None:
            return val
        return self._get_threshold_from_json('crit_low_threshold')