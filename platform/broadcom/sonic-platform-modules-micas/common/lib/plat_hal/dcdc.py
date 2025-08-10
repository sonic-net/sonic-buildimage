#!/usr/bin/env python3
from plat_hal.devicebase import devicebase
from plat_hal.sensor import sensor


class dcdc(devicebase):
    def __init__(self, conf=None):
        if conf is not None:
            self.name = conf.get('name', None)
            self.dcdc_id = conf.get("dcdc_id", None)
            self.sensor = sensor(conf)
