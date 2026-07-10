#!/usr/bin/env python
#
# Name: platform.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs 
#


try:
    from sonic_platform_base.platform_base import PlatformBase
    from sonic_platform.chassis import Chassis
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")
 
 
class Platform(PlatformBase):

    def __init__(self):
        PlatformBase.__init__(self)
        self._chassis = Chassis()