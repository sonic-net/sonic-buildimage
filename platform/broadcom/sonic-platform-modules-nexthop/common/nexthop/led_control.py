# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

# Platform-specific LED control functionality for SONiC

import syslog
from swsscommon.swsscommon import SonicV2Connector

try:
    import sonic_platform.platform
    from sonic_led.led_control_base import LedControlBase
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


LOG_ID = "nexthop-led-control"


def log_err(msg):
    syslog.openlog(ident=LOG_ID)
    syslog.syslog(syslog.LOG_INFO, msg)
    syslog.closelog()


APP_PORT_PREFIX = "PORT_TABLE:"
STATE_XCVR_PREFIX = "TRANSCEIVER_INFO|"


def get_chassis():
    return sonic_platform.platform.Platform().get_chassis()


class NexthopLedControlBase(LedControlBase):
    """Platform specific LED control class"""

    def __init__(self):
        self.db = SonicV2Connector()
        self.db.connect(self.db.APPL_DB)
        self.db.connect(self.db.STATE_DB)

    def _get_port_status(self, port):
        admin_status = self.db.get(
            self.db.APPL_DB, APP_PORT_PREFIX + port, "admin_status"
        )
        if admin_status is None:
            admin_status = "down"
        oper_status = self.db.get(
            self.db.APPL_DB, APP_PORT_PREFIX + port, "oper_status"
        )
        return admin_status, oper_status

    def _get_xcvr_info(self, port):
        return self.db.get_all(self.db.STATE_DB, STATE_XCVR_PREFIX + port)

    def _get_xcvr_presence(self, port_num):
        logical_ports = self._get_interfaces_for_port(port_num)
        if not logical_ports:
            log_err(f"Could not find logical interfaces for Port{port_num}")
            return False
        # TRANSCEIVER_INFO is only set for the first interface in the breakout
        base_port = logical_ports[0]
        xcvr_info = self._get_xcvr_info(base_port)
        if xcvr_info:
            return True
        return False

    def _set_led_color(self, port, color):
        led_device_name = f"PORT_LED_{port}"
        if not get_chassis().set_system_led(led_device_name, color):
            log_err(f"Error setting {led_device_name} to {color}")
        return

    def _get_port_num(self, interface):
        return int(self.db.get(self.db.APPL_DB, APP_PORT_PREFIX + interface, "index"))

    def _get_interfaces_for_port(self, port_num):
        raise NotImplementedError

    def port_link_state_change(self, port):
        """
        Called when port link state changes:
        admin_status, oper_status, or xcvr presence

        :param port: A string, SONiC port name (e.g., "Ethernet0")
        """
        port_num = self._get_port_num(port)
        if port_num is None:
            log_err(f"Unexpected port name: {port}")
            return

        color = "green"
        all_oper_down = True
        all_admin_disabled = True
        logical_ports = self._get_interfaces_for_port(port_num)

        for logical_port in logical_ports:
            admin_status, oper_status = self._get_port_status(logical_port)

            if oper_status is None:
                continue

            if oper_status == "up":
                all_oper_down = False

            if admin_status == "up":
                all_admin_disabled = False

            if admin_status != oper_status:
                color = "yellow"

        if all_oper_down:
            if all_admin_disabled:
                color = "off"
            else:
                if not self._get_xcvr_presence(port_num):
                    color = "off"
        else:
            if not self._get_xcvr_presence(port_num):
                color = "yellow"

        self._set_led_color(port_num, color)
        return
