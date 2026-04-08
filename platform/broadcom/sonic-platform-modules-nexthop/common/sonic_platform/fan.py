#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

try:
    from sonic_platform_pddf_base.pddf_fan import PddfFan
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")
from sonic_py_common import syslogger
from swsscommon import swsscommon

_FAN_INFO_TABLE_NAME = "FAN_INFO"
_STATE_MAX_SPEED_KEY = "max_speed"

SYSLOG_IDENTIFIER = "sonic_platform.fan"
logger = syslogger.SysLogger(SYSLOG_IDENTIFIER)

def _try_get_state_db_table(table_name) -> swsscommon.Table | None:
    """
    Attempts to establish a connection to STATE_DB and returns the table.

    If failed, it is likely that redis-server is not up yet, especially
    when SONiC Platform is initialized by some services early after boot.
    Ignore the error and return None to not break fan initialization, as
    we will retry later when needed by the runtime operations (and we expect
    redis-server to already be up by then, e.g. when utilized by pmon container).
    """
    try:
        state_db = swsscommon.DBConnector("STATE_DB", 0)
    except Exception as e:
        logger.log_warning(f"Failed to connect to STATE_DB: {e}. Ignoring.")
        return None
    return swsscommon.Table(state_db, table_name)

class Fan(PddfFan):
    """PDDF Platform-Specific Fan class"""

    # Default maximum speed if not specified in STATE_DB
    _DEFAULT_MAX_SPEED = 75.0

    def __init__(
        self,
        tray_idx,
        fan_idx=0,
        pddf_data=None,
        pddf_plugin_data=None,
        is_psu_fan=False,
        psu_index=0,
    ):
        # idx is 0-based
        PddfFan.__init__(
            self, tray_idx, fan_idx, pddf_data, pddf_plugin_data, is_psu_fan, psu_index
        )
        self._state_fan_tbl = _try_get_state_db_table(_FAN_INFO_TABLE_NAME)

    def get_model(self):
        if self.get_presence() and not self.is_psu_fan:
            return "FAN-80G1-F"
        return "N/A"

    def get_serial(self):
        return "N/A"

    def get_psu_presence(self):
        if not self.is_psu_fan:
            return False

        status = 0
        device = "PSU{}".format(self.fans_psu_index)
        output = self.pddf_obj.get_attr_name_output(device, "psu_present")
        if not output:
            return False

        mode = output['mode']
        status = output['status']

        vmap = self.plugin_data['PSU']['psu_present'][mode]['valmap']

        if status.rstrip('\n') in vmap:
            return vmap[status.rstrip('\n')]
        else:
            return False

    def get_presence(self):
        if self.is_psu_fan:
            return self.get_psu_presence()
        return PddfFan.get_presence(self)

    def set_speed(self, speed):
        max_speed = self.get_max_speed()
        if speed > max_speed:
            logger.log_warning(
                f"Requested fan speed {speed}% exceeds current max {max_speed}%; "
                "capping to max."
            )
            speed = max_speed
        return PddfFan.set_speed(self, speed)

    def get_max_speed(self):
        """
        Gets maximum speed for this fan from STATE_DB's FAN_INFO table.

        If the key doesn't exist in the DB, return the default maximum speed of 75%.

        Returns:
            Maximum speed for this fan in percent (0-100)
        """
        if self._state_fan_tbl is None:
            self._state_fan_tbl = _try_get_state_db_table(_FAN_INFO_TABLE_NAME)
            if self._state_fan_tbl is None:
                logger.log_error(
                    "Can't connect to 'STATE_DB:FAN_INFO' table; "
                    f"fallback to default max_speed={self._DEFAULT_MAX_SPEED}%."
                )
                return self._DEFAULT_MAX_SPEED

        fan_name = PddfFan.get_name(self)
        found, data = self._state_fan_tbl.get(fan_name)
        if not found:
            logger.log_error(
                f"'STATE_DB:FAN_INFO|{fan_name}' not found; "
                f"fallback to default max_speed={self._DEFAULT_MAX_SPEED}%"
            )
            return self._DEFAULT_MAX_SPEED

        data_dict = dict(data)
        if _STATE_MAX_SPEED_KEY not in data_dict:
            logger.log_error(
                f"max_speed not found in 'STATE_DB:FAN_INFO|{fan_name}'; "
                f"fallback to default={self._DEFAULT_MAX_SPEED}%."
            )
            return self._DEFAULT_MAX_SPEED
        return float(data_dict[_STATE_MAX_SPEED_KEY])

    def set_max_speed(self, max_speed):
        """
        Sets maximum speed for this fan into STATE_DB's FAN_INFO table.

        We write it onto the DB so that Fan instances across containers will see the same value.

        Args:
            max_speed: Maximum speed in percent (0-100)
        
        Returns:
            True if the maximum speed is stored successfully, False otherwise.
        """
        if self._state_fan_tbl is None:
            self._state_fan_tbl = _try_get_state_db_table(_FAN_INFO_TABLE_NAME)
            if self._state_fan_tbl is None:
                logger.log_error(
                    "Can't connect to 'STATE_DB:FAN_INFO' table; "
                    f"ignore writing max_speed={self._DEFAULT_MAX_SPEED}% "
                    f"for {self.get_name()}."
                )
                return False

        fvp = swsscommon.FieldValuePairs([(_STATE_MAX_SPEED_KEY, str(max_speed))])
        self._state_fan_tbl.set(PddfFan.get_name(self), fvp)
        return True
