#!/usr/bin/env python

from __future__ import print_function

import imp
import os

try:
    from sonic_platform_base.sfp_base import SfpBase
    from sonic_py_common import device_info
except ImportError as e:
    raise ImportError("%s - required module not found" % e)

USR_SHARE_SONIC_PATH = "/usr/share/sonic"
HOST_DEVICE_PATH = USR_SHARE_SONIC_PATH + "/device"
CONTAINER_PLATFORM_PATH = USR_SHARE_SONIC_PATH + "/platform"

class Sfp(SfpBase):
    """
    Platform-specific sfp class

    Unimplemented methods:
    - get_model
    - get_serial
    - get_status
    - get_transceiver_info
    - get_transceiver_bulk_status
    - get_transceiver_threshold_info
    - get_reset_status
    - get_rx_los
    - get_tx_fault
    - get_tx_disable_channel
    - get_power_override
    - get_temperature
    - get_voltage
    - get_tx_bias
    - get_rx_power
    - get_tx_power
    - tx_disable_channel
    - set_power_override
    """

    def __init__(self, index):
        self._index = index
        
        if os.path.isdir(CONTAINER_PLATFORM_PATH):
            platform_path = CONTAINER_PLATFORM_PATH
        else:
            platform = device_info.get_platform()
            if platform is None:
                return
            platform_path = os.path.join(HOST_DEVICE_PATH, platform)

        module_file = "/".join([platform_path, "plugins", "sfputil.py"])
        module = imp.load_source("sfputil", module_file)
        sfp_util_class = getattr(module, "SfpUtil")
        self._sfputil = sfp_util_class()

    def get_id(self):
        return self._index

    def get_name(self):
        return "Ethernet{}".format(self._index)

    def get_lpmode(self):
        return False

    def set_lpmode(self, lpmode):
        return False

    def get_tx_disable(self):
        return False

    def tx_disable(self, tx_disable):
         return False

    def reset(self):
            pass

    def clear_interrupt(self):
        return False

    def get_interrupt_file(self):
        return None

    def _get_sfputil(self):
        return self._sfputil

    def get_presence(self):
        return self._get_sfputil().get_presence(self._index)

    def get_transceiver_info(self):
        return self._get_sfputil().get_transceiver_info_dict(self._index)

    def get_transceiver_bulk_status(self):
        return self._get_sfputil().get_transceiver_dom_info_dict(self._index)

    def get_transceiver_threshold_info(self):
        return self._get_sfputil().get_transceiver_dom_threshold_info_dict(self._index)

    def get_transceiver_change_event(self, timeout):
        return self._get_sfputil().get_transceiver_change_event(timeout)
