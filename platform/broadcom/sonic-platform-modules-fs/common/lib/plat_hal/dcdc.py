#!/usr/bin/env python3
from plat_hal.devicebase import devicebase
from plat_hal.sensor import sensor, sensor_s3ip

class dcdc(devicebase):
    def __init__(self, conf = None, s3ip_conf = None, id = None):
        if conf is not None:
            self.name = conf.get('name', None)
            self.dcdc_id = conf.get("dcdc_id", None)
            self.sensor = sensor(conf)
        if s3ip_conf is not None:
            self.sensor = sensor_s3ip(s3ip_conf)
            self.name = self.sensor.name
            self.dcdc_id = id

