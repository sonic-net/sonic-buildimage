#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import abc
import re
import threading
import time

from nexthop import fpga_lib
from sonic_platform_base.thermal_base import ThermalBase
from sonic_platform_pddf_base.pddf_thermal import PddfThermal
from sonic_py_common import syslogger
from swsscommon import swsscommon

from sonic_platform.syslog import SYSLOG_IDENTIFIER_THERMAL

# Nexthop FPGA thermal sensor value to Celsius conversion
TYPE_TO_CELSIUS_LAMBDA_DICT = {
  'th5': lambda field_val:  0.158851 * (3000 - field_val),
  'th6': lambda field_val:  -0.25968 * (field_val/2 - 1) + 378.85
}

thermal_syslogger = syslogger.SysLogger(SYSLOG_IDENTIFIER_THERMAL)


def _intf_name_key(intf_name):
    """Convert interface name to key for sorting"""
    match = re.match(r"Ethernet(\d+)", intf_name)
    if match:
        return int(match.group(1))
    return None


class PortIndexMapper:
    """Singleton class for mapping port index to interface name from CONFIG_DB PORT table"""
    _instance = None
    _lock = threading.Lock()

    def __new__(cls):
        if cls._instance is None:
            with cls._lock:
                if cls._instance is None:
                    cls._instance = super(PortIndexMapper, cls).__new__(cls)
                    cls._instance._initialized = False
        return cls._instance

    def __init__(self):
        if self._initialized:
            return
        self._initialized = True
        self._port_index_to_interface = {}
        self._mapping_lock = threading.RLock()

    def _build_mapping(self):
        """Build port index to interface mapping from CONFIG_DB"""
        with self._mapping_lock:
            cfg_db = swsscommon.SonicV2Connector(use_unix_socket_path=True)
            cfg_db.connect(cfg_db.CONFIG_DB)
            # Get the underlying DBConnector for swsscommon.Table
            db_connector = cfg_db.get_redis_client(cfg_db.CONFIG_DB)
            port_tbl = swsscommon.Table(db_connector, swsscommon.CFG_PORT_TABLE_NAME)

            self._port_index_to_interface.clear()

            for intf_name in port_tbl.getKeys():
                (status, port_data) = port_tbl.get(intf_name)
                if not status:
                    continue

                port_dict = dict(port_data)
                port_index = port_dict.get('index')
                try:
                    port_index = int(port_index)
                except (ValueError, TypeError):
                    continue

                dict_intf = self._port_index_to_interface.get(port_index)
                dict_intf_key = float("inf") if dict_intf is None else _intf_name_key(dict_intf)
                db_intf_key = _intf_name_key(intf_name)
                if db_intf_key is None:
                    continue

                # Map to lowest interface name for this port index
                if db_intf_key < dict_intf_key:
                    self._port_index_to_interface[port_index] = intf_name

    def get_interface_name(self, port_index):
        """Get interface name by port index"""
        with self._mapping_lock:
            port_index = int(port_index)
            if port_index not in self._port_index_to_interface:
                self._build_mapping()
            return self._port_index_to_interface.get(port_index)


class PidThermalMixin(abc.ABC):
    DEFAULT_PID_SETPOINT_MARGIN = 10.0

    """Mixin class for thermal objects that support PID control"""
    def __init__(self, pddf_device_data, margin_override=None, setpoint_override=None, pid_domain=None):

        # Setpoint margin
        if margin_override is None:
            self._pid_setpoint_margin = pddf_device_data.get('nexthop_thermal_setpoint_margin',
                                                             self.DEFAULT_PID_SETPOINT_MARGIN)
        else:
            self._pid_setpoint_margin = margin_override

        # Setpoint override
        if setpoint_override is None:
            self._pid_setpoint_override = pddf_device_data.get('nexthop_thermal_setpoint_override')
        else:
            self._pid_setpoint_override = setpoint_override

        # PID domain
        if pid_domain is None:
            self._pid_domain = pddf_device_data.get('nexthop_thermal_pid_domain')
        else:
            self._pid_domain = pid_domain

    def is_controlled_by_pid(self):
        return self._pid_domain is not None

    def get_pid_domain(self):
        return self._pid_domain

    @abc.abstractmethod
    def get_high_critical_threshold(self):
        """Expected to be implemented by derived classes."""
        raise NotImplementedError

    def get_pid_setpoint(self):
        if self._pid_setpoint_override is not None:
            return self._pid_setpoint_override
        # Threshold may change during the lifecycle of the thermal object. For example, a transceiver may be hotswapped.
        # Therefore, we need to query the threshold every time.
        val = self.get_high_threshold()
        if val is None:
            return None
        return val - self._pid_setpoint_margin


class MinMaxTempMixin:
    """Mixin class for thermal objects that support min/max temperature recording"""
    def __init__(self):
        self._min_temperature = None
        self._max_temperature = None

    def _update_min_max_temp(self, temp):
        if temp is None:
            return
        if not self._min_temperature or temp < self._min_temperature:
            self._min_temperature = temp
        if not self._max_temperature or temp > self._max_temperature:
            self._max_temperature = temp

    def get_minimum_recorded(self):
        return self._min_temperature

    def get_maximum_recorded(self):
        return self._max_temperature


class Thermal(PddfThermal, MinMaxTempMixin, PidThermalMixin):
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
        MinMaxTempMixin.__init__(self)

        dev_info = {} if is_psu_thermal else self.thermal_obj['dev_info']
        PidThermalMixin.__init__(self, dev_info)

    def get_model(self):
        return "N/A"

    def get_serial(self):
        return "N/A"

    def get_status(self):
        return self.get_presence()

    def get_temperature(self):
        temp = PddfThermal.get_temperature(self)
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


class NexthopFpgaAsicThermal(ThermalBase, MinMaxTempMixin, PidThermalMixin):
    """ASIC temperature sensor read from the FPGA register"""

    def __init__(
        self,
        index,
        position_offset,
        pddf_data
    ):
        ThermalBase().__init__()
        MinMaxTempMixin.__init__(self)

        pddf_obj_name = f'NEXTHOP-FPGA-ASIC-TEMP-SENSOR{index}'
        pddf_obj_data = pddf_data.data[pddf_obj_name]
        dev_info = pddf_obj_data['dev_info']

        PidThermalMixin.__init__(self, dev_info)

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
        self._thermal_position_in_parent = index + position_offset + 1

        self._supported = self._check_sensor_supported()

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

    def get_presence(self):
        return True

    def get_model(self):
        return "N/A"

    def get_serial(self):
        return "N/A"

    def get_revision(self):
        return "N/A"

    def get_status(self):
        return True

    def get_position_in_parent(self):
        return self._thermal_position_in_parent

    def is_replaceable(self):
        return False

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
        self._update_min_max_temp(val)
        return val

    def get_low_threshold(self):
        return None

    def get_low_critical_threshold(self):
        return None

    def get_high_threshold(self):
        val = self._temp1_high_threshold
        if val:
            return float(val)
        return None

    def get_high_critical_threshold(self):
        val = self._temp1_high_crit_threshold
        if val:
            return float(val)
        return None

    def get_minimum_recorded(self):
        # Make sure temp is recorded at least once.
        self.get_temperature()
        return MinMaxTempMixin.get_minimum_recorded(self)

    def get_maximum_recorded(self):
        # Make sure temp is recorded at least once.
        self.get_temperature()
        return MinMaxTempMixin.get_maximum_recorded(self)


class SfpThermal(ThermalBase, MinMaxTempMixin, PidThermalMixin):
    """SFP thermal interface class"""
    THRESHOLDS_CACHE_INTERVAL_SEC = 5
    MIN_VALID_SETPOINT = 30.0
    DEFAULT_SETPOINT = 62.0

    def __init__(self, sfp, pddf_data):
        self._sfp = sfp
        self._min_temperature = None
        self._max_temperature = None
        self._threshold_info = {}
        self._threshold_info_time = 0
        self._invalid_setpoint_logged = False
        self._state_db = None
        ThermalBase.__init__(self)
        MinMaxTempMixin.__init__(self)
        PidThermalMixin.__init__(self, pddf_data.data["PLATFORM"],
                                 pddf_data.data["PLATFORM"].get('nexthop_thermal_xcvr_setpoint_margin'),
                                 pddf_data.data["PLATFORM"].get('nexthop_thermal_xcvr_setpoint_override'),
                                 pddf_data.data["PLATFORM"].get('nexthop_thermal_xcvr_pid_domain'))

    def __del__(self):
        """Cleanup database connection when object is destroyed"""
        if self._state_db:
            try:
                self._state_db.close(self._state_db.STATE_DB)
            except (AttributeError, ConnectionError, OSError):
                pass  # Ignore errors during cleanup

    def get_name(self):
        return f"Transceiver {self._sfp.get_name().capitalize()}"

    def get_presence(self):
        presence = self._sfp.get_presence()
        if not presence:
            self._threshold_info = {}
            self._threshold_info_time = 0
            self._invalid_setpoint_logged = False
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
        self._update_min_max_temp(temp)
        return temp

    def _get_state_db(self):
        """Connect to STATE_DB if not already connected"""
        if not self._state_db:
            self._state_db = swsscommon.SonicV2Connector(use_unix_socket_path=True)
            self._state_db.connect(self._state_db.STATE_DB)
        return self._state_db

    def maybe_update_threshold_info(self):
        time_elapsed = time.monotonic() - self._threshold_info_time
        if time_elapsed < self.THRESHOLDS_CACHE_INTERVAL_SEC:
            return
        self._threshold_info_time = time.monotonic()
        if not self.get_presence():
            return

        self._state_db = self._get_state_db()
        port_mapper = PortIndexMapper()
        intf_name = port_mapper.get_interface_name(self._sfp.get_position_in_parent())
        if not intf_name:
            thermal_syslogger.log_warning(f"Failed to get interface name for port {self._sfp.get_position_in_parent()}"
                                          ". Threshold information will not be available.")
            return

        threshold_key = f"TRANSCEIVER_DOM_THRESHOLD|{intf_name}"
        threshold_data = self._state_db.get_all(self._state_db.STATE_DB, threshold_key)

        if threshold_data:
            # Convert string values to float, handle "N/A" and invalid values
            self._threshold_info = {}
            for key, value in threshold_data.items():
                if isinstance(value, str) and value.lower() in ['n/a', 'none', '']:
                    self._threshold_info[key] = None
                else:
                    try:
                        self._threshold_info[key] = float(value)
                    except (ValueError, TypeError):
                        self._threshold_info[key] = None

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
        # Make sure temp is recorded at least once.
        self.get_temperature()
        return MinMaxTempMixin.get_minimum_recorded(self)

    def get_maximum_recorded(self):
        # Make sure temp is recorded at least once.
        self.get_temperature()
        return MinMaxTempMixin.get_maximum_recorded(self)

    def get_pid_setpoint(self):
        setpoint = super().get_pid_setpoint()
        if setpoint is None:
            if self.get_presence():
                # Threshold info may be a little bit delayed as it is read by xcvrd. Start with a default setpoint
                # until thresholds are available.
                return self.DEFAULT_SETPOINT
            else:
                return setpoint
        # Setpoint cannot be guaranteed on pluggables - some modules may have invalid values such as 0.
        # For these cases, use a default setpoint.
        if setpoint < self.MIN_VALID_SETPOINT:
            if not self._invalid_setpoint_logged:
                thermal_syslogger.log_warning(f"Invalid setpoint {setpoint:.1f} for {self.get_name()}, "
                                              f"using default setpoint {self.DEFAULT_SETPOINT:.1f}")
                self._invalid_setpoint_logged = True
            return self.DEFAULT_SETPOINT
        return setpoint
