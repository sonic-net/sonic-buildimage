#!/usr/bin/env python

#############################################################################
# Ciena Voltage Sensor
#
# Supports two voltage sensor types:
#   1. MAX1139 ADC (IIO sysfs) — board-level 12V rails
#   2. FPGA PMBus VI_MON (direct FPGA reads) — VRM monitor outputs
#
# Delegates MAX1139 reads to PddfVoltageSensor (BMC commands).
# Routes VRM reads to vrm_sensor.read_vrm_field() for direct FPGA access.
#
# SONiC VoltageSensorBase expects get_value() to return millivolts (mV).
#############################################################################

import logging

try:
    from sonic_platform_pddf_base.pddf_voltage_sensor import PddfVoltageSensor
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

# Conditional import of VRM helpers (used if vrm_device_name is present)
from sonic_platform.vrm_sensor import (
    read_vrm_field,
    _get_pci_address,
    configure_vrm_sensor_dev_attr,
)

import os

logger = logging.getLogger(__name__)


class VoltageSensor(PddfVoltageSensor):
    """
    Ciena Platform-specific VoltageSensor class.

    Supports both PDDF-based and VRM-based voltage sensors:
      - PDDF: Reads MAX1139 ADC via IIO sysfs (VOLTAGE1-3)
      - VRM: Reads FPGA PMBus VI_MON registers directly (VOLTAGE4-12)

    VRM detection: If dev_attr contains vrm_device_name and vrm_field_name,
    uses direct FPGA read instead of BMC commands.
    """

    def __init__(self, index, pddf_data, pddf_plugin_data):
        PddfVoltageSensor.__init__(self, index, pddf_data, pddf_plugin_data)
        self._min_recorded = None
        self._max_recorded = None
        
        # Extract VRM metadata if present
        self._vrm_device_name = None
        self._vrm_field_name = None
        self._vrm_high_threshold = None
        self._vrm_low_threshold = None
        self._vrm_high_crit_threshold = None
        self._vrm_low_crit_threshold = None
        try:
            if pddf_data and hasattr(pddf_data, 'data') and isinstance(pddf_data.data, dict):
                if configure_vrm_sensor_dev_attr:
                    vrm_sensor = pddf_data.data.get('VRM_SENSOR', {})
                    configure_vrm_sensor_dev_attr(vrm_sensor.get('dev_attr', {}))

                sensor_list = pddf_data.data.get('VOLTAGE%d' % (index + 1), {})
                dev_attr = sensor_list.get('dev_attr', {})
                dev_info = sensor_list.get('dev_info', {})
                self._vrm_device_name = dev_attr.get('vrm_device_name')
                self._vrm_field_name = dev_attr.get('vrm_field_name')
                if self._vrm_device_name:
                    self._vrm_high_threshold = dev_info.get('high_threshold')
                    self._vrm_low_threshold = dev_info.get('low_threshold')
                    self._vrm_high_crit_threshold = dev_info.get('high_crit_threshold')
                    self._vrm_low_crit_threshold = dev_info.get('low_crit_threshold')
        except Exception:
            pass

    # ------------------------------------------------------------------
    # DeviceBase methods
    # ------------------------------------------------------------------

    def get_name(self):
        try:
            return PddfVoltageSensor.get_name(self)
        except Exception:
            return "N/A"

    def get_presence(self):
        if self._vrm_device_name:
            # VRM sensor: check PCI/regmap availability
            if read_vrm_field and _get_pci_address:
                pci = _get_pci_address()
                return pci is not None or (_REGMAP_READ and os.path.exists(_REGMAP_READ))
            return False
        # PDDF sensor: always present
        return True

    def get_model(self):
        if self._vrm_device_name:
            return "VRM PMBus ({})".format(self._vrm_device_name)
        return "MAX1139 ADC"

    def get_serial(self):
        return "N/A"

    def get_status(self):
        return self.get_presence()

    def get_position_in_parent(self):
        return -1

    def is_replaceable(self):
        return False

    # ------------------------------------------------------------------
    # SensorBase / VoltageSensorBase methods
    # ------------------------------------------------------------------

    def get_value(self):
        """
        Retrieves measurement reported by sensor in millivolts.

        For VRM sensors: reads directly from FPGA PMBus VI_MON register.
        For PDDF sensors: reads via BMC commands to IIO sysfs.

        Returns:
            A float number of voltage in mV, e.g. 12015.0 for ~12.015 V
        """
        mv = 0.0
        
        # VRM path: direct FPGA read
        if self._vrm_device_name and self._vrm_field_name and read_vrm_field and _get_pci_address:
            try:
                pci = _get_pci_address()
                volts = read_vrm_field(pci, self._vrm_device_name, self._vrm_field_name)
                if volts is not None:
                    mv = volts * 1000.0
            except Exception as e:
                logger.debug("VRM read failed: %s", e)
                return 0.0
        else:
            # PDDF path: BMC commands via IIO
            try:
                mv = float(PddfVoltageSensor.get_value(self))
            except Exception:
                return 0.0

        # Track min/max
        if self._min_recorded is None or mv < self._min_recorded:
            self._min_recorded = mv
        if self._max_recorded is None or mv > self._max_recorded:
            self._max_recorded = mv

        return float("{:.1f}".format(mv))

    def get_high_threshold(self):
        """High warning threshold in mV."""
        if self._vrm_device_name:
            return self._vrm_high_threshold
        try:
            return float(PddfVoltageSensor.get_high_threshold(self))
        except Exception:
            pass
        return 12600.0

    def get_low_threshold(self):
        """Low warning threshold in mV."""
        if self._vrm_device_name:
            return self._vrm_low_threshold
        try:
            return float(PddfVoltageSensor.get_low_threshold(self))
        except Exception:
            pass
        return 11400.0

    def get_high_critical_threshold(self):
        """High critical threshold in mV."""
        if self._vrm_device_name:
            return self._vrm_high_crit_threshold
        try:
            return float(PddfVoltageSensor.get_high_critical_threshold(self))
        except Exception:
            pass
        return 13200.0

    def get_low_critical_threshold(self):
        """Low critical threshold in mV."""
        if self._vrm_device_name:
            return self._vrm_low_crit_threshold
        try:
            return float(PddfVoltageSensor.get_low_critical_threshold(self))
        except Exception:
            pass
        return 10800.0

    def set_high_threshold(self, value):
        return False

    def set_low_threshold(self, value):
        return False

    def get_minimum_recorded(self):
        """Returns the minimum recorded voltage in mV."""
        if self._min_recorded is None:
            # Force a read so we have at least one sample
            self.get_value()
        return self._min_recorded if self._min_recorded is not None else 0.0

    def get_maximum_recorded(self):
        """Returns the maximum recorded voltage in mV."""
        if self._max_recorded is None:
            self.get_value()
        return self._max_recorded if self._max_recorded is not None else 0.0
