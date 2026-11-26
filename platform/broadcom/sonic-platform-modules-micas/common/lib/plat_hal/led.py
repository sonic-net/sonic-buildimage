#!/usr/bin/env python3
#
# Copyright (C) 2024 Micas Networks Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

from plat_hal.devicebase import devicebase


class led(devicebase):
    def __init__(self, conf=None):
        if conf is not None:
            self.name = conf.get('name', None)
            self.led_type = conf.get('led_type', None)
            self.led_config = conf.get('led', None)
            self.led_color_scheme = conf.get('led_color_scheme', None)

            led_attrs = conf.get('led_attrs')
            led_map = conf.get('led_map')
            if self.led_color_scheme is not None:
                ret, led_color_scheme_ori = self.get_value(self.led_color_scheme)
                if ret:
                    led_color_scheme = self.led_color_scheme.get('ledcolorschemedecode').get(led_color_scheme_ori, "default_scheme")
                else:
                    led_color_scheme = "default_scheme"
                self.led_attrs_config = self._get_led_config(led_attrs, led_color_scheme)
                self.led_map = self._get_led_config(led_map, led_color_scheme)
            else:
                self.led_attrs_config = led_attrs
                self.led_map = led_map

    def _get_led_config(self, config, color_scheme):
        """
        Select LED configuration according to color scheme.

        Supports:
        1. Single configuration:
            {"off": 0x00, "green": 0x02}

        2. Multiple configurations:
            NOTE: default configuration is MUST for this type.
            {
                scheme1: {...},
                scheme2: {...},
                ...,
                "default_scheme": {...}
            }
        """
        if "default_scheme" in config:
            if color_scheme in config:
                return config[color_scheme]
            return config["default_scheme"]

        return config

    def set_color(self, color):
        status = self.led_attrs_config.get(color, None)
        if status is None:
            return False

        mask = self.led_attrs_config.get('mask', 0xff)

        if isinstance(self.led_config, list):
            for led_config_index in self.led_config:
                ret, value = self.get_value(led_config_index)
                if (ret is False) or (value is None):
                    return False
                setval = (int(value) & ~mask) | (status)
                ret, val = self.set_value(led_config_index, setval)
                if ret is False:
                    return ret
        else:
            ret, value = self.get_value(self.led_config)
            if (ret is False) or (value is None):
                return False
            setval = (int(value) & ~mask) | (status)
            ret, val = self.set_value(self.led_config, setval)
        return ret

    def get_color(self):
        mask = self.led_attrs_config.get('mask', 0xff)
        ret, value = self.get_value(self.led_config)
        if ret is False or value is None:
            return False, 'N/A'
        ledval = int(value) & mask
        if self.led_map is not None:
            led_color = self.led_map.get(ledval, None)
            if led_color is None:
                return False, 'N/A'
            return True, led_color
        for key, val in self.led_attrs_config.items():
            if (ledval == val) and (key != "mask"):
                return True, key
        return False, 'N/A'
