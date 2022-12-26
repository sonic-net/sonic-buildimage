#!/usr/bin/env python3
###############################################################################
#
# Hardware Abstraction Layer APIs -- CPU APIs.
#
# Copyright (C) Ruijie, INC.
#
###############################################################################
from plat_hal.devicebase import devicebase


class cpu(devicebase):

    def __init__(self, conf=None):
        if conf is not None:
            self.name = conf.get('name', None)
            self.cpu_reset_cnt_reg = conf.get('CpuResetCntReg', None)

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
