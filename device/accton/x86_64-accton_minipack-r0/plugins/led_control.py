# led_control.py
#
# Platform-specific LED control functionality for SONiC
#

try:
    from sonic_led.led_control_base import LedControlBase
    import threading
    import os
    import logging
    import struct
    import time
    import syslog
    from socket import *
    from select import *
    from minipack.pimutil import PimUtil
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


class LedControl(LedControlBase):
    """Platform specific LED control class"""
    SONIC_PORT_NAME_PREFIX = "Ethernet"

    def __init__(self):
        pim = PimUtil()
        pim.init_pim_fpga()

    def _port_name_to_index(self, port_name):
        # Strip "Ethernet" off port name
        if not port_name.startswith(self.SONIC_PORT_NAME_PREFIX):
            return -1

        port_idx = int(port_name[len(self.SONIC_PORT_NAME_PREFIX):])
        return port_idx

    def _port_state_to_mode(self, port_idx, state):
        if state == "up":
            return 1, 4  # port linkup, led is green
        else:
            return 0, 0  # port linkdown, led is off

    def port_link_state_change(self, portname, state):
        pim = PimUtil()
        port_idx = self._port_name_to_index(portname)
        new_control, led_mode = self._port_state_to_mode(port_idx, state)
        color, control = pim.get_port_led(port_idx)

        if color == led_mode:
            if control == new_control:
                return

        pim.set_port_led(port_idx, led_mode, new_control)  # port linkup, led is green
        # port linkdown, led is off
