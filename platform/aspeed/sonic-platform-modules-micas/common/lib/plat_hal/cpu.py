#!/usr/bin/env python_nos
###############################################################################
#
# Hardware Abstraction Layer APIs -- CPU APIs.
#
###############################################################################
from plat_hal.devicebase import devicebase

MASTER_FLASH = 0
SLAVE_FLASH = 1


class cpu(devicebase):

    def __init__(self, conf=None):
        if conf is not None:
            self.name = conf.get('name', None)
            self.cpu_reset_cnt_reg = conf.get('CpuResetCntReg', None)
            self.boot_source = conf.get('boot_source', None)

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

    def get_cpu_boot_source(self):
        """
        get cpu boot source.
        """
        if self.boot_source is None:
            msg = "ERR: not support get boot source"
            self.logger_debug(msg)
            return False, msg
        ret, boot_source = self.get_value(self.boot_source)
        if ret is False or boot_source is None:
            msg = "ERR: get boot source, result:%s" % boot_source
            self.logger_debug(msg)
            return False, msg
        else:
            boot_source_val = int(boot_source, 10)
            if boot_source_val == MASTER_FLASH or boot_source_val == SLAVE_FLASH:
                return True, boot_source_val
            else:
                msg = "ERR: boot source invalid, result:%s" % boot_source
                self.logger_debug(msg)
                return False, msg
