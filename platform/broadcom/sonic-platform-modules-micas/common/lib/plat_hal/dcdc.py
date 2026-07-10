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
from plat_hal.sensor import sensor, sensor_s3ip

class dcdc(devicebase):
    def __init__(self, conf = None, s3ip_conf = None):
        if conf is not None:
            self.name = conf.get('name', None)
            self.dcdc_id = conf.get("dcdc_id", None)
            self.sensor = sensor(conf)
        if s3ip_conf is not None:
            self.sensor = sensor_s3ip(s3ip_conf)
            self.name = self.sensor.name
            self.dcdc_id = self.sensor.sensor_id
