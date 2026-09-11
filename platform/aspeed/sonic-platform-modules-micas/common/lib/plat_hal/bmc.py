#!/usr/bin/env python_nos
###############################################################################
#
# Hardware Abstraction Layer APIs -- BMC APIs.
#
###############################################################################
from plat_hal.devicebase import devicebase

MASTER_FLASH = 0
SLAVE_FLASH = 1


class bmc(devicebase):

    def __init__(self, conf=None):
        if conf is not None:
            self.name = conf.get('name', None)
            self.boot_source = conf.get('boot_source', None)

    def get_bmc_boot_source(self):
        """
        get bmc boot source.
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
