#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import logging
from sonic_platform_pddf_base.pddf_asic_thermal import PddfAsicThermal
from swsscommon import swsscommon
from sonic_py_common import syslogger
from nexthop.pddf_config_parser import load_platform_json
from .thermal import PidThermalMixin, MinMaxTempMixin

SYSLOG_IDENTIFIER = "nh_asic_thermal"
log = syslogger.SysLogger(SYSLOG_IDENTIFIER, log_level=logging.INFO, enable_runtime_config=False)


class AsicThermal(PddfAsicThermal, MinMaxTempMixin, PidThermalMixin):
    """PDDF Platform-Specific ASIC Thermal class"""

    _polling_enabled_checked = False

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

    def _get_platform_asic_sensor_config(self):
        """
        Read ASIC sensor configuration from platform.json.
        Returns a tuple of (poll_interval, poll_admin_status) or (None, None) if not found.
        """
        try:
            platform_json = load_platform_json()

            asic_sensors = platform_json.get("asic_sensors", {})
            if not asic_sensors:
                return None, None

            poll_interval = asic_sensors.get("poll_interval")
            poll_admin_status = asic_sensors.get("poll_admin_status")

            return poll_interval, poll_admin_status
        except Exception as e:
            log.log_warning(f"Failed to read ASIC sensor configuration from platform.json: {str(e)}")
            return None, None

    def _ensure_asic_sensor_polling_enabled(self):
        """
        Ensure ASIC sensor polling is enabled in CONFIG_DB.
        This is called every time we read temperature to guarantee polling is active.
        """
        if AsicThermal._polling_enabled_checked:
            return

        try:
            desired_interval, desired_admin_status = self._get_platform_asic_sensor_config()

            if desired_admin_status is None:
                AsicThermal._polling_enabled_checked = True
                return

            desired_interval = desired_interval if desired_interval is not None else "10"

            config_db = swsscommon.ConfigDBConnector()
            config_db.connect()

            asic_sensors_config = config_db.get_table("ASIC_SENSORS")

            poller_status = asic_sensors_config.get("ASIC_SENSORS_POLLER_STATUS", {})
            admin_status = poller_status.get("admin_status", "")

            if admin_status != desired_admin_status:
                config_db.mod_entry("ASIC_SENSORS", "ASIC_SENSORS_POLLER_STATUS",
                                  {"admin_status": desired_admin_status})
                log.log_info(f"Updated ASIC sensor polling admin_status to {desired_admin_status}")

            poller_interval = asic_sensors_config.get("ASIC_SENSORS_POLLER_INTERVAL", {})
            current_interval = poller_interval.get("interval", "")

            if current_interval != desired_interval:
                config_db.mod_entry("ASIC_SENSORS", "ASIC_SENSORS_POLLER_INTERVAL",
                                  {"interval": desired_interval})
                log.log_info(f"Updated ASIC sensor polling interval to {desired_interval}")

            AsicThermal._polling_enabled_checked = True

        except Exception as e:
            log.log_error(f"Failed to update ASIC sensor polling: {str(e)}")
            pass

    def get_temperature(self):
        self._ensure_asic_sensor_polling_enabled()

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
