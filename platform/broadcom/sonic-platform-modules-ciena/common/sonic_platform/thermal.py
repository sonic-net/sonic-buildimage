#!/usr/bin/env python

#############################################################################
# Ciena Thermal Sensor
#
# Reads temperatures and trip-point thresholds from the Linux thermal
# subsystem:  /sys/class/thermal/thermal_zone{N}/
#
# Trip-point mapping (kernel thermal framework):
#   trip_point_0_temp  →  high threshold  (get_high_threshold)
#   trip_point_1_temp  →  low threshold   (get_low_threshold)
#   trip_point_2_temp  →  critical high   (get_high_critical_threshold)
#     Falls back to trip_point_0 if trip_point_2 does not exist.
#
# All sysfs values are in millidegrees Celsius (e.g. 89000 → 89.0 °C).
#############################################################################

import logging
import os

try:
    from sonic_platform_pddf_base.pddf_thermal import PddfThermal
    from sonic_platform.helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

# VRM thermals can be represented as TEMP entries in PDDF and read
# from FPGA PMBus shadow registers instead of Linux thermal_zone sysfs.
from sonic_platform.vrm_sensor import (
    read_vrm_field,
    _get_pci_address,
    get_regmap_read_path,
    configure_vrm_sensor_dev_attr,
)

logger = logging.getLogger(__name__)

def _get_pddf_json(pddf_obj):
    """Return parsed PDDF JSON dict from PDDF object (or dict input)."""
    if pddf_obj is None:
        return {}
    pddf_json = getattr(pddf_obj, "data", pddf_obj)
    return pddf_json if isinstance(pddf_json, dict) else {}


def _get_thermal_dev_attr(pddf_obj, thermal_index, attr_name):
    """Get TEMPx dev_attr value from PDDF JSON."""
    pddf_json = _get_pddf_json(pddf_obj)
    thermal_key = "TEMP{}".format(thermal_index)
    dev_attr = pddf_json.get(thermal_key, {}).get("dev_attr", {})
    if isinstance(dev_attr, dict):
        return dev_attr.get(attr_name)


class Thermal(PddfThermal):
    """Ciena Platform-specific Thermal class"""

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None, is_psu_thermal=False, psu_index=0):
        PddfThermal.__init__(self, index - 1, pddf_data, pddf_plugin_data)
        self.index = index
        self._api_helper = APIHelper()
        self._base_path_tmpl = _get_thermal_dev_attr(
            self.pddf_obj, self.index, "THERMAL_ZONE_BASE"
        )
        self._min_temperature = None
        self._max_temperature = None
        # Cache the sysfs "type" string (read once)
        self._zone_type = self._read_sysfs("type")
        if self._zone_type:
            self._zone_type = self._zone_type.strip()

        if configure_vrm_sensor_dev_attr and pddf_data and hasattr(pddf_data, 'data') and isinstance(pddf_data.data, dict):
            vrm_sensor_entry = pddf_data.data.get('VRM_SENSOR', {})
            configure_vrm_sensor_dev_attr(vrm_sensor_entry.get('dev_attr', {}))

        self._vrm_device_name = _get_thermal_dev_attr(
            self.pddf_obj, self.index, "vrm_device_name"
        )

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------

    def _get_base_path(self):
        """Return thermal zone base path with index expanded."""
        try:
            return self._base_path_tmpl.format(index=self.index)
        except (AttributeError, IndexError, KeyError, ValueError):
            return None

    def _read_sysfs(self, filename):
        """Read a sysfs file under this thermal zone, return string or None."""
        base_path = self._get_base_path()        
        if base_path is None:
            return None
        path = os.path.join(base_path, filename)
        return self._api_helper.read_txt_file(path)

    def _get_default_threshold(self, attr_name):
        """Get default threshold value from TEMPx.dev_attr as float."""
        value = _get_thermal_dev_attr(self.pddf_obj, self.index, attr_name)
        try:
            if value is not None:
                return float(value)
        except (ValueError, TypeError):
            pass
        return None

    # ------------------------------------------------------------------
    # DeviceBase methods
    # ------------------------------------------------------------------

    def get_name(self):
        """Return a human-readable sensor name derived from the sysfs type.

        Format: "<friendly name> (<sysfs_type>)" so that both the
        descriptive label and the raw thermal zone type are visible
        in 'show platform temperature'.
        """
        try:
            return PddfThermal.get_name(self)
        except Exception:
            pass
        return "thermal_zone{}".format(self.index)

    def get_presence(self):
        if self._vrm_device_name:
            if _get_pci_address is None:
                return False
            pci = _get_pci_address()
            regmap_path = get_regmap_read_path() if get_regmap_read_path else None
            return pci is not None or (regmap_path and os.path.exists(regmap_path))
        return os.path.exists(os.path.join(self._base_path, "temp"))

    def get_model(self):
        return "N/A"

    def get_serial(self):
        return "N/A"

    def get_status(self):
        return self.get_presence()

    def get_position_in_parent(self):
        return self.index + 1

    def is_replaceable(self):
        return False

    # ------------------------------------------------------------------
    # ThermalBase methods
    # ------------------------------------------------------------------

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal zone.

        Returns:
            A float number of current temperature in Celsius, e.g. 30.125
        """        
        if self._vrm_device_name and read_vrm_field is not None and _get_pci_address is not None:
            pci = _get_pci_address()
            if pci is None:
                return 0.0
            temp = read_vrm_field(pci, self._vrm_device_name, "READ_TEMP1")
            if temp is None:
                temp = 0.0
            else:
                temp = float("{:.1f}".format(temp))
        else:
            try:
                temp = PddfThermal.get_temperature(self)
                if temp is None:
                    return 0.0
            except Exception:
                return 0.0

        # Track min/max
        if self._min_temperature is None or temp < self._min_temperature:
            self._min_temperature = temp
        if self._max_temperature is None or temp > self._max_temperature:
            self._max_temperature = temp

        return temp

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature.

        Reads trip_point_0_temp; falls back to a per-sensor-type default
        if the trip point is absent or bogus.

        Returns:
            A float number in Celsius, e.g. 85.0
        """
        try:
            threshold = PddfThermal.get_high_threshold(self)
            if threshold not in (None, 0, 0.0):
                return threshold
        except Exception:
            pass
        threshold = self._get_default_threshold("default_high_threshold")
        if threshold is not None:
            return threshold
        return 0.0

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature.

        Linux thermal zones only define high-side trip points, so there
        is no meaningful low threshold.  Return 0.0 (not applicable).

        Returns:
            A float number in Celsius (always 0.0).
        """
        return 0.0

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature.

        Tries trip_point_2_temp → trip_point_0_temp → per-type default.

        Returns:
            A float number in Celsius, e.g. 105.0
        """
        try:
            threshold = PddfThermal.get_high_critical_threshold(self)
            if threshold not in (None, 0, 0.0):
                return threshold
        except Exception:
            pass
        threshold = self._get_default_threshold("default_high_critical_threshold")
        if threshold is not None:
            return threshold
        return 0.0

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature.

        Not supported on this platform.
        """
        return 0.0

    def set_high_threshold(self, temperature):
        """
        Sets the high threshold temperature (trip_point_0_temp).
        """
        base_path = self._get_base_path()
        if base_path is None:
            return False
        path = os.path.join(base_path, "trip_point_0_temp")
        try:
            temp_mdeg = int(temperature * 1000)
            return self._api_helper.write_txt_file(path, temp_mdeg)
        except Exception:
            return False

    def set_low_threshold(self, temperature):
        """
        Sets the low threshold temperature (trip_point_1_temp).
        """
        base_path = self._get_base_path()
        if base_path is None:
            return False
        path = os.path.join(base_path, "trip_point_1_temp")
        try:
            temp_mdeg = int(temperature * 1000)
            return self._api_helper.write_txt_file(path, temp_mdeg)
        except Exception:
            return False

    def get_minimum_recorded(self):
        """Retrieves the minimum recorded temperature in Celsius."""
        if self._min_temperature is None:
            self.get_temperature()
        return self._min_temperature if self._min_temperature is not None else 0.0

    def get_maximum_recorded(self):
        """Retrieves the maximum recorded temperature in Celsius."""
        if self._max_temperature is None:
            self.get_temperature()
        return self._max_temperature if self._max_temperature is not None else 0.0
