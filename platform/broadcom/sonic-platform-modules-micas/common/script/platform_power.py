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

from platform_config import PLATFORM_POWER_CONF
from platform_util import get_value, get_format_value


class Power(object):
    def __init__(self, conf):
        self.name = None
        self.format = None
        self.unit = None
        self.val_conf = None
        self.value = 0
        self.child_list = []
        self.status = True
        self.pre_check = None
        self.__dict__.update(conf)

    def update_value(self):
        try:
            if self.pre_check is not None:
                ret, val = get_value(self.pre_check)
                if ret is False:
                    self.status = False
                    self.value = "ERR: %s" % val
                    return
                mask = self.pre_check.get("mask")
                if isinstance(val, str):
                    value = int(val, 16)
                else:
                    value = val
                ttt = value & mask
                okval = self.pre_check.get("okval")
                if ttt != okval:
                    self.status = False
                    self.value = "%s" % self.pre_check.get("not_ok_msg")
                    return

            val_list = []
            for val_conf_item in self.val_conf:
                ret, val_tmp = get_value(val_conf_item)
                if ret is False:
                    self.status = False
                    self.value = "ERR: %s" % val_tmp
                    return
                val_list.append(val_tmp)
            val_tuple = tuple(val_list)
            value = get_format_value(self.format % (val_tuple))
            self.status = True
            self.value = round(float(value), 1)
            return
        except Exception as e:
            self.status = False
            self.value = "ERR: %s" % str(e)
            return


def run():
    if len(PLATFORM_POWER_CONF) == 0:
        print("platform_power config error, config len is 0!")
        return

    power_obj_list = []
    for power_item in PLATFORM_POWER_CONF:
        power_obj = Power(power_item)
        tmp_value = 0
        power_obj.child_list = []
        children = power_item.get("children")
        # get power value with children
        if children is not None:
            for child in children:
                child_obj = Power(child)
                power_obj.child_list.append(child_obj)
                child_obj.update_value()
                if child_obj.status is True:
                    tmp_value += child_obj.value
            power_obj.value = round(float(tmp_value), 1)
        else:
            power_obj.update_value()

        if power_obj.status is False:
            print("%-34s : %s" % (power_obj.name, power_obj.value))
        else:
            print("%-34s : %s %s" % (power_obj.name, power_obj.value, power_obj.unit))
        if len(power_obj.child_list) != 0:
            for obj in power_obj.child_list:
                if obj.status is False:
                    print("    %-30s : %s" % (obj.name, obj.value))
                else:
                    print("    %-30s : %s %s" % (obj.name, obj.value, obj.unit))
        print("")
    return


if __name__ == "__main__":
    run()
