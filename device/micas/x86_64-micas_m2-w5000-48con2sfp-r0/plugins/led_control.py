#!/usr/bin/env python3
"""
Platform-specific LED control plugin for SONiC ledd.

ledd loads LedControl with no arguments and calls:

    port_link_state_change(port_name, port_state)

port_state is 'up' or 'down'. The daemon exits with code 2 if the plugin
raises, so all errors are handled inside this module.

This platform's SFP link LEDs are normally driven by the wb_amd_xgbe driver.
The plugin additionally supports HAL-defined port LEDs and CPLD sysfs port
LED nodes when they are available; otherwise it remains a safe no-op.
"""

import glob
import os
import re
import syslog

try:
    from plat_hal.interface import interface  # type: ignore
except Exception:
    interface = None


class LedControl(object):
    PORT_UP = "up"
    PORT_DOWN = "down"

    HAL_LED_NAME_TEMPLATES = (
        "PORT{}_LED",
        "PORT{}_LINK_LED",
        "ETH{}_LED",
    )
    HAL_COLOR_UP = "green"
    HAL_COLOR_DOWN = "off"

    CPLD_SYSFS_LED_TEMPLATE = "/sys/bus/i2c/devices/5-0030/port_led{}"
    SYSFS_LED_ON = "0x09"
    SYSFS_LED_OFF = "0x08"

    def __init__(self):
        self.int_case = None
        self._last_state = {}
        self._port_index_map = self._load_port_index_map()

        if interface is not None:
            try:
                self.int_case = interface()
            except Exception as exc:
                self._log("failed to initialize plat_hal interface: {}".format(exc))

    @staticmethod
    def _log(message, priority=syslog.LOG_INFO):
        try:
            syslog.openlog("ledd", syslog.LOG_PID)
            syslog.syslog(priority, "led_control: {}".format(message))
        except Exception:
            pass

    def _get_platform_name(self):
        for conf in ("/host/machine.conf", "/etc/sonic/machine.conf"):
            try:
                with open(conf, "r") as fd:
                    for line in fd:
                        if line.startswith("onie_platform="):
                            return line.split("=", 1)[1].strip()
            except Exception:
                continue
        return None

    def _load_port_index_map(self):
        port_index_map = {}
        candidates = []

        try:
            from sonic_py_common.device_info import get_paths_to_platform_and_hwsku_dirs  # type: ignore
            platform_path, hwsku_path = get_paths_to_platform_and_hwsku_dirs()
            candidates.append(os.path.join(hwsku_path, "port_config.ini"))
            candidates.append(os.path.join(platform_path, "port_config.ini"))
        except Exception:
            pass

        platform = self._get_platform_name()
        if platform:
            candidates.extend(glob.glob(
                "/usr/share/sonic/device/{}/*/port_config.ini".format(platform)
            ))

        for path in candidates:
            if not path or not os.path.isfile(path):
                continue
            try:
                with open(path, "r") as fd:
                    for line in fd:
                        line = line.strip()
                        if not line or line.startswith("#"):
                            continue
                        fields = line.split()
                        if len(fields) < 4:
                            continue
                        try:
                            port_index_map[fields[0]] = int(fields[3])
                        except ValueError:
                            continue
                if port_index_map:
                    break
            except Exception as exc:
                self._log("failed to read {}: {}".format(path, exc))
        return port_index_map

    def _get_port_number(self, port_name):
        if port_name in self._port_index_map:
            return self._port_index_map[port_name]

        match = re.search(r"(\d+)$", port_name)
        if match:
            return int(match.group(1)) + 1
        return None

    def _set_hal_led(self, port_number, color):
        if self.int_case is None or port_number is None:
            return False

        led_names = []
        for template in self.HAL_LED_NAME_TEMPLATES:
            led_names.append(template.format(port_number))

        for led_name in led_names:
            try:
                ret = self.int_case.set_led_color(led_name, color)
                if ret is True or ret == 1:
                    return True
            except Exception:
                continue
        return False

    def _set_sysfs_led(self, port_number, value):
        if port_number is None:
            return False

        led_index = port_number - 1
        path = self.CPLD_SYSFS_LED_TEMPLATE.format(led_index)
        try:
            if os.path.isfile(path):
                with open(path, "w") as fd:
                    fd.write(value)
                return True
        except Exception as exc:
            self._log("failed to write {}: {}".format(path, exc))
        return False

    def port_link_state_change(self, port_name, port_state):
        """
        Called by ledd on front-panel port link state changes.

        Args:
            port_name: Logical port name, e.g. 'Ethernet0'.
            port_state: 'up' or 'down'.
        """
        try:
            if port_state not in (self.PORT_UP, self.PORT_DOWN):
                self._log("ignore invalid port state {!r} for {}".format(port_state, port_name))
                return

            if self._last_state.get(port_name) == port_state:
                return

            port_number = self._get_port_number(port_name)
            if port_state == self.PORT_UP:
                hal_color = self.HAL_COLOR_UP
                sysfs_value = self.SYSFS_LED_ON
            else:
                hal_color = self.HAL_COLOR_DOWN
                sysfs_value = self.SYSFS_LED_OFF

            if self._set_hal_led(port_number, hal_color):
                self._last_state[port_name] = port_state
                return

            if self._set_sysfs_led(port_number, sysfs_value):
                self._last_state[port_name] = port_state
                return

            # No software-controlled LED is configured for this port. The
            # kernel driver still controls the physical SFP LED, so this is
            # not an error and must not be propagated to ledd.
            self._last_state[port_name] = port_state
        except Exception as exc:
            self._log("port_link_state_change failed: {}".format(exc), syslog.LOG_ERR)

    def __del__(self):
        pass
