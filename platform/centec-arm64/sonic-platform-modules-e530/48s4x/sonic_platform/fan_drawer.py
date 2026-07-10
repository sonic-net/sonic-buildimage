#!/usr/bin/env python

########################################################################
# Centec E530 48s4x
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Fan-Drawers' information available in the platform.
#
########################################################################

try:
    from sonic_platform_base.fan_drawer_base import FanDrawerBase
    from .fan import Fan
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

CENTEC_FANS_PER_FANTRAY = 4


class FanDrawer(FanDrawerBase):
    """Centec E530 48s4x Platform-specific Fan class"""

    def __init__(self, fantray_index):

        FanDrawerBase.__init__(self)
        self.fantrayindex = fantray_index
        for i in range(CENTEC_FANS_PER_FANTRAY):
            self._fan_list.append(Fan(fantray_index, i))

    def get_name(self):
        """
        Retrieves the fan drawer name
        Returns:
            string: The name of the device
        """
        return "FanTray{}".format(self.fantrayindex)
