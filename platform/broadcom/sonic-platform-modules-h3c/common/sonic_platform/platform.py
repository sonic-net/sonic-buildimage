#!/usr/bin/env python
"""
Version: 1.0

H3C B40X0
Module contains an implementation of SONiC Platform Base API and
provides the platform information
"""
try:
    from sonic_platform_base.platform_base import PlatformBase
    from sonic_platform.chassis import Chassis
except ImportError as error:
    raise ImportError(str(error) + "- required module not found")


class Platform(PlatformBase):
    """
    Platform-specific Platform class
    """

    dev_list_default = ['component', 'fan', 'psu', 'thermal', 'sfp', 'watchdog', 'syseeprom', 'sensor']

    def __init__(self, dev_list=dev_list_default):
        super(Platform, self).__init__(Chassis(dev_list))
