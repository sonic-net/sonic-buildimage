#!/usr/bin/python3

"""
Front-panel LED control daemon
"""

import sys

from nexthop.graceful_exitter import GracefulExitter
from sonic_py_common import daemon_base
from sonic_py_common.interface import backplane_prefix, inband_prefix, recirc_prefix
from swsscommon import swsscommon

SYSLOG_IDENTIFIER = "port-ledd"

LED_MODULE_NAME = "led_control"
LED_CLASS_NAME = "LedControl"

SELECT_TIMEOUT = 1000

APP_ADMIN_STATUS_KEY = "admin_status"
APP_OPER_STATUS_KEY = "oper_status"


class PortLedd(daemon_base.DaemonBase):

    def __init__(self):
        daemon_base.DaemonBase.__init__(self, SYSLOG_IDENTIFIER)

        # Load platform-specific LedControl module
        try:
            self.led_control = self.load_platform_util(LED_MODULE_NAME, LED_CLASS_NAME)
        except Exception as e:
            self.log_error("Failed to load led_control: %s" % (str(e)), True)
            sys.exit(1)

        self.sel = swsscommon.Select()

        # Subscribe to APPL_DB and STATE_DB events
        self.appl_db = daemon_base.db_connect("APPL_DB")
        self.state_db = daemon_base.db_connect("STATE_DB")

        self.app_port_tbl = swsscommon.SubscriberStateTable(
            self.appl_db, swsscommon.APP_PORT_TABLE_NAME
        )
        self.sel.addSelectable(self.app_port_tbl)

        self.state_xcvr_tbl = swsscommon.SubscriberStateTable(
            self.state_db, swsscommon.STATE_TRANSCEIVER_INFO_TABLE_NAME
        )
        self.sel.addSelectable(self.state_xcvr_tbl)

    def _ignore_key(self, key):
        if key in ["PortConfigDone", "PortInitDone"]:
            return True

        if key.startswith((backplane_prefix(), inband_prefix(), recirc_prefix())):
            return True

        return False

    def handle_port_event(self):
        key, op, fvp = self.app_port_tbl.pop()

        if self._ignore_key(key):
            return

        if op == "DEL":
            self.led_control.port_link_state_change(key)
        elif op == "SET" and fvp:
            fvp_dict = dict(fvp)
            if APP_ADMIN_STATUS_KEY in fvp_dict or APP_OPER_STATUS_KEY in fvp_dict:
                self.led_control.port_link_state_change(key)

        return

    def handle_xcvr_event(self):
        key, op, _ = self.state_xcvr_tbl.pop()

        if self._ignore_key(key):
            return

        if op == "SET" or op == "DEL":
            self.led_control.port_link_state_change(key)

        return

    def run(self):
        state, _ = self.sel.select(SELECT_TIMEOUT)

        if state == swsscommon.Select.TIMEOUT:
            return

        if state != swsscommon.Select.OBJECT:
            self.log_warning("sel.select() did not return swsscommon.Select.OBJECT")
            return

        self.handle_port_event()
        self.handle_xcvr_event()

        return


def main():
    exitter = GracefulExitter(SYSLOG_IDENTIFIER)
    ledd = PortLedd()

    while not exitter.should_exit():
        ledd.run()


if __name__ == "__main__":
    main()
