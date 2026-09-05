#!/usr/bin/env python

########################################################################
# DellEMC N3248PXE
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Fan-Drawers' information available in the platform.
#
########################################################################

try:
    from sonic_platform_base.fan_drawer_base import FanDrawerBase
    from sonic_platform.fan import Fan
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

N3248PXE_FANS_PER_FANTRAY = 2


class FanDrawer(FanDrawerBase):
    """DellEMC Platform-specific Fan class"""

    def __init__(self, fantray_index):

        FanDrawerBase.__init__(self)
        # FanTray is 1-based in DellEMC platforms
        self.fantrayindex = fantray_index + 1
        for i in range(N3248PXE_FANS_PER_FANTRAY):
            self._fan_list.append(Fan(fantray_index, i))

    def get_name(self):
        """
        Retrieves the fan drawer name
        Returns:
            string: The name of the device
        """
        return "FanTray{}".format(self.fantrayindex)
