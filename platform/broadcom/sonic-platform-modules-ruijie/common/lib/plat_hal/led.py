#!/usr/bin/env python3
#######################################################
#
# led.py
# Python implementation of the Class led
#
#######################################################
from plat_hal.devicebase import devicebase


class led(devicebase):
    def __init__(self, conf=None):
        if conf is not None:
            self.name = conf.get('name', None)
            self.led_type = conf.get('led_type', None)
            self.led_attrs_config = conf.get('led_attrs', None)
            self.led_config = conf.get('led', None)

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
        for key, val in self.led_attrs_config.items():
            if (ledval == val) and (key != "mask"):
                return True, key
        return False, 'N/A'
