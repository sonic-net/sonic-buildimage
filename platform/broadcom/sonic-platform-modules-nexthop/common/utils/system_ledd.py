#!/usr/bin/env python3

"""
System LED control daemon
"""

import sonic_platform
from nexthop.graceful_exitter import GracefulExitter
from sonic_py_common import daemon_base

SYSLOG_IDENTIFIER = "system-ledd"
UPDATE_INTERVAL_SEC = 5

FANS_OK = "green"
FANS_NOT_OK = "red"
FAN_DRAWER_OK = "green"
FAN_DRAWER_NOT_OK = "red"
PSUS_OK = "green"
PSUS_NOT_OK = "red"


class SystemLedd(daemon_base.DaemonBase):

    def __init__(self):
        daemon_base.DaemonBase.__init__(self, SYSLOG_IDENTIFIER)
        self.chassis = sonic_platform.platform.Platform().get_chassis()
        self.chassis.pddf_obj.set_system_led_color("LOC_LED", "off")

    def _set_fan_drawer_led(self, fan_drawer, color):
        curr = fan_drawer.get_status_led()
        if color != curr:
            result = fan_drawer.set_status_led(color)
            if result:
                self.log_info(
                    f"Setting FANTRAY{fan_drawer.fantray_index}_LED to {color}"
                )
            else:
                self.log_error(
                    f"Failed to set FANTRAY{fan_drawer.fantray_index}_LED to {color}"
                )

    def _set_fan_led(self, color):
        _, curr = self.chassis.pddf_obj.get_system_led_color("FAN_LED")
        if color != curr:
            result, _ = self.chassis.pddf_obj.set_system_led_color("FAN_LED", color)
            if result:
                self.log_info(f"Setting FAN_LED to {color}")
            else:
                self.log_error(f"Failed to set FAN_LED to {color}")

    def _set_psu_led(self, color):
        _, curr = self.chassis.pddf_obj.get_system_led_color("PSU_LED")
        if color != curr:
            result, _ = self.chassis.pddf_obj.set_system_led_color("PSU_LED", color)
            if result:
                self.log_info(f"Setting PSU_LED to {color}")
            else:
                self.log_error(f"Failed to set PSU_LED to {color}")

    def update_fan_status(self):
        all_fans_ok = True
        for fan_drawer in self.chassis._fan_drawer_list:
            if fan_drawer.get_presence() and fan_drawer.get_status():
                self._set_fan_drawer_led(fan_drawer, FAN_DRAWER_OK)
            else:
                self._set_fan_drawer_led(fan_drawer, FAN_DRAWER_NOT_OK)
                all_fans_ok = False
        if all_fans_ok:
            self._set_fan_led(FANS_OK)
        else:
            self._set_fan_led(FANS_NOT_OK)

    def update_psu_status(self):
        all_psus_ok = True
        for psu in self.chassis._psu_list:
            if not psu.get_presence() or not psu.get_status():
                all_psus_ok = False
        if all_psus_ok:
            self._set_psu_led(PSUS_OK)
        else:
            self._set_psu_led(PSUS_NOT_OK)

    def run(self):
        self.update_fan_status()
        self.update_psu_status()


def main():
    exitter = GracefulExitter(SYSLOG_IDENTIFIER)
    system_ledd = SystemLedd()

    while not exitter.should_exit():
        system_ledd.run()
        exitter.sleep_respect_exit(UPDATE_INTERVAL_SEC)


if __name__ == "__main__":
    main()
