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

import time
from plat_hal.devicebase import devicebase


class sensor(devicebase):

    __Value = None
    __Min = None
    __Max = None
    __Low = None
    __High = None
    __ValueConfig = None
    __Flag = None
    __Unit = None
    __format = None
    __read_times = None

    __Min_config = None
    __Max_config = None
    __Low_config = None
    __High_config = None

    @property
    def Min_config(self):
        return self.__Min_config

    @Min_config.setter
    def Min_config(self, val):
        self.__Min_config = val

    @property
    def Max_config(self):
        return self.__Max_config

    @Max_config.setter
    def Max_config(self, val):
        self.__Max_config = val

    @property
    def Low_config(self):
        return self.__Low_config

    @Low_config.setter
    def Low_config(self, val):
        self.__Low_config = val

    @property
    def High_config(self):
        return self.__High_config

    @High_config.setter
    def High_config(self, val):
        self.__High_config = val

    @property
    def Unit(self):
        return self.__Unit

    @Unit.setter
    def Unit(self, val):
        self.__Unit = val

    @property
    def format(self):
        return self.__format

    @format.setter
    def format(self, val):
        self.__format = val

    @property
    def read_times(self):
        return self.__read_times

    @read_times.setter
    def read_times(self, val):
        self.__read_times = val

    @property
    def ValueConfig(self):
        return self.__ValueConfig

    @ValueConfig.setter
    def ValueConfig(self, val):
        self.__ValueConfig = val

    @property
    def Flag(self):
        return self.__Flag

    @Flag.setter
    def Flag(self, val):
        self.__Flag = val

    def get_median(self, value_config, read_times):
        val_list = []
        for i in range(0, read_times):
            ret, real_value = self.get_value(value_config)
            if ret is False or real_value is None:
                if i != (read_times - 1):
                    time.sleep(0.01)
                continue
            val_list.append(real_value)
        val_list.sort()
        if val_list:
            return True, val_list[int((len(val_list) - 1) / 2)]
        return False, None

    @property
    def Value(self):
        try:
            ret, val = self.get_median(self.ValueConfig, self.read_times)
            if ret is False or val is None:
                return None
            if self.format is None:
                self.__Value = int(val)
            else:
                self.__Value = self.get_format_value(self.format % val)
            self.__Value = round(float(self.__Value), 3)
        except Exception:
            return None
        return self.__Value

    @Value.setter
    def Value(self, val):
        self.__Value = val

    @property
    def Min(self):
        try:
            if isinstance(self.Min_config, dict):
                if self.call_back is None:
                    self.__Min = self.Min_config.get("other")
                else:
                    ret = self.call_back()
                    if ret not in self.Min_config:
                        self.__Min = self.Min_config.get("other")
                    else:
                        self.__Min = self.Min_config[ret]
            else:
                self.__Min = self.Min_config

            if self.__Min is None:
                return None

            if self.format is not None:
                self.__Min = self.get_format_value(self.format % self.__Min)
            self.__Min = round(float(self.__Min), 3)
        except Exception:
            return None
        return self.__Min

    @Min.setter
    def Min(self, val):
        self.__Min = val

    @property
    def Max(self):
        try:
            if isinstance(self.Max_config, dict):
                if self.call_back is None:
                    self.__Max = self.Max_config.get("other")
                else:
                    ret = self.call_back()
                    if ret not in self.Max_config:
                        self.__Max = self.Max_config.get("other")
                    else:
                        self.__Max = self.Max_config[ret]
            else:
                self.__Max = self.Max_config

            if self.__Max is None:
                return None

            if self.format is not None:
                self.__Max = self.get_format_value(self.format % self.__Max)
            self.__Max = round(float(self.__Max), 3)
        except Exception:
            return None
        return self.__Max

    @Max.setter
    def Max(self, val):
        self.__Max = val

    @property
    def Low(self):
        try:
            if isinstance(self.Low_config, dict):
                if self.call_back is None:
                    self.__Low = self.Low_config.get("other")
                else:
                    ret = self.call_back()
                    if ret not in self.Low_config:
                        self.__Low = self.Low_config.get("other")
                    else:
                        self.__Low = self.Low_config[ret]
            else:
                self.__Low = self.Low_config

            if self.__Low is None:
                return None

            if self.format is not None:
                self.__Low = self.get_format_value(self.format % self.__Low)
            self.__Low = round(float(self.__Low), 3)
        except Exception:
            return None
        return self.__Low

    @Low.setter
    def Low(self, val):
        self.__Low = val

    @property
    def High(self):
        try:
            if isinstance(self.High_config, dict):
                if self.call_back is None:
                    self.__High = self.High_config.get("other")
                else:
                    ret = self.call_back()
                    if ret not in self.High_config:
                        self.__High = self.High_config.get("other")
                    else:
                        self.__High = self.High_config[ret]
            else:
                self.__High = self.High_config

            if self.__High is None:
                return None

            if self.format is not None:
                self.__High = self.get_format_value(self.format % self.__High)
            self.__High = round(float(self.__High), 3)
        except Exception:
            return None
        return self.__High

    @High.setter
    def High(self, val):
        self.__High = val

    def __init__(self, conf=None, call_back=None):
        self.ValueConfig = conf.get("value", None)
        self.Flag = conf.get("flag", None)
        self.Min_config = conf.get("Min", None)
        self.Max_config = conf.get("Max", None)
        self.Low_config = conf.get("Low", None)
        self.High_config = conf.get("High", None)
        self.Unit = conf.get('Unit', None)
        self.format = conf.get('format', None)
        self.read_times = conf.get('read_times', 1)
        self.call_back = call_back

    def __str__(self):
        formatstr =  \
            "ValueConfig:                : %s \n" \
            "Min :          %s \n" \
            "Max : %s \n" \
            "Unit  : %s \n" \
            "format:       : %s \n"

        tmpstr = formatstr % (self.ValueConfig, self.Min,
                              self.Max, self.Unit,
                              self.format)
        return tmpstr

class sensor_s3ip(sensor):
    def __init__(self, s3ip_conf):
        self.s3ip_conf = s3ip_conf
        value_conf = {}
        value_conf["loc"] = "%s/%s/%s" % (self.s3ip_conf.get("path"), self.s3ip_conf.get("sensor_dir"), "value")
        value_conf["way"] = "sysfs"
        conf = {}
        conf["value"] = value_conf
        conf["read_times"] = s3ip_conf.get("read_times", 1)
        conf["Unit"] = unit = s3ip_conf.get("Unit", None)
        conf["format"] = unit = s3ip_conf.get("format", None)
        super(sensor_s3ip, self).__init__(conf)
        self.min_path = "%s/%s/%s" % (self.s3ip_conf.get("path"), self.s3ip_conf.get("sensor_dir"), "min")
        self.max_path = "%s/%s/%s" % (self.s3ip_conf.get("path"), self.s3ip_conf.get("sensor_dir"), "max")
        self.alias = "%s/%s/%s" % (self.s3ip_conf.get("path"), self.s3ip_conf.get("sensor_dir"), "alias")
        self.sensor_id = self.s3ip_conf.get("type").upper()

    @property
    def Min(self):
        try:
            ret, val = self.get_sysfs(self.min_path)
            if ret is True:
                return val
        except Exception:
            pass
        return None

    @Min.setter
    def Min(self, val):
        try:
            return self.set_sysfs(self.min_path, val)
        except Exception as e:
            return False, (str(e) + " location[%s][%d]" % (self.min_path, val))

    @property
    def Max(self):
        try:
            ret, val = self.get_sysfs(self.max_path)
            if ret is True:
                return val
        except Exception:
            pass
        return None

    @Max.setter
    def Max(self, val):
        try:
            return self.set_sysfs(self.max_path, val)
        except Exception as e:
            return False, (str(e) + " location[%s][%d]" % (self.max_path, val))

    @property
    def name(self):
        try:
            ret, val = self.get_sysfs(self.alias)
            if ret is True:
                return val
        except Exception:
            pass
        return None

