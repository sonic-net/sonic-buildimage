try:
    from sonic_platform_base.fan_drawer_base import FanDrawerBase
    from sonic_platform.fan import Fan
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class FanDrawer(FanDrawerBase):
    """ Platform-specific Fan class"""

    def __init__(self, fantray_index, nums, attr_path):
        FanDrawerBase.__init__(self)
        self.fantrayindex = fantray_index
        for i in range(nums):
            self._fan_list.append(Fan(fantray_index, i, attr_path))

    def get_name(self):
        """
        Retrieves the fan drawer name
        Returns:
            string: The name of the device
        """
        return "FanTray{}".format(self.fantrayindex+1)

