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


class cpu(devicebase):

    def __init__(self, conf=None):
        if conf is not None:
            self.name = conf.get('name', None)
            self.cpu_reset_cnt_reg = conf.get('CpuResetCntReg', None)
            self.reboot_cause_path = conf.get('reboot_cause_path', "/etc/sonic/.reboot/.previous-reboot-cause.txt")

    def get_cpu_reset_num(self):
        """
        get cpu reset num.
        @return cpu reset number, -1 for failure
        """
        ret = -1
        if self.cpu_reset_cnt_reg is None:
            self.logger_debug("ERR: no support get cpu reset num")
            return ret
        ret, reset_num = self.get_value(self.cpu_reset_cnt_reg)
        if ret is False or reset_num is None:
            self.logger_debug("ERR: i2c read cpu_reset_cnt_reg,result:%s" % reset_num)
        else:
            if isinstance(reset_num, str):
                ret = int(reset_num, 16)
            else:
                ret = reset_num
        return ret

    def get_cpu_reboot_cause(self):
        """
        get_cpu_reboot_cause
        @return cpu reset number, -1 for failure
        """
        try:
            with open(self.reboot_cause_path) as fd:
                reboot_cause = fd.read().strip()
            return reboot_cause
        except Exception:
            return "Unknown reboot cause"

