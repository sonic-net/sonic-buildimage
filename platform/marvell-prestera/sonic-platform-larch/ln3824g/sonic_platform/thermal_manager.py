from sonic_platform_base.sonic_thermal_control.thermal_manager_base import ThermalManagerBase

from .thermal_actions import *
from .thermal_conditions import *
from .thermal_infos import *

class ThermalManager(ThermalManagerBase):

    @classmethod
    def start_thermal_algorithm(cls):
        """
        Start vendor specific thermal control algorithm.
        :return:
        """
        pass

    @classmethod
    def stop_thermal_algorithm(cls):
        """
        Stop vendor specific thermal control algorithm.
        :return:
        """
        pass
